#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <Windows.h>

int screenHeight = 40;
int screenWidth = 120;

float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerAngle = 0.0f;

int mapHeight = 16;
int mapWidth = 16;

float fFOV = 3.1415 / 4.0;
float fDepth = 16.0f;

int main()
{
	//Create and set console buffer
	wchar_t *screen = new wchar_t[screenHeight * screenWidth];
	HANDLE hConBuff = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	if (!SetConsoleActiveScreenBuffer(hConBuff))
	{
		std::cout << "Error creating console buffer.\n";
		return -1;
	}
	DWORD dwBytesWritten = 0;


	//MAP
	std::wstring map;
	map += L"################";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#........##....#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#..............#";
	map += L"################";
	//MAP END.


	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();	

	//Game loop
	while (1)
	{
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		//Controls
		//Handle camera rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
			fPlayerAngle -= 1.0f * fElapsedTime;
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
			fPlayerAngle += 1.0f * fElapsedTime;

		//Handle walking forwards and backwards
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY += sinf(fPlayerAngle) * 5.0f * fElapsedTime;

			//Collision detection
			if (map[(int)fPlayerY * mapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= cosf(fPlayerAngle) * 5.0f * fElapsedTime;
			fPlayerY -= sinf(fPlayerAngle) * 5.0f * fElapsedTime;

			//Collision detection
			if (map[(int)fPlayerY * mapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += cosf(fPlayerAngle) * 5.0f * fElapsedTime;
				fPlayerY += sinf(fPlayerAngle) * 5.0f * fElapsedTime;
			}
		}

		for (int x = 0; x < screenWidth; x++)
		{
			//For each coloumn, calculate the player's projected ray angle into world space	
			float fRayAngle = (fPlayerAngle - fFOV / 2.0f) + ((float)x / (float)screenWidth) * fFOV;

			float fDistToWall = 0.0f;
			bool bHitWall = false;
			bool bBoundary = false;

			//Unit vector for ray in player space
			float fEyeX = cosf(fRayAngle);
			float fEyeY = sinf(fRayAngle);

			//Trace the projected ray and find the distance where it hits the wall
			while (!bHitWall && fDistToWall < fDepth)
			{

				fDistToWall += 0.1f;

				int testX = (int)(fPlayerX + fEyeX * fDistToWall);
				int testY = (int)(fPlayerY + fEyeY * fDistToWall);

				//Test if ray is out of bounds
				if (testX < 0 || testX >= screenWidth || testY < 0 || testY >= screenHeight)
				{
					bHitWall = true;
					fDistToWall = fDepth; //Set distance to maximum depth
				}
				else
				{
					//Ray is within the bounds. Test to see if the ray cell is a wall
					if (map[testY * mapWidth + testX] == '#')
					{
						bHitWall = true;

						std::vector<std::pair<float, float>> p; //distance, dot product(cos of the angle between two unit vectors)

						for (int tx = 0; tx < 2; tx++)
							for (int ty = 0; ty < 2; ty++)
							{
								float vx = (float)testX + tx - fPlayerX;
								float vy = (float)testY + ty - fPlayerY;
								float d = sqrt(vx*vx + vy * vy);
								float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
								p.push_back(std::make_pair(d, dot));
							}

						//Sort pairs from closest to farthest
						std::sort(p.begin(), p.end(), [](const std::pair<float, float> &left, const std::pair<float, float> &right) { return left.first < right.first; });

						float fBound = 0.01f; //Maximum angle between perfect-corner vector and eye vector where we still assume that player is looking right at the corner
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
					}
				}
			}

			//Calculate distance to ceiling and floor
			int ceiling = (float)(screenHeight) / 2.0 - screenHeight / ((float)fDistToWall);
			int floor = screenHeight - ceiling;

			//Pick an appropriate shaded wall symbol depending on fDistToWall
			short shade = ' ';
			if (fDistToWall <= fDepth / 4.0) //Very close - full block (quarter of view distance)
				shade = 0x2588;
			else if (fDistToWall <= fDepth / 3.0) // third of view distance
				shade = 0x2593;
			else if (fDistToWall <= fDepth / 2.0) // half of view distance
				shade = 0x2592;
			else if (fDistToWall < fDepth) // Far away but still within the view distance
				shade = 0x2591;

			if (bBoundary)	shade = ' '; //Black out the boundary between blocks

			for (int y = 0; y < screenHeight; y++)
			{
				if (y <= ceiling)
					screen[y * screenWidth + x] = ' ';
				else if(y > ceiling && y < floor)
					screen[y * screenWidth + x] = shade;
				else
				{
					//Shade floor based on distance
					float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));
					if (b < 0.25f)		shade = '#';
					else if (b < 0.5)	shade = 'x';
					else if (b < 0.75)	shade = '-';
					else if (b < 0.9)	shade = '.';
					else				shade = ' ';

					screen[y * screenWidth + x] = shade;
				}
					
			}
		}


		//Display stats
		swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f, FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerAngle, 1.0f / fElapsedTime);
		
		//Display map
		for (int nx = 0; nx < mapWidth; nx++)
			for (int ny = 0; ny < mapHeight; ny++)
				screen[(ny + 1) * screenWidth + nx] = map[ny * mapWidth + nx];
		screen[((int)fPlayerY + 1) * screenWidth + (int)fPlayerX] = 'P';

		screen[screenHeight * screenWidth - 1] = '\0';
		WriteConsoleOutputCharacterW(hConBuff, screen, screenWidth * screenHeight, { 0, 0 }, &dwBytesWritten);
	}
	

	return 0;
}