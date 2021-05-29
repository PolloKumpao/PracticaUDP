#pragma once
#include "ClientUdp.h"

ClientUdp::ClientUdp()
{
	socket = new sf::UdpSocket();
	clientInfo = new ClientInfo();
	//socket->bind(50000);


	clientInfo->salt.d = GetRandomFloat();
	//head = Head::HELLO;

	//sf::Packet p;
	//p << Head::HELLO << clientInfo->salt.d;

	accumX = 0;
	accumY = 0;

	contadorMns = 0;

	//send(p);

	receiveThread = std::thread(&ClientUdp::receive, this);
	receiveThread.detach();	
}

void ClientUdp::send(sf::Packet packet)
{
	status = socket->send(packet, sf::IpAddress::getLocalAddress().toString(), 50000);
	if (status != sf::Socket::Done)
	{
		std::cout << "Error al enviar\n";
	}
	//std::cout << std::endl;
}

void ClientUdp::sendPING()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		sf::Packet ping;
		ping << Head::PING << clientInfo->salt.d;
		//std::cout << "PING" << std::endl;
		send(ping);

	}
}

void ClientUdp::receive()
{

	bool sendingHello = true;

	while (true)
	{
		sf::Packet packet;
		sf::IpAddress senderIP;
		unsigned short senderPort;

		//Mientras server no envie CHALLENGE sendingHello sera true
		if (sendingHello)
		{
			sf::Packet p;
			p << Head::HELLO << clientInfo->salt.d;

			send(p);
		}

		socket->receive(packet, senderIP, senderPort);
		packet >> head;


		if (head != Head::CHALLENGE)
		{
		
			fp_bit_twiddler saltAux;
			packet >> saltAux.d;
			if (saltAux.d != clientInfo->salt.d)
			{
				//disconnect
				std::cout << "Wrong salt!" << std::endl;
			}
		}

		switch (head)
		{
			case (int)Head::CHALLENGE:
			{
				//Si ya has recibido el welcome el running estarà a true, por lo tanto no hacemos el challenge
				if (running)
					break;

				sendingHello = false;

				float question;
				packet >> question;

				fp_bit_twiddler serverSalt;
				packet >> serverSalt.d;
				fp_bit_twiddler clientSalt;
				packet >> clientSalt.d;

				if (clientInfo->salt.d == clientSalt.d)
				{
					fp_bit_twiddler result;
					result.i[0] = serverSalt.i[0] & clientSalt.i[0];
					result.i[1] = serverSalt.i[1] & clientSalt.i[1];

					clientInfo->salt = result;

					float answer = question * 3.1416;

					std::cout << "Como te llamas?" << std::endl;
					std::cin >> clientInfo->name;
					

					sf::Packet _packet;
					_packet << Head::CHALLENGE;
					_packet << result.d;
					_packet << answer;
					_packet << clientInfo->name;

					send(_packet);
				}


				break;
			}
			case (int)Head::WELCOME:
			{
				std::cout << "Welcome to the server" << std::endl;

				packet >> clientInfo->id >> clientInfo->pos.x >> clientInfo->pos.y;

				/*int size = clientInfo->id;

				for (int i = 0; i < size; i++)
				{
					int id, x, y;
					packet >> id >> x >> y;
					otherClients.push_back(new ClientInfo(id, sf::Vector2i(x, y)));
				}
				*/
				commandsThread = std::thread(&ClientUdp::sendCommands, this);
				commandsThread.detach();
				std::thread PINGthread = std::thread(&ClientUdp::sendPING, this);
				PINGthread.detach();

				running = true;
				break;
			}
			case (int)Head::MOVE:
			{
				float x, y;
				int id;
				packet >> badMove >> id >> x >> y;
				
				if (id == clientInfo->id && badMove)
				{
					//S'ha de comprovar si el moviment que ha fet no està permès i corregir-lo
					realX = x;
					realY = y;					
				}
				
				if (clientInfo->otherPlayer != nullptr)
				{
					for (auto it = clientInfo->otherPlayers.begin(); it != clientInfo->otherPlayers.end(); it++)
					{
						if (id == (*it).second->id && badMove)
						{
							(*it).second->UpdatePos(x, y);
						}

						else if (id == (*it).second->id && !badMove)
						{
							//falta interpolar entre x i y de otherPlayer amb x i y rebudes
							//clientInfo->otherPlayer->finalPos = sf::Vector2f(x, y);
							(*it).second->UpdatePos(x, y);
						}
					}
					//if (id == clientInfo->otherPlayer->id && badMove)
					//{
					//	clientInfo->otherPlayer->UpdatePos(x, y);
					//}

					//else if (id == clientInfo->otherPlayer->id && !badMove)
					//{
					//	//falta interpolar entre x i y de otherPlayer amb x i y rebudes
					//	//clientInfo->otherPlayer->finalPos = sf::Vector2f(x, y);
					//	clientInfo->otherPlayer->UpdatePos(x, y);
					//}
				}
				break;
			}
			case (int)Head::PLAYER_JOINED:
			{
				//Este paquete es critico
				std::cout << "Player found" << std::endl;
				int id;
				packet >> id;
				//Falta poner a 0 esta variable cuando acabes la partida con el otro player
				std::cout << "Recibido paquete critico con ID: " << id << std::endl;
				contadorMns++;


				//if (contadorMns == 1) 
				//{
					//Constructor de el otro player
					clientInfo->otherPlayer = new ClientInfo();
					packet >> clientInfo->otherPlayer->name;
					packet >> clientInfo->otherPlayer->id;
					packet >> clientInfo->otherPlayer->pos.x;
					packet >> clientInfo->otherPlayer->pos.y;

					clientInfo->otherPlayers.insert(std::pair<int, ClientInfo*>(clientInfo->otherPlayer->id, clientInfo->otherPlayer));

					clientInfo->newPlayerArrived = true;
					clientInfo->inMatchmake = false;
					clientInfo->inLobby = true;

					//std::thread otherPlayerMoves = std::thread(&ClientUdp::lerpPos, this);
					//otherPlayerMoves.detach();

					//Jugar vs el otro
				//}

				sf::Packet _packet;
				_packet << Head::ACK ;
				_packet << clientInfo->salt.d 
						<< id;
				send(_packet);
				break;
			}
			case (int)Head::END_LOBBY:
			{
				clientInfo->inLobby = false;
				contadorMns = 0;
				//Delete the player
				break;
			}
		}

	}
}

void ClientUdp::sendCommands()
{
	int counter = 0;
	
	while (true)
	{
		//Clear del paquet
		accumPacket.clear();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (playerMoved)
		{
		//	std::cout << "El player se movio weon\n";
			accumPacket << Head::MOVE << clientInfo->salt.d << clientInfo->id << counter << clientInfo->pos.x << clientInfo->pos.y;

			send(accumPacket);

			counter++;

			playerMoved = false;
		}
		
	}
}
float ClientUdp::GetRandomFloat()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}

//thread para interpolar la posicion del otro player
void ClientUdp::lerpPos()
{
	while (clientInfo->inLobby)
	{
		if (clientInfo->finalPos != clientInfo->pos)
		{
			//clientInfo->finalPos
			clientInfo->pos.x = lerp(clientInfo->pos.x, clientInfo->finalPos.x, 0.3f);
			clientInfo->pos.y = lerp(clientInfo->pos.y, clientInfo->finalPos.y, 0.3f);

		}
	}
}


ClientUdp::~ClientUdp()
{
	socket->unbind();
	delete socket;
	delete clientInfo;
}