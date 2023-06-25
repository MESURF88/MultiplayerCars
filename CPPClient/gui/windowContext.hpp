#ifndef _WINDOWCONTEXT_
#define _WINDOWCONTEXT_
#include <string>
#include <map>

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
void drawDefaultSquaresColor();
void drawEscButton();
void windowCloseWindow();
bool windowIsKeyPressedUp();
bool windowIsKeyPressedDown();
bool windowIsKeyPressedLeft();
bool windowIsKeyPressedRight();
bool windowIsKeyPressed(int key);
bool windowIsMouseButtonPressed();
std::map<int, ColorHexMap> windowGetColorSelectionMap();
int windowIsMouseInColorSelection();
bool windowIsMouseInEscape();


#endif // _WINDOWCONTEXT_