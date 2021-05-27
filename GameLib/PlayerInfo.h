#pragma once
#include <SFML\Graphics.hpp>
#include <iostream>


class PlayerInfo
{

public:
	const int numCards = 21;

	std::string name;
	sf::Vector2i position;
	int playerIndex;
	int movementLeft = 0;

	int numPlayers;
	std::vector<PlayerInfo> otherPlayers;

	PlayerInfo() {};
	PlayerInfo(std::string, sf::Vector2i, int);
	~PlayerInfo();
};