
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <random>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>


bool checkCollision(const std::vector<std::vector<bool>>& world, const std::vector<std::vector<bool>>& shape, int shapeY, int shapeX) {
	for (int y = 0; y < shape.size(); ++y)
		for (int x = 0; x < shape[y].size(); ++x)
			if (shape[y][x] && (y + shapeY < 0 || y + shapeY >= world.size() || x + shapeX < 0 || x + shapeX >= world[y + shapeY].size() || world[y + shapeY][x + shapeX]))
				return true;
	return false;
}

std::vector<std::vector<bool>> rotate_cw(const std::vector<std::vector<bool>>& values) {
	std::vector<std::vector<bool>> rotated;
	for (std::size_t y = 0; y < values[0].size(); ++y) {
		std::vector<bool> row;
		for (std::size_t x = values.size(); x > 0; --x)
			row.push_back(values[x - 1][y]);
		rotated.push_back(row);
	}
	return rotated;
}

int main() {
	const std::vector<std::vector<std::vector<bool>>> SHAPES {
		{
			{ 0, 0, 0, 0 },
			{ 1, 1, 1, 1 },
			{ 0, 0, 0, 0 },
			{ 0, 0, 0, 0 }
		},
		{
			{ 0, 0, 0 },
			{ 1, 1, 0 },
			{ 1, 1, 0 }
		},
		{
			{ 0, 0, 0 },
			{ 1, 1, 1 },
			{ 0, 1, 0 }
		},
		{
			{ 0, 0, 0 },
			{ 1, 1, 1 },
			{ 1, 0, 0 }
		},
		{
			{ 0, 0, 0 },
			{ 1, 1, 1 },
			{ 0, 0, 1 }
		},
		{
			{ 0, 0, 0 },
			{ 1, 1, 0 },
			{ 0, 1, 1 }
		},
		{
			{ 0, 0, 0 },
			{ 0, 1, 1 },
			{ 1, 1, 0 }
		}
	};

	std::mt19937 rng(std::random_device {}());
	std::uniform_int_distribution<> dist(0, SHAPES.size() - 1);

	constexpr int height = 20;
	constexpr int width = 10;
	std::vector<std::vector<bool>> world;
	for (int y = 0; y < height; ++y)
		world.push_back(std::vector<bool>(width, false));

	std::vector<std::vector<bool>> shape = SHAPES[dist(rng)];
	int shapeY = 0;
	int shapeX = width / 2 - shape.size() / 2;

	int score = 0;
	int sinceUpdate = 0;
	bool running = true;

	termios cooked;
	tcgetattr(STDIN_FILENO, &cooked);
	termios raw = cooked;
	cfmakeraw(&raw);
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
	std::cout << "\033[?25l";

	while (true) {
		std::string display;
		for (int y = 1; y < height; ++y) {
			for (int x = 0; x < width; ++x)
				display += world[y][x] ? "# " : ". ";
			display += "\r\n";
		}
		for (int y = 0; y < shape.size(); ++y)
			for (int x = 0; x < shape[y].size(); ++x)
				if (shape[y][x])
					display[(y + shapeY - 1) * width * 2 + (x + shapeX) * 2 + (y + shapeY - 1) * 2] = '@';
	
		std::cout << "\033[2J\033[HScore: " << score << "\r\n" << display;
		std::cout.flush();

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		char input = 0;
		while (read(STDIN_FILENO, &input, 1) == 1);

		switch (input) {
			case 's':
				if (!checkCollision(world, shape, shapeY + 1, shapeX))
					++shapeY;
				break;
			case 'd':
				if (!checkCollision(world, shape, shapeY, shapeX + 1))
					++shapeX;
				break;
			case 'a':
				if (!checkCollision(world, shape, shapeY, shapeX - 1))
					--shapeX;
				break;
			case 'w':
				{
					std::vector<std::vector<bool>> rotated = rotate_cw(shape);
					if (!checkCollision(world, shape, shapeY, shapeX))
						shape = rotated;
				}
				break;
			case 'q':
				running = false;
		}

		++sinceUpdate;
		sinceUpdate %= 10;
		if (!sinceUpdate) {
			if (checkCollision(world, shape, shapeY + 1, shapeX)) {
				if (!shapeY)
					break;
				for (int y = 0; y < shape.size(); ++y)
					for (int x = 0; x < shape[y].size(); ++x)
						if (shape[y][x])
							world[y + shapeY][x + shapeX] = true;
				shape = SHAPES[dist(rng)];
				shapeY = 0;
				shapeX = width / 2 - shape.size() / 2;
			} else
				++shapeY;
		}

		for (int y = 0; y < height; ++y) {
			int x = 0;
			for (; x < world[y].size(); ++x)
				if (!world[y][x])
					break;
			if (x == world[y].size()) {
				++score;
				world.erase(world.begin() + y);
				world.insert(world.begin(), std::vector<bool>(width, false));
			}
		}
	}

Reset:
	tcsetattr(STDIN_FILENO, TCSANOW, &cooked);
	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) & ~O_NONBLOCK);
	std::cout << "\033[?25h";
}
