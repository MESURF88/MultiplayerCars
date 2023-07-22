#ifndef _WINDOWCONTEXT_
#define _WINDOWCONTEXT_
#include "raylib-cpp.hpp" 
#include <string>
#include <map>

class TextContext
{
public:
	TextContext() {
		m_leftSide = false;
		m_colorStr = "ffffff";
		m_text = "";
		m_timestamp = "";
	}
	TextContext(bool leftSide, std::string colorStr, std::string text, std::string timestamp)
	{
		m_leftSide = leftSide;
		m_colorStr = colorStr;
		m_text = text;
		m_timestamp = timestamp;
	}
	bool m_leftSide;
	std::string m_colorStr;
	std::string m_text;
	std::string m_timestamp;
};

class ColorHexMap
{
public:
	ColorHexMap()
	{
		hexValue = 0;
		hexString = "000000";
	}
	ColorHexMap(const int& hexV, const std::string& hexS)
	{
		hexValue = hexV;
		hexString = hexS;
	}
	int hexValue;
	std::string hexString;
};

enum colorSelectionType
{
	NOCOLOR = 0,
	BLUECOLOR,
	GREENCOLOR,
	REDCOLOR,
	MAGENTACOLOR,
	ORANGECOLOR,
	YELLOWCOLOR,
	SKYBLUECOLOR,
	LIGHTGREYCOLOR,
	MAXCOLORSELECTION,
};

std::string colorHexToString(int hexValue);
int colorHexToString(std::string hexString);
int windowScreenWidth();
int windowScreenHeight();
int windowYBoundary();
void windowSetTargetFPS(int fps);
bool windowShouldCloseWrapper();
void windowBeginDrawing(); // bookend any drawing with windowBeginDrawing and windowEndDrawing
void windowEndDrawing();
void windowDrawBackground();
void drawTextTestBox(std::string testStr);
void drawChatBoxContainer();
void drawChatSendBox(bool mouseOnText, const char* text);
void drawPortalRaceInfoPane(bool playerInRacePortal);
void drawSendTextButton();
void drawPortalRectangles(int xPos, int yPos);
void drawChatSendBoxBlinkingUnderscore(const int& framesCounter, const char* tex);
void drawDefaultSquaresColor();
void drawTextLine(int idx, const TextContext& context);
void drawEscButton();
void windowCloseWindow();
bool windowIsKeyPressedUp();
bool windowIsKeyPressedDown();
bool windowIsKeyPressedLeft();
bool windowIsKeyPressedRight();
bool windowIsKeyPressedBackSpace();
bool windowIsKeyOnlyPressed(int keyID);
bool windowIsKeyReleasedEnter();
bool windowIsKeyPressed(int key);
bool windowIsKeyReleased(int key);
bool windowIsMouseButtonPressed();
int windowGetCharPressed();
std::map<int, ColorHexMap> windowGetColorSelectionMap();
int windowIsMouseInColorSelection();
bool windowIsMouseInEscape();
bool windowIsMouseCollidesChatBox();
bool windowIsMouseCollidesChatSendButton();
void windowSetMouseCursorIBeam();
void windowSetMouseCursorDefault();
bool windowIsPlayerCollidesRacePortal(int xPos, int yPos);


#endif // _WINDOWCONTEXT_