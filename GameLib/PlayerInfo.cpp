#include "PlayerInfo.h"
#include <random>

PlayerInfo::PlayerInfo(std::string _name, sf::Vector2i _position, int _playerIndex)
{
	name = _name;
	position = _position;
	playerIndex = _playerIndex;
}

PlayerInfo::~PlayerInfo()
{
}
