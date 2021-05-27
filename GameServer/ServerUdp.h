#include <random>
#include <unordered_map>
#include <mutex>

union fp_bit_twiddler {
	double d;
	int i[2];
};


class ClientInfo
{
public:
	int id;
	fp_bit_twiddler salt;
	sf::Vector2f pos;

	int inLobbyCounter = 0;
	int AFK_Timer = 0;
	std::string name;
	bool inMatchmaking;
	bool inLobby;
	bool afk;
	//Other info
	ClientInfo() 
	{
		inMatchmaking = false;
		afk = false;
	}

	ClientInfo(int _id, sf::Vector2f _pos)
	{
		id = _id;
		pos = _pos;
		inMatchmaking = false;
		afk = false;
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
	fp_bit_twiddler salt;

	PeerAdress(sf::IpAddress _ip, unsigned short _port/*, int _localPacketID*/)
	{
		ip = _ip;
		port = _port;
		//localPacketID = _localPacketID;
	}

	friend bool operator==(const PeerAdress& p1, const PeerAdress& p2)
	{
		return p1.port == p2.port;
	}
};

class MyHashFunction {
public:

	// Use sum of lengths of first and last names 
	// as hash function. 
	size_t operator()(PeerAdress* p) const
	{
		return p->port;
	}
};

class Movement {
public:
	
	int id;
	int moveNum;
	sf::Vector2f position;
	unsigned short port;
	sf::IpAddress ip;

	Movement(){}

	Movement(int _id, int _moveNum, sf::Vector2f _position, unsigned short _port, sf::IpAddress _ip)
	{
		id = _id;
		moveNum = _moveNum;
		position = _position;
		port = _port;
		ip = _ip;
	}

};

struct PacketCritics
{
	//Guarda PeerAddress donde debe enviar este paquete
	PeerAdress* peer;

	//Paquete en si
	sf::Packet paquete;

	PacketCritics(PeerAdress* _peer, sf::Packet _paquete)
	{
		peer = _peer;
		paquete = _paquete;
	}

};

struct AllClientInfo
{
	ClientInfo* clientInfo;
	PeerAdress* peerAdress;

	AllClientInfo() {}

	AllClientInfo(ClientInfo* _clientInfo, PeerAdress* _peerAdress)
	{
		clientInfo = _clientInfo;
		peerAdress = _peerAdress;
	}
};

class ServerUdp
{
public:
	sf::UdpSocket* socket;
	sf::Socket::Status status;

	std::mutex mtx;

	std::list<std::pair<PeerAdress*,float>> waitingClients;

	//std::unordered_map<unsigned short, ClientInfo*> clients;
	std::unordered_map<unsigned short, AllClientInfo*> clients;
	//std::unordered_map<unsigned short, AllClientInfo*> matchMakingPlayers;

	////////////

	//std::list<std::list<std::pair<PeerAdress*, ClientInfo*>>> matchMakingRooms;

	std::list<Movement*> movements;

	std::thread matchMakingThread;
	std::thread AFKthread;

	//=======================MARC===================================

	//Paquetes criticos
	//std::unordered_map<int, sf::Packet> clients;
	//std::map<std::pair<PeerAdress*, sf::Packet>> listMsgNonAck; //Lista para guardar mensajes sin confirmacion //PeerAddres en vez de client info
	std::map<int, PacketCritics*> mapMsgNonAck; //Map para guardar mensajes sin confirmacion


	//THREAD
	int paquete;
	int crtkPkId = 0;
	std::thread paquetesCriticos;
	void TratamientoPaquetesCriticos();

	//==========================================================

	std::thread commandsThread;
	std::list<std::pair<int, Movement*>> movementIDOrder; 

	//Moviment-> 0,20 
	


	enum Head {
		MOVE, HELLO, WELCOME, CHALLENGE, PLAY, PLAYER_JOINED, END_MATCHMAKE, ACK, DISCONNECTION, END_LOBBY, PING
	};

	ServerUdp();
	void send(sf::Packet, sf::IpAddress, unsigned short, sf::UdpSocket*);
	void receive();
	void updatePositions();
	void joinCommands();
	static float GetRandomFloat();
	void MatchMaking();
	void AFKController();
	bool DistanceFuncion(std::string n1, std::string n2);
	void LobbyTimer(AllClientInfo* c1, AllClientInfo* c2);
	~ServerUdp();
};