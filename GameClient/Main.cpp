#pragma once
#include <iostream>
#include <SFML\Network.hpp>
#include "Graphics.h"

int main()
{
	Graphics* g = new Graphics();
	g->DrawDungeon();
	return 0;
}