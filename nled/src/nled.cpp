#include <nled.h>

// Serial Lib
#include <serial/ww_serial.h>

// Led devices
#include <nleddevice.h>

// Standard Includes
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <chrono>      
#include <thread>
#include <assert.h>
#include <thread>

// Namespace
using namespace serial;
using namespace std;

// Typedefs
typedef map<int, NLedDevice*>	NLedDeviceMap;

//////////////////////////////////////////////////////////////////////////
// Statics local to this module
//////////////////////////////////////////////////////////////////////////
const static int				sBautRate(9600);
static NLedDeviceMap			sLedInterfaces;								//< Holds all the hardware interfaces
static NLedDeviceMap			sDisplayToInterfaceMap;						//< Maps panel numbers to hardware interfaces

const static int				sBytesPerLed(3);							//< Total amount of bytes per led
static int*						sLedDisplayNumbers(nullptr);				//< Will hold a flat array of unique display id's
static int*						sGammaTable(nullptr);						//< Will hold the gamma table
const static int				sLedCharBufferOffset(3);					//< Holds the hardware device offset to color buffers


//////////////////////////////////////////////////////////////////////////
// Module specific functionality
//////////////////////////////////////////////////////////////////////////

/**
@brief Initializes (populates) a led device based on the received serial data
**/
static bool InitLedDevice(NLedDevice& inDevice, const PortInfo& inPortInfo)
{
	// Open connection
	inDevice.mConnection.open();

	// Make sure the connection is valid
	if(!inDevice.mConnection.isOpen())
	{
		cout << "ERROR: unable to open connection to port: " << inPortInfo.port.c_str() << ", " << inPortInfo.description.c_str() << "\n";
		return false;
	}

	if(!inDevice.mConnection.write(string("?")))
	{
		cout << "ERROR: unable to send interface query command to port: " << inPortInfo.port.c_str() << ", " << inPortInfo.description.c_str() << "\n";
		inDevice.mConnection.close();
		return false;
	}

	// Wait before reading
	this_thread::sleep_for(chrono::seconds(1));

	// Get info
	unsigned char* teensy_info = new unsigned char[250];
	inDevice.mConnection.read(teensy_info, 250);
	vector<string> parsed_info;

	// Sample info
	string temp_info;
	for(int i=0; i < 250; i++)
	{
		if(teensy_info[i] == '\n') { break; }

		if(teensy_info[i] == ',')
		{
			parsed_info.push_back(temp_info);
			temp_info.clear();
			continue;;
		}

		temp_info += teensy_info[i];
	}

	// Validate received info (bit hacky)
	if(parsed_info.size() == 0)
	{
		cout << "ERROR: unable to read led display layout configuration for device on port: " << inPortInfo.port.c_str() << ", " << inPortInfo.description.c_str() << "\n";
		inDevice.mConnection.close();
		return false;
	}

	// Sample device settings
	inDevice.mLayout = atoi(parsed_info[5].c_str()) == 0;
	inDevice.mStripLength = atoi(parsed_info[0].c_str());
	inDevice.mLedHeight = atoi(parsed_info[1].c_str());
	inDevice.mUUID = atoi(parsed_info[11].c_str());
	inDevice.mDeviceName = "Interface" + to_string(inDevice.mUUID);
	inDevice.mPanelUUIDOne = (inDevice.mUUID * 2) + 0;
	inDevice.mPanelUUIDTwo = (inDevice.mUUID * 2) + 1;

	// Log warning regarding height being a multiple of 8
	if(inDevice.mLedHeight % 8 != 0)
	{
		assert(false);
		cout << "WARNING: Display height nog a multiple of 8 for device on port: " << inPortInfo.port.c_str() << ", " << inPortInfo.description.c_str() << "\n";
	}

	// Create the container for the output buffer
	inDevice.mByteSize = (inDevice.mLedHeight * inDevice.mStripLength * sBytesPerLed) + sLedCharBufferOffset;
	inDevice.mConvertedData = new unsigned char[inDevice.mByteSize];

	delete[] teensy_info;

	inDevice.mValid = true;
	return true;
}



