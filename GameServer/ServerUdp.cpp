#pragma once
#include <SFML\Network.hpp>
#include <iostream>
#include <list>
#include <thread>         
#include <mutex>  
#include "ServerUdp.h"

ServerUdp::ServerUdp()
{
	// Create a socket and bind it to the port 50000
	socket = new sf::UdpSocket();
	status = socket->bind(50000);
	
	if (status != sf::Socket::Done)
	{
		std::cout << "Error al conectar con el puerto" << std::endl;
		return;
	}

	commandsThread = std::thread(&ServerUdp::updatePositions, this);
	commandsThread.detach();

	//========================NEW===============================
	//Paquetes criticos

	paquetesCriticos = std::thread(&ServerUdp::TratamientoPaquetesCriticos, this);
	paquetesCriticos.detach();
	matchMakingThread = std::thread(&ServerUdp::MatchMaking, this);
	matchMakingThread.detach(); //Hay que hacer detach?
	AFKthread = std::thread(&ServerUdp::AFKController, this);
	paquete = 0;

	//==========================================================

	std::cout << "Esperando conexiones..." << std::endl;
	receive();
}
void ServerUdp::AFKController()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		//auto ot = clients.begin();
		for (auto it = clients.begin(); it != clients.end(); it++)
		{
			(*it).second->clientInfo->AFK_Timer++;
			if ((*it).second->clientInfo->AFK_Timer == 5)
			{
				(*it).second->clientInfo->afk = true;
				clients.erase((*it).second->peerAdress->port);
				std::cout << "Jugador desconectado por inactividad" << std::endl;
				//ot = it;
				break;
			}

		}
		/*if ((*ot).second->clientInfo->afk)
		{
			clients.erase((*ot).second->peerAdress->port);
			std::cout << "Jugador desconectado por inactividad" << std::endl;
		}*/

	}
	
}
void ServerUdp::TratamientoPaquetesCriticos()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		//Recorrer lista
		if (mapMsgNonAck.size() > 0) {
			for (auto it = mapMsgNonAck.begin(); it != mapMsgNonAck.end(); it++)
			{
				//Enviar que el paquete ha llegado
				//send((*it).second, (*it).first->ip, (*it).first->port, socket);
				send((*it).second->paquete, (*it).second->peer->ip, (*it).second->peer->port, socket);


				//Delay (Para que no este enviando mensajes muy seguidos
			}
		}
	}
}

void ServerUdp::send(sf::Packet packet, sf::IpAddress ipAdress, unsigned short port, sf::UdpSocket* socket)
{
	status = socket->send(packet, ipAdress, port);
	if (status != sf::Socket::Done)
	{
		std::cout << "Error al enviar\n";
	}
}

