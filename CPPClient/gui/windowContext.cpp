#include "windowContext.hpp"
#include "raylib-cpp.hpp"
static constexpr int screenWidth = 1000;
static constexpr int screenHeight = 450;
static const raylib::Color defaultMainTextColor = raylib::Color::LightGray();
// the one and only window
raylib::Window window(screenWidth, screenHeight, "raylib [core] example - basic window");
	
bool windowShouldCloseWrapper()
{
	return !window.ShouldClose();
}

void drawTextTestBox(std::string testStr)
{
	BeginDrawing();
	{
		window.ClearBackground(RAYWHITE);
		defaultMainTextColor.DrawText("Congrats! " + testStr, 190, 200, 20);
	}
	EndDrawing();
}