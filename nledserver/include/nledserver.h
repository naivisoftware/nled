#pragma once

// Include asio networking lib
#include <asio.hpp>

// Resolve namespaces
using namespace std;
using asio::ip::tcp;

namespace nledserver
{

	/**
	@brief NLedServer

	Acts as a server that can handle led panel specific requests
	This server runs synchronous and can retrieve and set led related data
	**/
	class NLedServer
	{
	public:
		//@name Construction / Destruction
		NLedServer(int inPortNumber);
		virtual ~NLedServer();

		//@name Starts the server
		void StartServer();
		void RestartServer();

		//@name Getters
		int	 GetPortNumber() const			{ return mPortNumber; }

	private:
		tcp::endpoint*						mEndpoint;
		tcp::acceptor*						mDataAcception;
		int									mPortNumber;
		asio::io_service					mIOService;
		tcp::socket*						mSocket;

		void HandleClientCommands();										//< Interfaces with the led lib
		void Init();														//< Initializes the server and nled lib
	};
}