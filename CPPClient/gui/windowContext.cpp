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
static constexpr int racePortalBoxHeight = 132;
static constexpr int racePortalBoxWidth = 286;
static constexpr int racePortalX = screenWidth - racePortalBoxWidth - 112;
static constexpr int racePortalY = 286;
static constexpr int futurePortalX = racePortalX - racePortalBoxWidth - 72;

static const raylib::Color defaultMainTextColor = raylib::Color::Gray();
// the one and only window
raylib::Window window(screenWidth, screenHeight, std::string("car sim game alpha v") + VERSION_LABEL);

static const Rectangle chatSendBoxRect = { 0, screenHeight - menuPanelHeight - chatSendBoxHeight, chatSendBoxWidth, chatSendBoxHeight };
static const Rectangle chatSendButtonRect = { chatSendBoxWidth-1, screenHeight - menuPanelHeight - chatSendBoxHeight, 100, chatSendBoxHeight };
static const Rectangle racePortalRect = { racePortalX, racePortalY, racePortalBoxWidth, racePortalBoxHeight };
static const Rectangle futurePortalRect = { futurePortalX, racePortalY, racePortalBoxWidth, racePortalBoxHeight };

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

static void drawStreetLamp(int x, int baseY)
{
	DrawRectangle(x, baseY - 142, 6, 142, GetColor(0x27323AFF));
	DrawCircle(x + 3, baseY - 146, 18, GetColor(0x38444CFF));
	DrawCircle(x + 3, baseY - 146, 11, GetColor(0xFFD66BFF));
	DrawCircle(x + 3, baseY - 146, 28, Fade(GetColor(0xFFD66BFF), 0.16f));
}

static void drawLobbyBuilding(int x, int y, int width, int height, Color facade, Color awning)
{
	DrawRectangle(x, y, width, height, facade);
	DrawRectangle(x, y, width, 8, Fade(BLACK, 0.12f));
	DrawRectangle(x + 12, y + 34, width - 24, 44, GetColor(0xD7F0FFFF));
	DrawRectangleLines(x + 12, y + 34, width - 24, 44, Fade(BLACK, 0.28f));
	DrawRectangle(x, y + 86, width, 18, awning);
	for (int stripeX = x + 10; stripeX < x + width - 8; stripeX += 36)
	{
		DrawRectangle(stripeX, y + 86, 18, 18, Fade(RAYWHITE, 0.72f));
	}
	DrawRectangle(x + 24, y + height - 54, width - 48, 54, GetColor(0x2E363CFF));
	DrawRectangle(x + 34, y + height - 44, width - 68, 34, GetColor(0x8ECFE8FF));
}

static void drawLobbyBillboard()
{
	const Rectangle billboard = { 34.0f, 18.0f, 620.0f, 90.0f };
	DrawRectangle(72, 104, 10, 44, GetColor(0x56616AFF));
	DrawRectangle(604, 104, 10, 44, GetColor(0x56616AFF));
	DrawRectangleRounded({ billboard.x + 8.0f, billboard.y + 8.0f, billboard.width, billboard.height }, 0.08f, 8, Fade(BLACK, 0.18f));
	DrawRectangleRounded(billboard, 0.08f, 8, RAYWHITE);
	DrawRectangleRoundedLinesEx(billboard, 0.08f, 8, 3.0f, GetColor(0x25313AFF));
	DrawRectangle(static_cast<int>(billboard.x) + 18, static_cast<int>(billboard.y) + 18, 8, 54, GetColor(0x2E363CFF));
	DrawText("DOWNTOWN LOBBY", static_cast<int>(billboard.x) + 42, static_cast<int>(billboard.y) + 16, 30, GetColor(0x25313AFF));
	DrawText("Cruise to a course car and press E to enter", static_cast<int>(billboard.x) + 44, static_cast<int>(billboard.y) + 54, 20, GetColor(0x43515BFF));
}

