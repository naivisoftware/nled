// nledtcpclientexampleapp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Std includes
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <time.h>

// Server command ids
#include <nledserverids.h>

// Asio
#include <asio.hpp>

// Namespace
using asio::ip::tcp;
using namespace std;
using namespace asio::detail::socket_ops;

// typedefs
typedef map<int, unsigned char*> LedColorMap;

// Holds server sampled display information
static int				sLedDisplayCount;
static vector<int>		sDisplayNumbers;
map<int, vector<int>>	sDisplayInfo;



/**
@brief Reads a single int from the stream
**/
INT32 ReadInt(tcp::socket& inSocket, asio::error_code& ioError)
{
	INT32 sample_data(-1);
	asio::read(inSocket, asio::buffer(&sample_data, sizeof(INT32)), ioError);
	return network_to_host_long(sample_data);
}



/**
@brief Sends an int to the server
**/
void WriteInt(INT32 inInt, tcp::socket& inSocket, asio::error_code& ioError)
{
	INT32 formatted_int = host_to_network_long(inInt);
	asio::write(inSocket, asio::buffer(&formatted_int, sizeof(INT32)), ioError);
}



/**
@brief Reads the led config
**/
bool ReadLedConfig(tcp::socket& inSocket)
{

	// Ask for led config
	asio::error_code error;
	WriteInt(NLED_ID_GETCONFIG, inSocket, error);

	// Make sure the data was send
	if(error)
		return false;

	// Get number of panels
	sLedDisplayCount = ReadInt(inSocket, error);
	if(error)
		return false;
	cout << "Number of panels: " << sLedDisplayCount << "\n";
	
	// Sample panel configs
	for(int i = 0; i < sLedDisplayCount; i++)
	{
		// Get panel id
		INT32 display_id = ReadInt(inSocket, error);
		if(error)
			return false;
		sDisplayNumbers.push_back(display_id);

		// Get panel info
		vector<int> info;
		for(int s = 0; s < 4; s++)
		{
			info.push_back(ReadInt(inSocket, error));
			if(error)
				return false;
		}
		sDisplayInfo[display_id] = info;
	}

	return true;
}


/**
@brief Updates the displays by repopulating the color map
**/
void UpdateDisplayColors(tcp::socket& inSocket, LedColorMap& inColorMap)
{
	asio::error_code error;
	INT32 ask(NLED_ID_DRAW_PANEL);

	for(auto d : sDisplayInfo)
	{
		// Send over the panel to set data for
		int display_id = d.first;
		int display_he = d.second[2];
		int display_wi = d.second[3];

		// Get the pointer to the temp display data
		unsigned char* display_data = inColorMap[d.first];

		// Create random display color
		unsigned char cr = rand() % 255;
		unsigned char cg = rand() % 255;
		unsigned char cb = rand() % 255;
		printf("Color Display %02d: %03d %03d %03d\n", d.first, (int)cr, (int)cg, (int)cb);

		// Set the color for the display
		for(int y = 0; y < display_he; y++)
		{
			int yoff = y * display_wi;
			for(int x = 0; x < display_wi; x++)
			{
				int   index = (yoff + x) * 3;
				float scale =  pow(((float)x / (float)(display_wi-1)), 1.0f);

				display_data[index + 0] = cr * scale;
				display_data[index + 1] = cg * scale;
				display_data[index + 2] = cb * scale;
			}
		}

		// Signal that we want to change display color for panel
		//asio::write(inSocket, asio::buffer(&ask, sizeof(INT32)), error);
		WriteInt(ask, inSocket, error);

		// Signal panel id
		WriteInt(display_id, inSocket, error);

		// Write buffer
		asio::write(inSocket, asio::buffer(display_data, d.second[1]), error);
	}

	WriteInt(NLED_ID_FLUSH, inSocket, error);
}



/**
@brief Update All Display Colors at once
**/
void UpdateAllDisplayColors(tcp::socket& inSocket, LedColorMap& inColorMap)
{
	asio::error_code error;
	INT32 ask(NLED_ID_DRAW_ALL);

	// Signal start drawing
	WriteInt(NLED_ID_DRAW_ALL, inSocket, error);

	// Set all data for panels
	for(auto d : sDisplayInfo)
	{
		int display_id = d.first;
		int display_he = d.second[2];
		int display_wi = d.second[3];

		// Get the pointer to the temp display data
		unsigned char* display_data = inColorMap[d.first];

		// Create random display color
		unsigned char cr = rand() % 255;
		unsigned char cg = rand() % 255;
		unsigned char cb = rand() % 255;
		printf("Color Display %02d: %03d %03d %03d\n", d.first, (int)cr, (int)cg, (int)cb);

		// Set the color for the display (populate local buffer)
		for(int y = 0; y < display_he; y++)
		{
			int yoff = y * display_wi;
			for(int x = 0; x < display_wi; x++)
			{
				int   index = (yoff + x) * 3;
				float scale =  pow(((float)x / (float)(display_wi-1)), 1.0f);

				display_data[index + 0] = cr * scale;
				display_data[index + 1] = cg * scale;
				display_data[index + 2] = cb * scale;
			}
		}

		// Send the panel number and data for that panel
		asio::write(inSocket, asio::buffer(display_data, d.second[1]), error);
	}

	WriteInt(NLED_ID_FLUSH, inSocket, error);
}




/**
@brief Connects to the server specified as the command line argument (Name)
**/
int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: client <host> " << "port <number> " << std::endl;
		return 1;
	}

	// Initialize random function
	srand(time(nullptr));

	// All programs that use asio need to have at least one io_service object.
	asio::io_service io_service;

	// We need to turn the server name that was specified as a parameter to the application, into a TCP endpoint. To do this we use an ip::tcp::resolver object.
	tcp::resolver resolver(io_service);

	// Get the port number as a string
	string server_name = argv[1];
	string port_number = argv[2];

	std::cout << "Connecting to: " << server_name.c_str() << " port number: " << port_number.c_str() << "\n";

	// A resolver takes a query object and turns it into a list of endpoints. We construct a query using the name of the server, specified in argv[1], and the name of the service, in this case "daytime".
	tcp::resolver::query query(server_name, port_number);

	// The list of endpoints is returned using an iterator of type ip::tcp::resolver::iterator. (Note that a default constructed ip::tcp::resolver::iterator object can be used as an end iterator.)
	asio::error_code error;
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query, error);

	// Now we create and connect the socket. The list of endpoints obtained above may contain both IPv4 and IPv6 endpoints, so we need to try each of them until we find one that works. This keeps the client program independent of a specific IP version. The asio::connect() function does this for us automatically.
	tcp::socket socket(io_service);
	asio::connect(socket, endpoint_iterator, error);

	// Return if we can't connect
	if(error)
	{
		cout << "Unable to connect to: " << server_name.c_str() << "\n";
		return -1;
	}

	// Read the led config
	if(!ReadLedConfig(socket))
	{
		cout << "Unable to retrieve LED Configuration";
		socket.close();
		return -1;
	}

	// Create data map
	LedColorMap map;
	for(auto d : sDisplayInfo)
	{
		int size = d.second[1];
		map[d.first] = new unsigned char[size];
	}

	// Update display colors
	// On any key send pixel data to individual panels
	while(true)
	{
		std::cout << "Press any key to change display colors, q to quit";
		std::string return_line;
		getline(std::cin, return_line);
		if(return_line == "q")
			break;
		UpdateDisplayColors(socket, map);
	}

	// Close connection
	socket.close();
}

