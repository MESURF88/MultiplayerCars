#include "windowContext.hpp"
#include "raylib-cpp.hpp" // raylib has to be included in cpp file to avoid name conflicts with windows.h
static constexpr int screenWidth = 1600;
static constexpr int screenHeight = 850;
static constexpr int menuPanelHeight = 120;
static constexpr int textBoxX = 100;
static constexpr int textBoxY = screenHeight - 100;
static constexpr int colorBoxX = 850;
static constexpr int colorBoxY = screenHeight - 110;
static constexpr int colorBoxWidth = 400;
static constexpr int colorBoxHeight = 50;
static constexpr int colorSquareLength = 50;
static constexpr int escBoxWidth = 100;
static constexpr int escBoxHeight = 40;
static constexpr int escBoxX = screenWidth - escBoxWidth - 5;
static constexpr int escBoxY = screenHeight - escBoxHeight - 5;
static constexpr int chatPanelHeight = 160;

static const raylib::Color defaultMainTextColor = raylib::Color::LightGray();
// the one and only window
raylib::Window window(screenWidth, screenHeight, "car sim game alpha");

static const std::map<int, ColorHexMap> colorEnumToHexValue = {
	{colorSelectionType::BLUECOLOR, ColorHexMap(7991807, "0079F1")},
	{colorSelectionType::GREENCOLOR, ColorHexMap(14954751, "00E430")},
	{colorSelectionType::REDCOLOR, ColorHexMap(-433506305, "E62937")},
	{colorSelectionType::MAGENTACOLOR, ColorHexMap(-16711681, "FF00FF")},
	{colorSelectionType::ORANGECOLOR, ColorHexMap(-6225665,"FFA100")},
	{colorSelectionType::YELLOWCOLOR, ColorHexMap(-34012929,"FDF900")},
	{colorSelectionType::SKYBLUECOLOR, ColorHexMap(1723858943,"25FDCB")},
	{colorSelectionType::LIGHTGREYCOLOR, ColorHexMap(-926365441,"C8C8C8")},
};

static const std::map<int, std::string> colorHexValueToString = {
	{ 7991807, "0079F1" },
	{ 14954751, "00E430" },
	{ -433506305, "E62937" },
	{ -16711681, "FF00FF" },
	{ -6225665,"FFA100" },
	{ -34012929,"FDF900" },
	{ 1723858943,"25FDCB" },
	{ -926365441,"C8C8C8" },
};

static const std::map<std::string, int> colorStringToHexValue = {
	{ "0079F1",  7991807 },
	{ "00E430", 14954751 },
	{ "E62937", -433506305 },
	{ "FF00FF", -16711681},
	{ "FFA100", -6225665 },
	{ "FDF900", -34012929 },
	{ "25FDCB", 1723858943 },
	{ "C8C8C8", -926365441 },
};

std::string colorHexToString(int hexValue)
{
	if (colorHexValueToString.count(hexValue))
	{
		return colorHexValueToString.at(hexValue);
	}
	return "0079F1"; //Blue
}

int colorHexToString(std::string hexString)
{
	if (colorStringToHexValue.count(hexString))
	{
		return colorStringToHexValue.at(hexString);
	}
	return 7991807; //Blue
}

int windowScreenWidth()
{
	return screenWidth;
}

int windowScreenHeight()
{
	return screenHeight;
}

int windowYBoundary()
{
	return screenHeight - menuPanelHeight - chatPanelHeight; // account for text box and chatbox
}

void windowSetTargetFPS(int fps)
{
	SetTargetFPS(fps);
}

bool windowShouldCloseWrapper()
{
	return !window.ShouldClose();
}
void windowBeginDrawing()
{
	BeginDrawing();
}

void windowEndDrawing()
{
	EndDrawing();
}

void windowDrawBackground()
{
	window.ClearBackground(RAYWHITE);
}

void drawTextTestBox(std::string testStr)
{
	DrawLine(0, screenHeight - menuPanelHeight, screenWidth, screenHeight - menuPanelHeight, defaultMainTextColor);
	defaultMainTextColor.DrawText("CarGameSim: " + testStr, textBoxX, textBoxY, 20);
}

void drawChatBoxContainer()
{
	DrawLine(0, screenHeight - menuPanelHeight - chatPanelHeight, screenWidth, screenHeight - menuPanelHeight - chatPanelHeight, defaultMainTextColor);
}

void drawDefaultSquaresColor()
{
	int initX = colorBoxX;
	for (int color = BLUECOLOR; color < MAXCOLORSELECTION; color++)
	{
		DrawRectangle(initX, colorBoxY, colorSquareLength, colorBoxHeight, GetColor(colorEnumToHexValue.at(color).hexValue));
		initX += colorSquareLength;
	}
}

void drawEscButton()
{
	DrawRectangleLines(escBoxX, escBoxY, escBoxWidth, escBoxHeight, raylib::Color::Black());
	defaultMainTextColor.DrawText("ESC", escBoxX + 30, escBoxY + 10, 20);
}

void windowCloseWindow()
{
	CloseWindow();
}

bool windowIsKeyPressedUp()
{
	return windowIsKeyPressed(KEY_UP);
}
bool windowIsKeyPressedDown()
{
	return windowIsKeyPressed(KEY_DOWN);
}
bool windowIsKeyPressedLeft()
{
	return windowIsKeyPressed(KEY_LEFT);
}
bool windowIsKeyPressedRight()
{
	return windowIsKeyPressed(KEY_RIGHT);
}
bool windowIsKeyPressed(int key)
{
	return IsKeyDown(key) || IsKeyPressed(key);
}

bool windowIsMouseButtonPressed()
{
	return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

std::map<int, ColorHexMap> windowGetColorSelectionMap()
{
	return colorEnumToHexValue;
}

int windowIsMouseInColorSelection()
{
	Vector2 mouseCoords = GetMousePosition();
	if ((mouseCoords.x <= (colorBoxX + colorBoxWidth)) && (mouseCoords.x >= colorBoxX) && (mouseCoords.y <= (colorBoxY + colorBoxHeight)) && (mouseCoords.y >= colorBoxY))
	{
		if (mouseCoords.x < (colorBoxX + 50))
		{
			return colorSelectionType::BLUECOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50*2))
		{
			return colorSelectionType::GREENCOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50 * 3))
		{
			return colorSelectionType::REDCOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50 * 4))
		{
			return colorSelectionType::MAGENTACOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50 * 5))
		{
			return colorSelectionType::ORANGECOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50 * 6))
		{
			return colorSelectionType::YELLOWCOLOR;
		}
		else if (mouseCoords.x < (colorBoxX + 50 * 7))
		{
			return colorSelectionType::SKYBLUECOLOR;
		}
		else
		{
			return colorSelectionType::LIGHTGREYCOLOR;
		}
	}
	return colorSelectionType::NOCOLOR;
}

bool windowIsMouseInEscape()
{
	Vector2 mouseCoords = GetMousePosition();
	if ((mouseCoords.x <= (escBoxX + escBoxWidth)) && (mouseCoords.x >= escBoxX) && (mouseCoords.y <= (escBoxY + escBoxHeight)) && (mouseCoords.y >= escBoxY))
	{
		return true;
	}
	return false;
}
