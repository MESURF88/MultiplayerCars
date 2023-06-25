#ifndef _CARCLASS_
#define _CARCLASS_
#include <string>

int getCarWidth();
int getCarHeight();
void setCarColor(int colorHexValue);
int getCarColorHexValue();
std::string getCarColorString();
void drawCar(int X, int Y);
void drawCar(int X, int Y, int colorHexValue);

#endif // _CARCLASS_