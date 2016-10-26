#pragma once

// Standard lib includes
#include <string>
#include <vector>

// Asio Includes
#include <asio.hpp>

// command Ids
#include <nledserverids.h>

// Namespace
using namespace std;
using asio::ip::tcp;
using namespace asio::detail::socket_ops;

namespace nledserver
{
	// Forward declares
	class NLedServer;

	//////////////////////////////////////////////////////////////////////////
	class LedCommand
	{
	public:
		LedCommand(INT32 inCmdID, const string& inName) : mCommandID(inCmdID), mCommandName(inName)			{ }

		/// @name Returns command name
		INT32			GetCommandID() const			{ return mCommandID; }
		const string&	GetCommandName() const			{ return mCommandName; }

		///@name Performs server side action
		virtual bool	PerformAction(tcp::socket& inSocket, asio::error_code& ioError) = 0;

	private:
		INT32			mCommandID;
		string			mCommandName;
	};

	//////////////////////////////////////////////////////////////////////////

	extern const vector<LedCommand*>&	GetLedServerCommands();

	extern LedCommand*					GetLedCommand(INT32 inID);

	extern void							ReadUntilDelimiter(tcp::socket& inSocket, string& ioBuffer, asio::error_code& ioError);

	extern INT32						ReadInt(tcp::socket& inSocket, asio::error_code& ioError);

	extern void							SendInt(INT32 inInt, tcp::socket& inSocket, asio::error_code& ioError);

	extern const char					gLedCommandDelimiter; //< ';'

	//////////////////////////////////////////////////////////////////////////

	/**
	@brief LedGetConfig

	Returns the total number of available led panels as an int.
	After that: the panel id (unique identifier), total number of leds, total buffer size,
	height and width. 
	
	The data would look like this: [(total)6][2,900, 2700, 30, 30][3,900, 2700, 30, 30]..[]
	**/
	class LedGetConfig : LedCommand
	{
	public:
		LedGetConfig() : LedCommand(NLED_ID_GETCONFIG, "GetConfig")									{ }
		bool PerformAction(tcp::socket& inSocket, asio::error_code& ioError);
	};



	/**
	@brief Allows setting of data for all led displays in one go

	This function will cycle over every unique display id (in order as received by LedGetConfig).
	The total amount of bytes this command pulls is similar to the entire byte buffer size.
	**/
	class LedSetAll : LedCommand
	{
	public:
		LedSetAll() : LedCommand(NLED_ID_DRAW_ALL, "DrawAll")						{ }
		bool PerformAction(tcp::socket& inSocket, asio::error_code& ioError);
	};



	/**
	@brief Call this to flush the set display data to led hardware
	**/
	class LedFlush : LedCommand
	{
	public:
		LedFlush() : LedCommand(NLED_ID_FLUSH, "Flush")				{ }
		bool PerformAction(tcp::socket& inSocket, asio::error_code& ioError);
	};



	/**
	@brief Allows setting of display specific data
	After calling the command the server expects a unique panel identifier and an unsigned char array
	Syntax: [PanelIdx;][Display Data]
	**/
	class LedSetPanel : LedCommand
	{
	public:
		LedSetPanel() : LedCommand(NLED_ID_DRAW_PANEL, "DrawPanel")			{ }
		bool PerformAction(tcp::socket& inSocket, asio::error_code& ioError);
	};



	/**
	@brief Sets the debug mode on / off for the led server
	**/
	class LedSetDebugMode : LedCommand
	{
	public:
		LedSetDebugMode() : LedCommand(NLED_SET_DEBUG_MODE , "DebugMode")	{ }
		bool PerformAction(tcp::socket& inSocket, asio::error_code& ioError);
	};
}