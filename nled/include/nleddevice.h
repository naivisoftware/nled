#pragma once

// Serial Includes
#include <serial/ww_serial.h>

using namespace std;
using namespace serial;

/**
@brief Describes the hardware led layout 
**/
struct NLedDevice
{
	NLedDevice(const PortInfo& inPortInfo) : mValid(false), mPortInfo(inPortInfo), mRGBDataPanelOne(nullptr), mRGBDataPanelTwo(nullptr)							
	{
		mConnection.setPort(inPortInfo.port);
		mConnection.setBaudrate(9600);
		mConnection.setTimeout(Timeout::simpleTimeout(1000));
	}

	Serial			mConnection;							//< Serial connection to micro controller
	int				mStripLength;							//< Amount of leds on one strip
	int				mLedHeight;								//< Amount of leds in height
	bool			mValid;									//< If the led device is valid and operationg
	bool			mLayout;								//< Left to right / right to left
	string			mDeviceName;							//< Interface name
	int				mUUID;									//< Unique identifier of device
	PortInfo		mPortInfo;								//< Port information
	int				mPanelUUIDOne;						//< Panel id number 1
	int				mPanelUUIDTwo;						//< Panel id number 2
	int				mByteSize;								//< Total number of bytes associated with displays associated with this device

	// User Data
	unsigned char*	mRGBDataPanelOne;						//< RGB data for panel one
	unsigned char*	mRGBDataPanelTwo;						//< RGB data for panel two
	
	// Panel Data
	unsigned char*	mConvertedData;							//< Holds the converted RGB pixel data
};