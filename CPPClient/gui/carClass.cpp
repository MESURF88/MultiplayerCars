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