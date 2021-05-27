#include "Graphics.h"
#include <iostream>


Graphics::Graphics()
{
	clientUdp = new ClientUdp();
}

void Graphics::DrawDungeon()
{
	while (!clientUdp->running);

	sf::RenderWindow _window(sf::VideoMode(800, 600), "Ventanita");
	sf::RectangleShape shape(sf::Vector2f(SIZE, SIZE));
	shape.setOutlineColor(sf::Color::Black);
	shape.setOutlineThickness(2.f);

	player = Sala(clientUdp->clientInfo->name, clientUdp->clientInfo->pos.x, clientUdp->clientInfo->pos.y, 1, 1, sf::Color(0, 150, 0, 255));

	while (_window.isOpen())
	{
		if (clientUdp->clientInfo->newPlayerArrived)
		{
			otherPlayer = Sala(clientUdp->clientInfo->otherPlayer->name, 
									clientUdp->clientInfo->otherPlayer->pos.x, 
									clientUdp->clientInfo->otherPlayer->pos.y, 
									1, 1, sf::Color(150, 0, 0, 255));

			clientUdp->clientInfo->newPlayerArrived = false;
		}

		//std::cout << "In Lobbo: " << clientUdp->clientInfo->inLobby << "  gogo" << std::endl;
		if (clientUdp->clientInfo->inLobby)
		{
			otherPlayer.origen.x = clientUdp->clientInfo->otherPlayer->pos.x;
			otherPlayer.origen.y = clientUdp->clientInfo->otherPlayer->pos.y;
		}

		if (clientUdp->badMove)
		{
			player.origen.x = clientUdp->realX;
			clientUdp->clientInfo->pos.x= clientUdp->realX;
			player.origen.y = clientUdp->realY;
			clientUdp->clientInfo->pos.y = clientUdp->realY;
			clientUdp->badMove = false;
		}

		sf::Event event;
		while (_window.pollEvent(event))
		{

			switch (event.type)
			{
			case sf::Event::Closed:
			{
				


				_window.close();
				break;
			}
			case sf::Event::KeyPressed:
			{
				if (event.key.code == sf::Keyboard::Escape)
				{
					sf::Packet packet;
					packet << ClientUdp::Head::DISCONNECTION;
					clientUdp->send(packet);

					_window.close();
				}

				if (event.key.code == sf::Keyboard::P)
				{
					if (!clientUdp->clientInfo->inMatchmake && !clientUdp->clientInfo->inLobby) {
						sf::Packet packet;
						packet << ClientUdp::Head::PLAY;
						packet << clientUdp->clientInfo->salt.d;
						clientUdp->send(packet);

						clientUdp->clientInfo->inMatchmake = true;
					}
				}
				if (event.key.code == sf::Keyboard::E)
				{
					if (clientUdp->clientInfo->inMatchmake) {
						sf::Packet packet;
						packet << ClientUdp::Head::END_MATCHMAKE;
						packet << clientUdp->clientInfo->salt.d;
						clientUdp->send(packet);
						clientUdp->clientInfo->inMatchmake = false;

						std::cout << "Matchmake ended" << std::endl;
					}
				}

				if (event.key.code == sf::Keyboard::Left)
				{
					/*if (player.origen.x <= 0 || player.origen.x == 26 && player.origen.y >= 12 && player.origen.y <= 17)
						break;

					std::cout << "LEFT\n";*/

					clientUdp->clientInfo->pos.x -= 0.3f;
					player.origen.x -= 0.3f;

					clientUdp->playerMoved = true;

				}
				else if (event.key.code == sf::Keyboard::Up)
				{
					/*if (player.origen.y <= 0 || player.origen.x >= 12 && player.origen.x <= 25 && player.origen.y == 18)
						break;

					std::cout << "UP\n";*/

					clientUdp->clientInfo->pos.y -= 0.3f;
					player.origen.y -= 0.3f;

					clientUdp->playerMoved = true;
				}
				else if (event.key.code == sf::Keyboard::Right)
				{
					/*if (player.origen.x >= (800 / SIZE) - 1 || player.origen.x == 11 && player.origen.y >= 12 && player.origen.y <= 17)
						break;

					std::cout << "RIGTH\n";*/

					clientUdp->clientInfo->pos.x += 0.3f;
					player.origen.x += 0.3f;
					clientUdp->playerMoved = true;
				}
				else if (event.key.code == sf::Keyboard::Down)
				{
					/*if (player.origen.y >= (600 / SIZE) - 1 || player.origen.x >= 12 && player.origen.x <= 25 && player.origen.y == 11)
						break;

					std::cout << "DOWN\n";*/

					clientUdp->clientInfo->pos.y+=0.3;
					player.origen.y += 0.3;
					clientUdp->playerMoved = true;

				}

				break;
			}

			}
		}
		_window.clear();

		for (int i = 0; i < W_WINDOW_TITLE; i++)
		{
			for (int j = 0; j < H_WINDOW_TITLE; j++)
			{

				shape.setFillColor(sf::Color(90, 90, 90, 255));


				shape.setPosition(sf::Vector2f(i*SIZE, j*SIZE));
				_window.draw(shape);
			}
		}

		player.Draw(_window);

		if(clientUdp->clientInfo->inLobby)
			otherPlayer.Draw(_window);
			

		_window.display();
	}
}

Graphics::~Graphics()
{
	delete clientUdp;
}