void ServerUdp::receive()
{
	while (true)
	{
		sf::Packet packet;
		sf::IpAddress senderIP;
		unsigned short senderPort;

		socket->receive(packet, senderIP, senderPort);
		PeerAdress* peerAdress = new PeerAdress(senderIP, senderPort/*, paquete*/);
		//paquete++;

		int head;
		packet >> head;

		//Compobar siempre que la salt sea correcta
		if (head != Head::HELLO)
		{
			fp_bit_twiddler saltAux;
			packet >> saltAux.d;

			bool exist = false;
			bool wrongSalt = false;

			//Buscar si existe el peer adress en nuestro map de clientes NO VALIDADOS
			for(auto it = waitingClients.begin(); it != waitingClients.end(); ++it)
			{
				//Tambien se tendria que comprobar la IP 
				if ((*it).first->port == peerAdress->port)
				{
					if ((*it).first->salt.d != saltAux.d)
					{
						//disconnect client
						wrongSalt = true;
						break;
					}
					//Salt OK!
					peerAdress->salt = (*it).first->salt;
					exist = true;
				}
			}

			if (!exist) {
				//Buscar si existe el peer adress en nuestro map de clientes VALIDADOS
				for (auto it = clients.begin(); it != clients.end(); ++it)
				{
					if ((*it).first == peerAdress->port)
					{
						if ((*it).second->clientInfo->salt.d != saltAux.d)
						{
							//disconnect client
							wrongSalt = true;
							break;
						}
						//Salt OK!
						peerAdress->salt = (*it).second->clientInfo->salt;
						exist = true;
						break;
					}
				}
			}
			if (wrongSalt)
			{
				std::cout << "La salt no es correcta! :(" << std::endl;

			}

			else if (!exist && !wrongSalt)
			{
				//disconnect client
				std::cout << "El cliente no ha passado la fase de hello y challenge y no esta en nuestro map de clients" << std::endl;			
			}
		}
		switch (head)
		{
		case (int)Head::HELLO:
		{				
			bool existNonValidated = false;
			for (auto it = waitingClients.begin(); it != waitingClients.end(); it++)
			{
				if ((*it).first->port == peerAdress->port && (*it).first->ip == peerAdress->ip) 
				{
					existNonValidated = true;
				}
			}

			if (!existNonValidated) {

				bool existValidated = false;
				for (auto it = clients.begin(); it != clients.end(); it++)
				{
					if ((*it).second->peerAdress->port == peerAdress->port && (*it).second->peerAdress->ip == peerAdress->ip)
					{
						existValidated = true;
					}
				}

				if (existValidated)
				{
					int id = clients.size();
					float x = rand() % 40;
					float y = rand() % 30;

					sf::Packet p;
					p << Head::WELCOME;
					p << peerAdress->salt.d;
					p << id;
					p << x;
					p << y;
					send(p, senderIP, senderPort, socket);
					break;
				}
			}

			std::cout << "He recibido HELLO! :D" << std::endl;
			packet >> peerAdress->salt.d;

			fp_bit_twiddler serverSalt;
			serverSalt.d = GetRandomFloat();

			float random = rand()% 100;

			if(!existNonValidated)
				waitingClients.push_back(std::make_pair(peerAdress,random));

			sf::Packet p;
			p << Head::CHALLENGE;	
			p << random;

			fp_bit_twiddler result;
			result.i[0] = serverSalt.i[0] & peerAdress->salt.i[0];
			result.i[1] = serverSalt.i[1] & peerAdress->salt.i[1];

			p << serverSalt.d;
			p << peerAdress->salt.d;
			send(p, senderIP, senderPort, socket);

			peerAdress->salt = result;

			break;
		}
		case (int)Head::CHALLENGE:
		{
			float answer;
			packet >> answer;

			std::string name;
			packet >> name;

			for (auto it = waitingClients.begin(); it != waitingClients.end(); it++)
			{
				bool clientFound = false;
				if ((*it).first->port == peerAdress->port)
				{
					float _answer = (*it).second * 3.1416;
					if (_answer != answer)
					{
						std::cout << "Wrong challenge" << std::endl;
						return;
					}

					//Borramos del waiting client para ponmerlo en el map de clients confirmados
					waitingClients.erase(it);

					int id = clients.size();
					float x = rand() % 40;
					float y = rand() % 30;

					sf::Packet p;
					p << Head::WELCOME;
					p << peerAdress->salt.d;
					p << id;
					p << x;
					p << y;
					send(p, senderIP, senderPort, socket);

					ClientInfo* client = new ClientInfo();
					client->id = id;
					client->pos.x = x;
					client->pos.y = y;
					client->salt = peerAdress->salt;
					client->name = name;

					clients[peerAdress->port] = new AllClientInfo(client, peerAdress);
					//matchMakingPlayers[peerAdress->port] = new AllClientInfo(client, peerAdress);
					clientFound = true;
				}

				//Este break esta hecho para que salga una vez encuentre el cliente que quiere no pete el it
				if (clientFound)
					break;
			}
			break;
		}
		case (int)Head::PLAY:
		{
			if (clients[peerAdress->port]->clientInfo->inMatchmaking)
				break;

			clients[peerAdress->port]->clientInfo->inMatchmaking = true;
			break;
		}
		case (int)Head::END_MATCHMAKE:
		{
			clients[peerAdress->port]->clientInfo->inMatchmaking = false;
			break;
		}case (int)Head::PING:
		{
			clients[peerAdress->port]->clientInfo->AFK_Timer = 0;
			clients[peerAdress->port]->clientInfo->afk = false;
			std::cout << "PING" << std::endl;
			break;
		}
		case (int)Head::MOVE:
		{
			int id, counter;
			float x, y;
			packet >> id >> counter >> x >> y;
			//encolar comando a lista de comandos
			/*std::cout << "Se movio:" << std::endl;*/
			std::cout << "x " << x << "   y " << y << std::endl;

			movements.push_back(new Movement(id, counter, sf::Vector2f(x, y), peerAdress->port, peerAdress->ip));
			break;
		}
		case (int)Head::ACK:
		{
			//Sacamos el id del paquete ACK
			//De momento no lo hacemos, se tendria que enviar el id del mns des del client
			/*int id;
			packet >> id;*/

			for (auto it = mapMsgNonAck.begin(); it != mapMsgNonAck.end(); it++)
			{

				if ((*it).second->peer->port == peerAdress->port) {

					std::cout << "Borro el paquete de la lista" << std::endl;
					//Borrar del map el paquete confirmado
					mapMsgNonAck.erase(it);
					break;

				}
				//if ((*it).first->port == peerAdress->port)
				//{
				//
				//	//Pot ser que un mateix PeerAdress tingui varios paquets en la llista de critics??
				//	/*int aux;
				//	(*it).second >> aux;
				//	if (aux = _head)
				//	{

				//	}*/
				//	//Comprueba si es el paquete que toca
				//	//Borrar de la lista el paquete confirmado
				//	if ((*it).first->localPacketID == peerAdress->localPacketID) 
				//	{
				//		listMsgNonAck.erase(it);
				//	}
				//	
				//	break;
				//}
			}
			break;
		}
		case (int)Head::DISCONNECTION:
		{
			//clients[]
			clients.erase(peerAdress->port);
			std::cout << "Un jugador se ha desconectado" << std::endl;

			break;
		}
		default:
			break;
		}		
	}	
}

