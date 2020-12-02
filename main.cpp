#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <utility>
#include <chrono>

#include <stdio.h>
#include <windows.h>

int nScreenWidth = 120;
int nScreenHeight = 40;

float fPlayerX = 8.0f;	// Player starting position
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;	// Player starting angle
float fSpeed = 5.0f;	// Walking speed

int nMapWidth = 18;		// World dimensions
int nMapHeight = 18;

float fFOV = 3.14159 / 4.0;		// view angle frome neutral
float fDepth = 18.0;			// maximum rendering distance

int main()
{
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight]; 	// regular 2D array using our screen sizes
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);		// hConsole is a regular textmode buffer. We set it as the target.
	DWORD dwBytesWritten = 0; 	// not really useful but we have to use it anyway (Windows thing)

	// Create map of world space # = wall block, . = space
	std::wstring map;
	map += L"##################";
	map += L"#................#";
	map += L"#.##..######.#...#";
	map += L"#.##..##.....#...#";
	map += L"#.....##.#####...#";
	map += L"#................#";
	map += L"#..########....#.#";
	map += L"#..##..........#.#";
	map += L"#..##..........#.#";
	map += L"#..####........#.#";
	map += L"#..##.....##...#.#";
	map += L"#.........##...#.#";
	map += L"#.##########...#.#";
	map += L"#........#.....#.#";
	map += L"#.....#........#.#";
	map += L"################.#";
	map += L".................#";
	map += L"##################";

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();

	bool bRun = true;
	while (bRun)
	{
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle CCW Rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerA -= (fSpeed * 0.5f) * fElapsedTime;

		// Handle CW Rotation	
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerA += (fSpeed * 0.5f) * fElapsedTime;
		
		// Handle Forwards movement & collision
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
			}	
		}

		// Handle Backwards movement & collision
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
			if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
			{
				fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
			}	
		}

		for (int x = 0; x < nScreenWidth; x++)
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = (fPlayerA - (fFOV / 2.0f)) + ((float)x / (float)nScreenWidth) * fFOV;

			// Find distance to wall
			float fStepSize = 0.1f;	// Increment size for ray casting; decrease to increase resolution
			float fDistanceToWall = 0.0f;
			
			bool bHitWall = false;		// Set when ray hit wall block
			bool bBoundary = false;		// Set when ray hits boundary between two wall blocks

			float fEyeX = sinf(fRayAngle);		// Unit vector for ray in player space
			float fEyeY = cosf(fRayAngle);

			// Incrementally cast ray from player, along ray angle, testing for
			// intersection with a block
			while (!bHitWall && fDistanceToWall < fDepth)
			{
				fDistanceToWall += fStepSize;

				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// Test if ray is out of bounds
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth;	// just set distance to maximum length
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if (map.c_str()[nTestX*nMapWidth + nTestY] == '#')
					{
						// Ray has hit the wall
						bHitWall = true;

						std::vector<std::pair<float, float>> p; 	// distance, dot

						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;
								float d = sqrt(vx*vx + vy*vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}

							// Sort Pairs from closest to farthest with an ugly anonymous function
							sort(p.begin(), p.end(), [](const std::pair<float, float> &left, const std::pair<float, float> &right) {return left.first < right.first; });
							
							float fBound = 0.01;	// radians
							if (acos(p.at(0).second) < fBound) bBoundary = true;
							if (acos(p.at(1).second) < fBound) bBoundary = true;
					}
				}	
			}

			// Calculate the distance to ceiling and floor
			int nCeiling = (float)(nScreenHeight / 2.0f) - (float)nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;

			short nShade = ' ';

			if (fDistanceToWall <= fDepth / 4.0f)		nShade = 0x2588;	// Very close
			else if (fDistanceToWall < fDepth / 3.0f)	nShade = 0x2593;
			else if (fDistanceToWall < fDepth / 2.0f)	nShade = 0x2592;
			else if (fDistanceToWall < fDepth)			nShade = 0x2591;
			else										nShade = ' ';		// Too far
			
			if (bBoundary)								nShade = ' ';	// Black it out

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y <= nCeiling)
					screen[y*nScreenWidth + x] = ' ';
				else if (y > nCeiling && y <= nFloor)
					screen[y*nScreenWidth + x] = nShade;
				else	// Floor
				{
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)		nShade = '#';
					else if (b < 0.5) 	nShade = 'X';
					else if (b < 0.75)	nShade = '.';
					else if (b < 0.9)	nShade = '-';
					else 				nShade = ' ';
					screen[y*nScreenWidth + x] = nShade;
				}
			}
		}

		// Display Stats
		// snprintf((char*)screen, 40, "X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);
		
		// Display Map
		for (int nx = 0; nx < nMapWidth; nx++)
			for (int ny =0; ny < nMapHeight; ny++)
			{
				screen[(ny+1)*nScreenWidth + nx] = map[ny * nMapWidth + nx];
			}
		screen[((int)fPlayerX+1) * nScreenWidth + (int)fPlayerY] = 'P';

		// Display Frame
		screen[nScreenWidth * nScreenHeight - 1] = '\0';	// '\0' is the escape character
		WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth*nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}