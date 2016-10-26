// Include server commands
#include <nledservercommands.h>

// Include led server
#include <nledserver.h>

// Include nled interface
#include <nled.h>

// Include std
#include <iostream>

// Namespaces
using namespace std;

/**
@brief The command delimiter
**/
const char nledserver::gLedCommandDelimiter(';');



/**
@brief Returns all available led server commands
**/
const vector<nledserver::LedCommand*>& nledserver::GetLedServerCommands()
{
	static vector<nledserver::LedCommand*> led_cmds;
	if(led_cmds.empty())
	{
		led_cmds.push_back((nledserver::LedCommand*)(new nledserver::LedSetAll()));
		led_cmds.push_back((nledserver::LedCommand*)(new nledserver::LedGetConfig()));
		led_cmds.push_back((nledserver::LedCommand*)(new nledserver::LedFlush()));
		led_cmds.push_back((nledserver::LedCommand*)(new nledserver::LedSetPanel()));
		led_cmds.push_back((nledserver::LedCommand*)(new nledserver::LedSetDebugMode()));
	}
	return led_cmds;
}



/**
@brief Returns the led command associated with the name provided

nullptr if cmd is not found
**/
nledserver::LedCommand* nledserver::GetLedCommand(INT32 inID)
{
	for(const nledserver::LedCommand* cmd : nledserver::GetLedServerCommands())
	{
		if(cmd->GetCommandID() == inID)
			return const_cast<nledserver::LedCommand*>(cmd);
	}
	return nullptr;
}



/**
@brief Reads the data from the socket until the delimiter is found
**/
void nledserver::ReadUntilDelimiter(tcp::socket& inSocket, string& ioBuffer, asio::error_code& ioError)
{
	// Holds the 1 byte received data
	char   received_data;

	// Clear io buffer
	ioBuffer.clear();

	// Iterate over socket until delimiter is found
	while(true)
	{
		asio::read(inSocket, asio::buffer(&received_data, 1), ioError);
		if(received_data == gLedCommandDelimiter || ioError )
			break;
		ioBuffer.push_back(received_data);
	}
}



/**
@brief Reads a single int from the stream
**/
INT32 nledserver::ReadInt(tcp::socket& inSocket, asio::error_code& ioError)
{
	INT32 sample_data(-1);
	asio::read(inSocket, asio::buffer(&sample_data, sizeof(INT32)), ioError);
	return network_to_host_long(sample_data);
}



/**
@brief Sends an int
**/
void nledserver::SendInt(INT32 inInt, tcp::socket& inSocket, asio::error_code& ioError)
{
	INT32 formatted_int = host_to_network_long(inInt);
	asio::write(inSocket, asio::buffer(&formatted_int, sizeof(INT32)), ioError);
}



/**
@brief LedGetConfig

Returns the total number of available led panels as an int.
After that: the panel id (unique identifier), total number of leds, total buffer size,
height and width. 

The data would look like this: [(total)6][2,900, 2700, 30, 30][3,900, 2700, 30, 30]..[]
**/
bool nledserver::LedGetConfig::PerformAction(tcp::socket& inSocket, asio::error_code& ioError)
{
	int* display_numbers = nled::GetAvailableDisplayNumbers();
	int  display_count	 = nled::GetDisplayCount();

	// First send the amount of panels as an int
	SendInt(display_count, inSocket, ioError);

	// Send amount of panels
	if(ioError)
	{
		cout << "ERROR: Unable to send panel count\n";
		return false;
	}

	INT32 display_number(-1);
	INT32 display_data(-1);
	INT32 data(-1);

	// Send over individual display numbers
	for(int i = 0 ; i < display_count ; i++)
	{
		// Send display number
		display_number = display_numbers[i];
		SendInt(display_number, inSocket, ioError);

		// Send config data (size)
		data = nled::GetDisplaySize(display_number);
		SendInt(data, inSocket, ioError);

		// Send total byte size
		data = nled::GetDisplayByteSize(display_number);
		SendInt(data, inSocket, ioError);

		// Send height
		data = nled::GetDisplayHeight(display_number);
		SendInt(data, inSocket, ioError);

		// Send width
		data = nled::GetDisplayStride(display_number);
		SendInt(data, inSocket, ioError);

		// Ensure data was send
		if(ioError)
		{
			cout << "ERROR: Unable to send display configuration: " << display_number << "\n";
			return false;
		}
	}

	// Return success
	return true;
}



/**
@brief Allows setting of display specific data
After calling the command the server expects a unique panel identifier and an unsigned char array
Syntax: [PanelIdx;][Display Data]
**/
bool nledserver::LedSetPanel::PerformAction(tcp::socket& inSocket, asio::error_code& ioError)
{
	// Read the panel id
	UINT32 panel_id = ReadInt(inSocket, ioError);

	if(ioError)
	{
		cout << "ERROR: Unable to read display number\n";
		return false;
	}

	// Check if the display exists
	if(!nled::DisplayExists(panel_id))
	{
		cout << "ERROR: Display with number: " << panel_id << " does not exist\n";
		return false;
	}

	// Get buffer and size of buffer to set
	int bytes_to_fetch = nled::GetDisplayByteSize(panel_id);
	unsigned char* display_data = nled::GetData(panel_id);
	
	// Read data from stream directly in to buffer
	asio::read(inSocket, asio::buffer(display_data, bytes_to_fetch), ioError);
	if(ioError)
	{
		cout << "ERROR: Unable to read display stream for panel: " << panel_id << "\n";
		return false;
	}

	return true;
}



/**
@brief Allows setting of data for all led displays in one go

This function will cycle over every unique display id (in order as received by LedGetConfig).
The total amount of bytes this command pulls is similar to the entire byte buffer size.
**/
bool nledserver::LedSetAll::PerformAction(tcp::socket& inSocket, asio::error_code& ioError)
{	
	INT32 panel_id(0);
	int*  panel_ids = nled::GetAvailableDisplayNumbers();

	// Cycle over every panel and set data accordingly
	for(int i = 0; i < nled::GetDisplayCount(); i++)
	{
		// Get the current panel id to address
		panel_id = panel_ids[i];

		// Figure out how many bytes we need to fetch
		int bytes_to_fetch = nled::GetDisplayByteSize(panel_id);
		
		// Get the pointer in memory regarding the panel we want to update
		unsigned char* display_data = nled::GetData(panel_id);
		
		// Read byte stream
		asio::read(inSocket, asio::buffer(display_data, bytes_to_fetch), ioError);
		if(ioError)
		{
			cout << "Error reading data for display: " << panel_id << "\n";
			return false;
		}
	}

	// If we get to this point all the data was set correctly
	return true;
}



/**
@brief Signals the system that we're done drawing and want to send the data to the panels
**/
bool nledserver::LedFlush::PerformAction(tcp::socket& inSocket, asio::error_code& ioError)
{
	// Flush to devices
	nled::EndDisplay();

	return true;
}



/**
@brief Sets the debug mode on / off for the led server
**/
bool nledserver::LedSetDebugMode::PerformAction(tcp::socket& inSocket, asio::error_code& ioError)
{
	// Read the int. ensure it's valid
	INT32 mode = ReadInt(inSocket, ioError);
	if(mode != 0 || mode != 1)
	{
		cout << "Error reading debug display mode\n";
		return false;
	}

	// TODO: Set debug mode on /  off
	return true;
}