void ServerUdp::MatchMaking()
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(300));

		if (clients.size() > 0) {
			//MatchMaking
			for (auto it = clients.begin(); it != clients.end(); it++)
			{
				if ((*it).second->clientInfo->inMatchmaking)
				{
					for (auto _it = clients.begin(); _it != clients.end(); _it++)
					{
						if (it != _it)
						{
							if ((*_it).second->clientInfo->inMatchmaking )
							{
								if (DistanceFuncion((*it).second->clientInfo->name, (*_it).second->clientInfo->name))
								{
									//He juntat 2 jugadors

									//Player 1 join
									sf::Packet packet;
									packet << Head::PLAYER_JOINED;
									packet << (*it).second->clientInfo->salt.d;
									//Passar-li en el packet la client info que necessitin
									packet << (*_it).second->clientInfo->name << (*_it).second->clientInfo->id << (*_it).second->clientInfo->pos.x << (*_it).second->clientInfo->pos.y;


									//Se envia como paquete critico
									mapMsgNonAck[(*it).second->clientInfo->id] = new PacketCritics((*it).second->peerAdress, packet);
									//send(packet, (*it).second->peerAdress->ip, (*it).second->peerAdress->port, socket);
									(*it).second->clientInfo->inMatchmaking = false;
									(*it).second->clientInfo->inLobby = true;


									//Player 2 join
									sf::Packet packet2;
									packet2 << Head::PLAYER_JOINED;
									packet2 << (*_it).second->clientInfo->salt.d;
									packet2 << (*it).second->clientInfo->name << (*it).second->clientInfo->id << (*it).second->clientInfo->pos.x << (*it).second->clientInfo->pos.y;
									//Passar-li en el packet la client info que necessitin


									//Se envia como paquete critico
									mapMsgNonAck[(*_it).second->clientInfo->id] = new PacketCritics((*_it).second->peerAdress, packet2);
									//send(packet2, (*_it).second->peerAdress->ip, (*_it).second->peerAdress->port, socket);
									(*_it).second->clientInfo->inMatchmaking = false;
									(*_it).second->clientInfo->inLobby = true;

									std::thread threadLobbyTimer = std::thread(&ServerUdp::LobbyTimer, this, (*_it).second, (*it).second);
									threadLobbyTimer.detach();

								}
							}
						}
					}
				}
			}
		}
	}
}

