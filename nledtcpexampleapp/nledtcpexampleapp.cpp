// nledtcpexampleapp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <nledserver.h>

// Std
#include <iostream>
#include <ctime>
#include <assert.h>
#include <string>

using namespace nledserver;

int _tmain(int argc, _TCHAR* argv[])
{
	// Default port is 13
	int port_number(7845);

	// Try to get the port number from the command arguments
	if(argc > 1)
	{
		std::string port_number_str = argv[1];
		port_number = atoi(port_number_str.c_str());
	}

	// Make sure the port is valid
	if(port_number == 0)
	{
		std::cout << "ERROR: Invalid port specified\n";
		return -1;
	}
	else
		std::cout << "Selected port: " << port_number << "\n\n";

	// Create and start server
	NLedServer led_server(port_number);
	led_server.StartServer();

	// Restart server if connection was lost
	while (true)
	{
		cout << "Connection lost, restarting...";
		led_server.RestartServer();
	}
	return 0;
}

