#include "windowContext.hpp"
#include "raylib-cpp.hpp" // raylib has to be included in cpp file to avoid name conflicts with windows.h
static constexpr int screenWidth = 1600;
static constexpr int screenHeight = 850;
static const raylib::Color defaultMainTextColor = raylib::Color::LightGray();
// the one and only window
raylib::Window window(screenWidth, screenHeight, "raylib [core] example - basic window");
	
void windowSetTargetFPS(int fps)
{
	SetTargetFPS(fps);
}

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