void ServerUdp::LobbyTimer(AllClientInfo* c1, AllClientInfo* c2)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(30000));
	c1->clientInfo->inLobby = false;
	c2->clientInfo->inLobby = false;

	sf::Packet p;
	p << Head::END_LOBBY;
	p << c1->clientInfo->salt.d;
	send(p, c1->peerAdress->ip, c1->peerAdress->port, socket);

	p.clear();

	p << Head::END_LOBBY;
	p << c2->clientInfo->salt.d;
	send(p, c2->peerAdress->ip, c2->peerAdress->port, socket);
}

void ServerUdp::updatePositions()
{
	bool badMove;
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		joinCommands();
		
		for (auto it = movementIDOrder.begin(); it != movementIDOrder.end(); ++it)
		{
			badMove = false;
			//Controlar limitacions de pantalla
			if (it->second->position.x < 0)
			{
				badMove = true;
			//	it->second->position.x = 0;
				it->second->position.x = 0;

			}

			else if (it->second->position.x > 39)
			{
			//	it->second->position.x = 25;
				it->second->position.x = 39;

				badMove = true;
			}

			else if (it->second->position.y < 0)
			{
				badMove = true;
				//it->second->position.y = 0;
				it->second->position.y = 0;

			}

			else if (it->second->position.y > 29)
			{
				badMove = true;
				//it->second->position.y = 17;
				it->second->position.y = 29;

			}
			
			if (clients[(*it).second->port] != nullptr && badMove)
			{
				clients[(*it).second->port]->clientInfo->UpdatePos((*it).second->position.x, (*it).second->position.y);

				sf::Packet p;

				p << Head::MOVE;
				p << clients[(*it).second->port]->clientInfo->salt.d;
				p << badMove;
				p << (*it).second->id;
				p << (*it).second->position.x;
				p << (*it).second->position.y;

				//nomes enviant correccio al client que corretgeix hauria de ser suficient
				send(p, (*it).second->ip, (*it).second->port, socket);
				/*for (auto _it = clients.begin(); _it != clients.end(); ++_it)
				{
				}*/
			}
			else if (clients[(*it).second->port] != nullptr && !badMove)
			{
				sf::Packet p;

				for (auto it2 = clients.begin(); it2 != clients.end(); it2++)
				{
					if ((*it2).second->clientInfo->id != (*it).second->id)
					{
						p << Head::MOVE;
						p << (*it2).second->clientInfo->salt.d;
						p << badMove;
						p << (*it).second->id;
						p << (*it).second->position.x;
						p << (*it).second->position.y;

						send(p, (*it2).second->peerAdress->ip, (*it2).second->peerAdress->port, socket);
						p.clear();
					}
				}
			}
		}

		if (movementIDOrder.size() > 0)
			movementIDOrder.clear();
	}
}

void ServerUdp::joinCommands()
{
	for (auto it = movements.begin(); it != movements.end(); ++it)
	{
		movementIDOrder.push_back(std::make_pair((*it)->id, *it));
		
		for (auto _it = movementIDOrder.begin(); _it != movementIDOrder.end(); ++_it)
		{
			if ((*it)->id == (*_it).first)
			{
				if ((*it)->moveNum <= (*_it).second->moveNum) {

					break;
				}
				else
				{
					(*_it).second = *it;
				}
			}
			else
			{
				movementIDOrder.push_back(std::make_pair((*it)->id, *it));
			}
		}
	}

	movements.clear();
}

float ServerUdp::GetRandomFloat()
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	static std::uniform_real_distribution<float> dis(0.f, 1.f);
	return dis(gen);
}

bool ServerUdp::DistanceFuncion(std::string n1, std::string n2)
{
	//return true;
	int result = abs(n1[0] - n2[0]);

	if (result > 2)
		return false;
	
	return true;
}

ServerUdp::~ServerUdp()
{
	socket->unbind();
	delete socket;
}