static void drawCoursePortalCar(Rectangle portalRect, bool highlighted, bool enabled, const char* courseLabel, const char* statusText)
{
	const Color glowColor = highlighted ? GOLD : (enabled ? Fade(ORANGE, 0.58f) : Fade(LIGHTGRAY, 0.42f));
	const Color bodyColor = enabled ? GetColor(0xE62937FF) : GetColor(0x7F878EFF);
	const Color trimColor = enabled ? GetColor(0x11171DFF) : GetColor(0x3E464DFF);
	const Color labelColor = enabled ? RAYWHITE : GetColor(0xD3D8DDFF);
	const int carX = static_cast<int>(portalRect.x + 24);
	const int carY = static_cast<int>(portalRect.y + 36);
	const int carW = static_cast<int>(portalRect.width - 48);
	const int carH = 62;

	DrawRectangleRounded({ portalRect.x - 10, portalRect.y - 10, portalRect.width + 20, portalRect.height + 20 }, 0.18f, 12, Fade(glowColor, highlighted ? 0.24f : 0.10f));
	DrawRectangleRoundedLinesEx({ portalRect.x - 4, portalRect.y - 4, portalRect.width + 8, portalRect.height + 8 }, 0.18f, 12, enabled ? 3.0f : 2.0f, glowColor);

	DrawRectangleRounded({ portalRect.x + 12, portalRect.y + 14, portalRect.width - 24, portalRect.height - 20 }, 0.15f, 10, enabled ? GetColor(0x2A2F34FF) : GetColor(0x404850FF));
	DrawLineEx({ portalRect.x + 28, portalRect.y + portalRect.height - 20 }, { portalRect.x + portalRect.width - 28, portalRect.y + portalRect.height - 20 }, 3.0f, enabled ? GetColor(0xF8F3D8FF) : GetColor(0x9FA7AEFF));

	DrawRectangleRounded({ static_cast<float>(carX), static_cast<float>(carY + 18), static_cast<float>(carW), static_cast<float>(carH) }, 0.34f, 18, bodyColor);
	DrawTriangle({ static_cast<float>(carX + 62), static_cast<float>(carY + 24) }, { static_cast<float>(carX + 116), static_cast<float>(carY - 8) }, { static_cast<float>(carX + 178), static_cast<float>(carY + 24) }, bodyColor);
	DrawRectangleRounded({ static_cast<float>(carX + 92), static_cast<float>(carY + 4), 72.0f, 32.0f }, 0.22f, 8, enabled ? GetColor(0xBFEAFFFF) : GetColor(0xB9C0C6FF));
	DrawRectangle(carX + 148, carY + 40, 54, 10, enabled ? GetColor(0xFFD24AFF) : GetColor(0xC3C8CDFF));
	DrawRectangle(carX + 18, carY + 40, 40, 10, enabled ? GetColor(0xFFFFFFFF) : GetColor(0xD6DADEFF));
	DrawRectangleRounded({ static_cast<float>(carX + carW - 36), static_cast<float>(carY + 4), 48.0f, 10.0f }, 0.5f, 8, trimColor);
	DrawCircle(carX + 56, carY + carH + 16, 24, trimColor);
	DrawCircle(carX + carW - 54, carY + carH + 16, 24, trimColor);
	DrawCircle(carX + 56, carY + carH + 16, 11, GetColor(0xA8B2BAFF));
	DrawCircle(carX + carW - 54, carY + carH + 16, 11, GetColor(0xA8B2BAFF));

	const int labelFontSize = highlighted ? 26 : 22;
	const int labelX = carX + ((carW - MeasureText(courseLabel, labelFontSize)) / 2);
	DrawText(courseLabel, labelX, carY + carH + 38, labelFontSize, highlighted ? GOLD : labelColor);
	if (statusText != nullptr)
	{
		const int statusX = carX + ((carW - MeasureText(statusText, 15)) / 2);
		DrawText(statusText, statusX, carY + carH + 66, 15, enabled ? GetColor(0xE8EEF2FF) : GetColor(0xC1C7CCFF));
	}
}

static void drawParkedRacePortalCar(bool playerInRacePortal)
{
	drawCoursePortalCar(futurePortalRect, false, false, "Future Circuit", "Coming soon");
	drawCoursePortalCar(racePortalRect, playerInRacePortal, true, "Simple Circuit", playerInRacePortal ? "Press E to race" : "Open");
}

