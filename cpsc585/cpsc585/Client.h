#pragma once

#include <winsock.h>
#include <iostream>
#include <sstream>
#include <string.h>

#include "Intention.h"

class Client
{
public:
	Client();
	~Client();

	bool connectToServer(int port, std::string ipAddress);
	void getTCPMessages();

	int ready();

	int sendButtonState(Intention intention);

	std::string world;
	std::string track;

private:
	int sendTCPMessage(std::string message);
	int sendUDPMessage(std::string message);
	void closeConnection();

	SOCKET sTCP;
	SOCKET sUDP;
	SOCKADDR_IN target; //Socket address info

	bool start;
};
