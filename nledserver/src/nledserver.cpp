#include <nledserver.h>

// nled includes
#include <nled.h>

// std includes
#include <iostream>
#include <string>
#include <ctime>
#include <vector>

// server cmd includes
#include <nledservercommands.h>

/**
@brief Creates a string that shows connection time
**/
std::string CreateConnectionString()
{
	// Buffer that will hold current time
	static char* tbuffer(nullptr);
	static char* fbuffer(nullptr);
	if(tbuffer == nullptr)
	{
		tbuffer = new char[26];
		fbuffer = new char[250];
	}

	// Sample time
	time_t now = time(0);
	ctime_s(tbuffer, 26, &now);

	// Print in to final buffer
	sprintf_s(fbuffer, 250, "Client connection established on: %s", tbuffer);
	return string(fbuffer);
}



/**
@brief Constructor
**/
nledserver::NLedServer::NLedServer(int inPortNumber) : mPortNumber(inPortNumber),
	mEndpoint(nullptr),
	mDataAcception(nullptr),
	mSocket(nullptr)
{
	Init();
}



/**
@brief Init
**/
void nledserver::NLedServer::Init()
{
	// Create connection point
	mEndpoint = new tcp::endpoint(tcp::v4(), mPortNumber);

	// Create data acception pipe using the io service
	mDataAcception = new tcp::acceptor(mIOService, *mEndpoint);

	// Initialize the panels
	cout << "Initializing LED panels\n\n";
	nled::InitDisplays(1.75f);

	// Create the buffers for every panel and assign to display pointer
	cout << "Creating Display buffers\n\n";
	int* displays = nled::GetAvailableDisplayNumbers();
	for(int i=0; i< nled::GetDisplayCount(); i++)
	{
		// Get size in bytes
		int byte_size = nled::GetDisplayByteSize(displays[i]);

		// Create display buffer
		unsigned char* data = new unsigned char[byte_size];
		
		// Set to 0
		for(int p = 0 ; p < byte_size; p++)
			data[p] = 0;

		// Initialize data for display
		nled::SetData(displays[i], data);
	}
}



/**
@brief Destructor
**/
nledserver::NLedServer::~NLedServer()
{
	// Delete server data
	delete mEndpoint, mDataAcception;
	if(mSocket)  { delete mSocket; }

	// Delete displays associated data
	int* displays = nled::GetAvailableDisplayNumbers();
	for(int i=0; i< nled::GetDisplayCount(); i++)
	{
		unsigned char* data = new unsigned char[nled::GetDisplayByteSize(displays[i])];
		delete nled::GetData(displays[i]);
	}
}



/**
@brief Starts running the server

Note that this is not threaded and will hold execution on main thread
**/
void nledserver::NLedServer::StartServer()
{
	// Clear existing socket
	if(mSocket) 
		delete mSocket;

	// This is an iterative server, which means that it will handle one connection at a time. Create a socket that will represent the connection to the client, and then wait for a connection.
	mSocket = new tcp::socket(mIOService);

	// Show that we're waiting for a connection
	std::cout << "\nStarted NLED server on port: " << mPortNumber << " ,waiting for connection\n";

	// Wait until someone connects
	mDataAcception->accept(*mSocket, *mEndpoint);

	// After this point someone connected
	std::cout << CreateConnectionString().c_str() << "\n";

	// We're read to handle client commands
	HandleClientCommands();

	// Close socket after client connection close
	mSocket->close();
}



/**
@brief Restarts the server
**/
void nledserver::NLedServer::RestartServer()
{
	assert(mSocket);
	if(mSocket->is_open())
		mSocket->close();

	StartServer();
}



/**
@brief Handles client commands
**/
void nledserver::NLedServer::HandleClientCommands()
{
	// Holds the command up to the newline character
	string sampled_data;
	sampled_data.reserve(250);

	// Possible error code
	asio::error_code ioError;

	// Receive commands
	while(true)
	{
		// Command id
		INT32 read = ReadInt(*mSocket, ioError);

		// Read potential error
		if(ioError == asio::error::eof)
		{
			cout << "Client disconnected\n";
			break;
		}

		// General error occurred
		else if(ioError)
		{
			cout << "ERROR: " << ioError.message().c_str() << " closing connection";
			break;
		}

		// Based on received char set, get the led cmd
		nledserver::LedCommand* led_cmd = nledserver::GetLedCommand(read);
		if(led_cmd == nullptr)
		{
			cout << "Unknown led server command: " << sampled_data << "\n";
			continue;
		}

		// Execute cmd
		if(!led_cmd->PerformAction(*mSocket, ioError) || ioError)
		{
			cout << "Unable to execute led action: " << led_cmd->GetCommandName().c_str() << "\n";
			break;
		}
	}
}

