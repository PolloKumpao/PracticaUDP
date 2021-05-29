#pragma once
#include <SFML\Network.hpp>
#include <iostream>
#include <vector>
#include <thread>         
#include <mutex>  
#include <random>
#include <unordered_map>
#include <cmath>

union fp_bit_twiddler {
	double d;
	int i[2];
};

class ClientInfo
{
public:

	int id;
	std::string name;
	fp_bit_twiddler salt;
	sf::Vector2f pos;
	sf::Vector2f finalPos;

	bool inMatchmake = false;
	bool inLobby;
	
	bool newPlayerArrived;
	std::map <int, ClientInfo*>otherPlayers;
	ClientInfo* otherPlayer;

	//Other info
	ClientInfo()
	{
		newPlayerArrived = false;
		inLobby = false;
	}

	ClientInfo(int _id, sf::Vector2f _pos)
	{
		id = _id;
		pos = _pos;
	}

	void UpdatePos(float x, float y)
	{
		pos.x = x;
		pos.y = y;
	}
};

struct PeerAdress
{
	sf::IpAddress ip;
	unsigned short port;

	//======================== MARC ===================================

	//Paquetes criticos
	int localPacketID;

	//==========================================================

	PeerAdress(sf::IpAddress _ip, unsigned short _port)
	{
		ip = _ip;
		port = _port;
	}
};

class ClientUdp
{
public:
	sf::UdpSocket* socket;
	ClientInfo* clientInfo;

	std::vector<ClientInfo*> otherClients;

	sf::Socket::Status status;

	sf::Packet accumPacket;

	int accumX;
	int accumY;
	int realX, realY;


	int contadorMns;

	std::thread receiveThread;
	std::thread commandsThread;
	std::thread lerpThread;

	bool playerMoved;
	bool badMove;
	bool running;

	//std::mutex mtx;

	int head;
	enum Head {
		MOVE, HELLO, WELCOME, CHALLENGE, PLAY, PLAYER_JOINED, END_MATCHMAKE, ACK, DISCONNECTION, END_LOBBY, PING
	};

	ClientUdp();
	void send(sf::Packet packet);
	void sendCommands();
	void sendPING();
	void lerpPos();
	void receive();
	static float GetRandomFloat();
	~ClientUdp();
};