/**
@brief Converts a packed rgb color in to a led compatible data format
**/
static int GetColorWiring(int c) 
{
	int red =	(c & 0xFF0000) >> 16;
	int green = (c & 0x00FF00) >> 8;
	int blue =	(c & 0x0000FF);
	
	red =	sGammaTable[red];
	green = sGammaTable[green];
	blue =	sGammaTable[blue];

	return (green << 16) | (red << 8) | (blue);		// GRB - most common wiring
}



/**
@brief Converts the char data of every panel in to led led data streams
**/
void PixelsToLed(NLedDevice* inDevice)
{
	int  width(inDevice->mStripLength);
	int  height(inDevice->mLedHeight);
	int  offset(sLedCharBufferOffset);
	int  pixel[8];
	bool layout(inDevice->mLayout);
	int  strips_per_pin(height / 8);

	// Variables used in this loop
	int x, y, xbegin, xend, xinc, mask;

	// Holds the unsigned char data to be send over
	unsigned char red(0);
	unsigned char gre(0);
	unsigned char blu(0);

	// Where to sample from in the image
	int image_index(-1);

	// Calculate max index value
	int  display_max_index = (inDevice->mStripLength * (inDevice->mLedHeight/2));
	bool second_panel(false);

	// For the amount of horizontal strips connected to a pin, iterate over every horizontal pixel
	// Sample the color for that horizontal led on every pin (total number of 8)
	for (y = 0; y < strips_per_pin; y++) 
	{
		if ((y & 1) == (layout ? 0 : 1)) 
		{
			// even numbered rows are left to right
			xbegin = 0;
			xend = width;
			xinc = 1;
		} 
		else 
		{
			// odd numbered rows are right to left
			xbegin = width - 1;
			xend = -1;
			xinc = -1;
		}

		// Iterate over every horizontal pixel per strip and convert color data
		for (x = xbegin; x != xend; x += xinc) 
		{
			for (int i=0; i < 8; i++) 
			{
				// Calculate image lookup index and sample color values
				image_index = x + (y + strips_per_pin * i) * width;
				unsigned char* image_data = image_index < display_max_index ?  inDevice->mRGBDataPanelOne : inDevice->mRGBDataPanelTwo;  
				
				// Remap image index based on sample display
				image_index = image_index % display_max_index;
				
				red = image_data[(image_index * sBytesPerLed) + 0]; 
				gre = image_data[(image_index * sBytesPerLed) + 1];
				blu = image_data[(image_index * sBytesPerLed) + 2];

				// Combine in to int and get correct led data set
				pixel[i] = red << 16|gre << 8|blu << 0;

				// Sample to int and update color wiring
				pixel[i] = red<<16|gre<<8|blu<<0;
				pixel[i] = GetColorWiring(pixel[i]);
			}

			// convert 8 pixels to 24 bytes (some black magic here)
			for (mask = 0x800000; mask != 0; mask >>= 1) 
			{
				unsigned char b = 0;
				for (int i=0; i < 8; i++) 
				{
					if ((pixel[i] & mask) != 0) b |= (1 << i);
				}

				inDevice->mConvertedData[offset++] = b;
			}
		}
	}

	// Fill first 3 bytes with sync info
	inDevice->mConvertedData[0] = '*';							// first device is the frame sync master
	int usec = (int)((1000000.0 / 30) * 0.75);
	inDevice->mConvertedData[1] = (unsigned char)(usec);		// request the frame sync pulse
	inDevice->mConvertedData[2] = (unsigned char)(usec >> 8);	// at 75% of the frame time
}



/**
@brief Thread safe method to convert and transfer pixel data to hardware device
**/
void FlushToDevice(NLedDevice* inDevice)
{
	PixelsToLed(inDevice);
	inDevice->mConnection.write(inDevice->mConvertedData, inDevice->mByteSize);
}



