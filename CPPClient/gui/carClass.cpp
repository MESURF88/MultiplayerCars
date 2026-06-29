#include "carClass.hpp"
#include "windowContext.hpp"

static constexpr int carWidth = 50;
static constexpr int carHeight = 50;
static const raylib::Color defaultCarColor = raylib::Color::Blue();

ColorHexMap carColor(ColorToInt(defaultCarColor), "0079F1"); // Blue

int getCarWidth()
{
	return carWidth;
}

int getCarHeight()
{
	return carHeight;
}

void setCarColor(int colorHexValue)
{
	carColor.hexValue = colorHexValue;
	carColor.hexString = colorHexToString(colorHexValue);
}

int getCarColorHexValue()
{
	return carColor.hexValue;
}

std::string getCarColorString()
{
	return carColor.hexString;
}

void drawCar(int X, int Y)
{
	DrawRectangle(X, Y, carWidth, carHeight, GetColor(carColor.hexValue));
}

void drawCar(int X, int Y, int colorHexValue)
{
	DrawRectangle(X, Y, carWidth, carHeight, GetColor(colorHexValue));
}

void drawLobbyDriver(int X, int Y)
{
	drawLobbyDriver(X, Y, carColor.hexValue);
}

void drawLobbyDriver(int X, int Y, int colorHexValue)
{
	const Color playerColor = GetColor(colorHexValue);
	const float centerX = static_cast<float>(X) + (static_cast<float>(carWidth) * 0.5f);
	const float centerY = static_cast<float>(Y) + (static_cast<float>(carHeight) * 0.5f);
	const Vector2 center = { centerX, centerY };

	DrawCircleV(center, 36.0f, Fade(playerColor, 0.10f));
	DrawCircleV(center, 28.0f, Fade(playerColor, 0.18f));
	DrawRing(center, 24.0f, 28.0f, 0.0f, 360.0f, 48, Fade(playerColor, 0.86f));
	DrawRing(center, 28.0f, 31.0f, 0.0f, 360.0f, 48, Fade(RAYWHITE, 0.28f));

	DrawRectangleRounded({ centerX - 15.0f, centerY - 3.0f, 30.0f, 28.0f }, 0.18f, 6, GetColor(0x26313AFF));
	DrawRectangleRounded({ centerX - 11.0f, centerY + 1.0f, 22.0f, 20.0f }, 0.14f, 5, playerColor);
	DrawRectangle(centerX - 19.0f, centerY + 2.0f, 8, 20, GetColor(0x1B242BFF));
	DrawRectangle(centerX + 11.0f, centerY + 2.0f, 8, 20, GetColor(0x1B242BFF));
	DrawRectangle(centerX - 13.0f, centerY + 24.0f, 10, 11, GetColor(0x1B242BFF));
	DrawRectangle(centerX + 3.0f, centerY + 24.0f, 10, 11, GetColor(0x1B242BFF));

	DrawRectangleRounded({ centerX - 14.0f, centerY - 27.0f, 28.0f, 26.0f }, 0.24f, 7, GetColor(0xF0C08DFF));
	DrawRectangle(centerX - 16.0f, centerY - 25.0f, 32, 8, GetColor(0x2C241FFF));
	DrawRectangle(centerX - 16.0f, centerY - 18.0f, 6, 10, GetColor(0x2C241FFF));
	DrawRectangle(centerX + 10.0f, centerY - 18.0f, 6, 10, GetColor(0x2C241FFF));
	DrawRectangle(centerX - 8.0f, centerY - 12.0f, 5, 4, GetColor(0x11171DFF));
	DrawRectangle(centerX + 3.0f, centerY - 12.0f, 5, 4, GetColor(0x11171DFF));
	DrawRectangle(centerX - 5.0f, centerY - 3.0f, 10, 3, GetColor(0x8B4A3AFF));

	DrawRectangleRounded({ centerX - 18.0f, centerY - 35.0f, 36.0f, 10.0f }, 0.25f, 6, playerColor);
	DrawRectangle(centerX - 10.0f, centerY - 41.0f, 20, 8, GetColor(0x1B242BFF));
}