void drawLobbyStreetScene()
{
	const int playableBottom = windowYBoundary();
	const int streetTop = 142;
	const int sidewalkTop = streetTop - 36;
	const int curbY = streetTop + 8;
	const int laneCenterY = streetTop + 170;

	DrawRectangleGradientV(0, 0, screenWidth, streetTop, GetColor(0x9ED8FFFF), GetColor(0xEAF8FFFF));
	DrawCircle(135, 68, 42, GetColor(0xFFE078FF));
	DrawCircle(135, 68, 62, Fade(GetColor(0xFFE078FF), 0.18f));

	drawLobbyBuilding(70, 48, 175, 116, GetColor(0xD5DDE5FF), GetColor(0xE62937FF));
	drawLobbyBuilding(278, 30, 210, 134, GetColor(0xBFCBD2FF), GetColor(0x0079F1FF));
	drawLobbyBuilding(520, 58, 170, 106, GetColor(0xD8C9B8FF), GetColor(0xFFA100FF));
	drawLobbyBuilding(724, 42, 230, 122, GetColor(0xC4D6CDFF), GetColor(0x00A86BFF));
	drawLobbyBuilding(990, 54, 190, 110, GetColor(0xCACFD6FF), GetColor(0x7A5CFFFF));

	DrawRectangle(0, sidewalkTop, screenWidth, 58, GetColor(0xC8CED3FF));
	for (int x = 0; x < screenWidth; x += 96)
	{
		DrawLine(x, sidewalkTop, x + 36, streetTop + 22, Fade(WHITE, 0.32f));
		DrawLine(x, streetTop + 22, x + 84, streetTop + 22, Fade(BLACK, 0.12f));
	}
	DrawRectangle(0, curbY, screenWidth, 12, GetColor(0x8C969EFF));

	DrawRectangleGradientV(0, streetTop + 20, screenWidth, playableBottom - streetTop - 20, GetColor(0x303A42FF), GetColor(0x1F252BFF));
	for (int x = -40; x < screenWidth; x += 145)
	{
		DrawRectangleRounded({ static_cast<float>(x), static_cast<float>(laneCenterY), 74.0f, 8.0f }, 0.45f, 6, GetColor(0xF8F3D8FF));
	}
	for (int x = 0; x < screenWidth; x += 82)
	{
		DrawRectangle(x, playableBottom - 44, 44, 5, Fade(WHITE, 0.22f));
	}

	DrawRectangle(0, playableBottom - 36, screenWidth, 36, GetColor(0x4B555CFF));
	DrawLine(0, playableBottom - 36, screenWidth, playableBottom - 36, GetColor(0xBBC3C9FF));
	drawStreetLamp(1180, streetTop + 18);
	drawStreetLamp(1428, streetTop + 18);
	drawLobbyBillboard();
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
		const int promptX = static_cast<int>(racePortalRect.x + 34);
		const int promptY = static_cast<int>(racePortalRect.y - 78);
		DrawRectangleRounded({ static_cast<float>(promptX), static_cast<float>(promptY), 218.0f, 58.0f }, 0.18f, 8, Fade(GetColor(0x11171DFF), 0.86f));
		DrawRectangleRoundedLinesEx({ static_cast<float>(promptX), static_cast<float>(promptY), 218.0f, 58.0f }, 0.18f, 8, 2.0f, GOLD);
		DrawText("E", promptX + 24, promptY + 14, 30, GOLD);
		DrawText("Simple Circuit", promptX + 66, promptY + 18, 22, RAYWHITE);
	}
}

void drawSendTextButton()
{
	DrawRectangleLines((int)chatSendButtonRect.x, (int)chatSendButtonRect.y, (int)chatSendButtonRect.width, (int)chatSendButtonRect.height, BLACK);
	defaultMainTextColor.DrawText("SEND", chatSendButtonRect.x + 25, chatSendButtonRect.y + 5, 20);
}

void drawPortalRectangles(int xPos, int yPos)
{
	const bool playerInRacePortal = CheckCollisionRecs({ static_cast<float>(xPos), static_cast<float>(yPos), static_cast<float>(getCarWidth()), static_cast<float>(getCarHeight()) }, racePortalRect);
	drawParkedRacePortalCar(playerInRacePortal);
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
