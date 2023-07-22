#include "windowContext.hpp"
#include "carClass.hpp"
#include "version.hpp"

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
static constexpr int chatSendBoxHeight = 25;
static constexpr int chatSendBoxWidth = 1500;
static constexpr int racePortalBoxHeight = 125;
static constexpr int racePortalBoxWidth = 75;
static constexpr int racePortalY = screenHeight - racePortalBoxHeight - 600;

static const raylib::Color defaultMainTextColor = raylib::Color::Gray();
// the one and only window
raylib::Window window(screenWidth, screenHeight, "car sim game alpha v" + std::to_string(VERSION_NUM));

static const Rectangle chatSendBoxRect = { 0, screenHeight - menuPanelHeight - chatSendBoxHeight, chatSendBoxWidth, chatSendBoxHeight };
static const Rectangle chatSendButtonRect = { chatSendBoxWidth-1, screenHeight - menuPanelHeight - chatSendBoxHeight, 100, chatSendBoxHeight };
static const Rectangle racePortalRect = { 1, racePortalY, racePortalBoxWidth, racePortalBoxHeight };

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
	DrawLine(0, screenHeight - menuPanelHeight - chatPanelHeight, screenWidth, screenHeight - menuPanelHeight - chatPanelHeight, BLACK);
}

void drawChatSendBox(bool mouseOnText, const char *text)
{
	if(mouseOnText) DrawRectangleLines((int)chatSendBoxRect.x, (int)chatSendBoxRect.y, (int)chatSendBoxRect.width, (int)chatSendBoxRect.height, BLACK);
	else DrawRectangleLines((int)chatSendBoxRect.x, (int)chatSendBoxRect.y, (int)chatSendBoxRect.width, (int)chatSendBoxRect.height, LIGHTGRAY);
	DrawText(text, (int)chatSendBoxRect.x + 5, (int)chatSendBoxRect.y + 5, 20, MAROON);
}

void drawPortalRaceInfoPane(bool playerInRacePortal)
{
	if (playerInRacePortal)
	{
		DrawRectangleLines(120, racePortalY + racePortalBoxHeight + 15, 235, 50, BLACK);
		DrawText("E", 180, racePortalY + racePortalBoxHeight + 18, 20, BLACK);
		DrawText("- Enter Race Portal", 125, racePortalY + racePortalBoxHeight + 35, 20, BLACK);
	}
}

void drawSendTextButton()
{
	DrawRectangleLines((int)chatSendButtonRect.x, (int)chatSendButtonRect.y, (int)chatSendButtonRect.width, (int)chatSendButtonRect.height, BLACK);
	defaultMainTextColor.DrawText("SEND", chatSendButtonRect.x + 25, chatSendButtonRect.y + 5, 20);
}

void drawPortalRectangles(int xPos, int yPos)
{
	DrawRectangleLines((int)racePortalRect.x, (int)racePortalRect.y, (int)racePortalRect.width, (int)racePortalRect.height, ORANGE);
}

void drawChatSendBoxBlinkingUnderscore(const int& framesCounter, const char * text)
{
	if (((framesCounter / 20) % 2) == 0) DrawText("_", (int)chatSendBoxRect.x + 5 + MeasureText(text, 20), (int)chatSendBoxRect.y + 5, 20, MAROON);
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

void drawTextLine(int idx, const TextContext& context)
{
	// idx 0 is the bottom
	int y = screenHeight - menuPanelHeight - chatPanelHeight + (100 - (25 * idx)) + 1; // plus 1 space
	if (colorStringToHexValue.count(context.m_colorStr))
	{
		if (context.m_leftSide)
		{
			DrawRectangle(125, y, 23, 23, GetColor(colorStringToHexValue.at(context.m_colorStr)));
		}
		else
		{
			DrawRectangle(1125, y, 23, 23, GetColor(colorStringToHexValue.at(context.m_colorStr)));
		}
	}
	int sizeUnits = MeasureText(context.m_text.c_str(), 20) - 900;
	if (sizeUnits < 0)
	{
		defaultMainTextColor.DrawText(context.m_text.c_str(), 200, y + 2, 20);
	}
	else
	{
		// truncate
		std::string truncText = context.m_text.substr(0, context.m_text.length() - sizeUnits/ MeasureText("A", 20) - 1); // 1 padding
		defaultMainTextColor.DrawText(truncText + "...", 200, y + 2, 20);
	}
	defaultMainTextColor.DrawText(context.m_timestamp.c_str(), 1200, y + 2, 20);
}

void drawFadeBackgroundLowerBox()
{
	DrawRectangle(0, screenHeight - menuPanelHeight - chatPanelHeight, screenWidth, chatPanelHeight - 25, Fade(GetColor(0xAF1ACBF), 0.11f));
	DrawRectangle(0, screenHeight - menuPanelHeight, screenWidth, menuPanelHeight, Fade(GetColor(0xA2B6C4), 0.06f));
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
bool windowIsKeyPressedBackSpace()
{
	return windowIsKeyPressed(KEY_BACKSPACE);
}
bool windowIsKeyOnlyPressed(int keyID)
{
	return windowIsKeyPressed(keyID);
}
bool windowIsKeyReleasedEnter()
{
	return  windowIsKeyReleased(KEY_ENTER);
}
bool windowIsKeyPressed(int key)
{
	return IsKeyDown(key) || IsKeyPressed(key);
}
bool windowIsKeyReleased(int key)
{
	return IsKeyReleased(key);
}

bool windowIsMouseButtonPressed()
{
	return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

int windowGetCharPressed()
{
	return GetCharPressed();
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

bool windowIsMouseCollidesChatBox()
{
	return CheckCollisionPointRec(GetMousePosition(), chatSendBoxRect);
}

bool windowIsMouseCollidesChatSendButton()
{
	return CheckCollisionPointRec(GetMousePosition(), chatSendButtonRect);
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

void windowSetMouseCursorIBeam()
{
	SetMouseCursor(MOUSE_CURSOR_IBEAM);
}

void windowSetMouseCursorDefault()
{
	SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

bool windowIsPlayerCollidesRacePortal(int xPos, int yPos)
{
	return CheckCollisionRecs({ static_cast<float>(xPos), static_cast<float>(yPos), static_cast<float>(getCarWidth()), static_cast<float>(getCarHeight()) }, racePortalRect);
}