/**
@brief Finds the led device for the given display number
**/
static NLedDevice* FindLedDevice(int inDisplayNumber)
{
	// Find in map
	NLedDeviceMap::iterator it = sDisplayToInterfaceMap.find(inDisplayNumber);
	
	// Return pointer or null
	return it == sDisplayToInterfaceMap.end() ? nullptr : it->second;
}



/**
@brief Initialize all the available displays
**/
void nled::InitDisplays(float inGammaValue)
{
	// Clear all existing led devices
	sLedInterfaces.clear();
	sDisplayToInterfaceMap.clear();

	// Cycle over all avaialble devices and add valid interfaces
	vector<PortInfo> found_devices = serial::list_ports();
	for(const PortInfo& p : found_devices)
	{
		// Create a new led communication device 
		NLedDevice* new_led_device = new NLedDevice(p);
		
		// Initialize the device based on the current port
		InitLedDevice(*new_led_device, p);

		// Make sure the device is valid (initialized and open)
		if(!new_led_device->mValid)
		{
			delete new_led_device;
			continue;
		}

		// Make sure the id is unique for the device and displays attached to device
		assert(sLedInterfaces.find(new_led_device->mUUID) == sLedInterfaces.end());
		assert(sDisplayToInterfaceMap.find(new_led_device->mPanelUUIDOne) == sDisplayToInterfaceMap.end());
		assert(sDisplayToInterfaceMap.find(new_led_device->mPanelUUIDTwo) == sDisplayToInterfaceMap.end());

		// Store results local to module (for fast access later on)
		sLedInterfaces[new_led_device->mUUID] = new_led_device;
		sDisplayToInterfaceMap[new_led_device->mPanelUUIDOne] = new_led_device;
		sDisplayToInterfaceMap[new_led_device->mPanelUUIDTwo] = new_led_device;

		cout << "Added led interface on port: " << p.port << ", " << p.description << ", device id: "<< new_led_device->mUUID <<", width: " << new_led_device->mStripLength << ", height: " << new_led_device->mLedHeight << "\n";
	}

	// Signal success
	std::cout << "Found: " << sLedInterfaces.size() << " valid LED interfaces\n";

	// Create gamma table
	sGammaTable = new int[256];
	for(int i=0; i<256; i++)
		sGammaTable[i] = (int)(pow((float)i / 255.0, inGammaValue) * 255.0f + 0.5);
}



/**
@brief Closes all serial connections and clears display buffers
**/
void nled::ClearDisplays()
{
	for(auto& v : sLedInterfaces)
	{
		// Close serial connection
		v.second->mConnection.close();
		
		// Delete data buffer
		delete[] v.second->mConvertedData;
	}

	// Clear interfaces
	sLedInterfaces.clear();

	// Clear sampled display numbers
	if(sLedDisplayNumbers != nullptr)
		delete[] sLedDisplayNumbers;

	// Delete gamma table
	delete[] sGammaTable;
}



/**
@brief Returns if a display exists or not
**/
bool nled::DisplayExists(int inDisplayNumber)
{
	int* displays = GetAvailableDisplayNumbers();
	for(int i=0; i < GetDisplayCount(); i++)
	{
		if(displays[i] == inDisplayNumber)
			return true;
	}
	return false;
}



/**
@brief Returns all the available led displays
**/
int nled::GetDisplayCount()
{
	return sLedInterfaces.size() * 2;
}


/**
@brief Returns all the available unique display numbers
**/
int* nled::GetAvailableDisplayNumbers()
{
	if(sLedDisplayNumbers == nullptr)
	{
		sLedDisplayNumbers = GetDisplayCount() > 0 ? new int[GetDisplayCount()] : nullptr;
		
		int count(0);
		for(auto v : sLedInterfaces)
		{
			NLedDevice* current_dev = v.second;
			sLedDisplayNumbers[(count * 2) + 0] = v.second->mPanelUUIDOne;
			sLedDisplayNumbers[(count * 2) + 1] = v.second->mPanelUUIDTwo;
			count++;
		}
	}

	return sLedDisplayNumbers;
}



/**
@brief Returns the display size for the given display number
**/
int nled::GetDisplaySize(int inDisplayNumber)
{
	NLedDevice* found_device = FindLedDevice(inDisplayNumber);

	// Make sure the device was found
	assert(found_device);

	// Return the info
	return found_device == nullptr ? -1 : (found_device->mLedHeight / 2) * found_device->mStripLength;
}


/**
@brief Returns the total amount of bytes for a display
**/
int nled::GetDisplayByteSize(int inDisplayNumber)
{
	NLedDevice* found_device = FindLedDevice(inDisplayNumber);

	// Make sure the device was found
	assert(found_device);

	// Return info
	return found_device == nullptr ? -1 : (found_device->mLedHeight / 2) * found_device->mStripLength * GetBytesPerLed();
}



/**
@brief Returns the max number of bytes relative to the biggest available display
**/
int nled::GetMaxDisplayByteSize()
{
	int max_size(-1);
	for(int i=0; i< GetDisplayCount(); i++)
	{
		int s = GetDisplayByteSize(GetAvailableDisplayNumbers()[i]);
		max_size = s > max_size ? s : max_size;
	}
	return max_size;
}



/**
@brief Returns amount of bytes per led (3 -> RGB)
**/
int nled::GetBytesPerLed()
{
	return sBytesPerLed;
}




/**
@brief Returns the stride (amount of leds on a single connected strip)
**/
int nled::GetDisplayStride(int inDisplayNumber)
{
	NLedDevice* found_device = FindLedDevice(inDisplayNumber);

	// Make sure the device was found
	assert(found_device);

	return found_device == nullptr ? -1 : found_device->mStripLength;
}



/**
@brief Returns the height of the display
**/
int nled::GetDisplayHeight(int inDisplayNumber)
{
	NLedDevice* found_device = FindLedDevice(inDisplayNumber);

	// Make sure the device was found
	assert(found_device);

	return found_device == nullptr ? -1 : found_device->mLedHeight / 2;
}



/**
@brief Returns the total amount of pixels of all displays combined
**/
int nled::GetTotalDisplaySize()
{
	int total_count(0);
	for(auto v : sLedInterfaces)
		total_count += (v.second->mLedHeight * v.second->mStripLength);
	return total_count;
}



/**
@brief Returns the total byte size for all displays
**/
int nled::GetTotalDisplayByteSize()
{
	return GetTotalDisplaySize() * sBytesPerLed;
}



/**
@brief Sets the data for an individual led panel
**/
void nled::SetData(int inDisplayIndex, unsigned char* inData)
{
	// Find the right device to set the data for
	NLedDevice* found_device = FindLedDevice(inDisplayIndex);
	
	// Make sure it's valid
	assert(found_device != nullptr);
	if(found_device == nullptr)
		return;

	// Point panel to right data
	if(found_device->mPanelUUIDOne == inDisplayIndex)
		found_device->mRGBDataPanelOne = inData;
	else
		found_device->mRGBDataPanelTwo = inData;
}



/**
@brief Returns the data pointer for the individual led display
**/
unsigned char* nled::GetData(int inDisplayIndex)
{
	NLedDevice* found_device = FindLedDevice(inDisplayIndex);
	assert(found_device != nullptr);
	
	return found_device->mPanelUUIDOne == inDisplayIndex ? found_device->mRGBDataPanelOne : found_device->mRGBDataPanelTwo;
}



/**
@brief Converts and sends the data in the display buffers to the various devices

Note that this method rund parallel for every device and syncs up before returning
**/
void nled::EndDisplay()
{
	// Create container that will hold threads
	vector<thread> conversion_threads;
	conversion_threads.reserve(sLedInterfaces.size());

	// Start threaded conversion and uploading
	for(auto v : sLedInterfaces)
		conversion_threads.push_back(thread(FlushToDevice, v.second));

	// Sync threads
	for(thread& v: conversion_threads)
		v.join();
}