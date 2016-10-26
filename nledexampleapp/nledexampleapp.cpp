// nledexampleapp.cpp : Defines the entry point for the console application.
//

// Standard h
#include "stdafx.h"

// Led lib
#include <nled.h>

// std libs
#include <iostream>
#include <map>
#include <time.h>
#include <string>
#include <math.h>

// namespacce
using namespace std;

// typedefs
typedef map<int, unsigned char*> LedColorMap;

//////////////////////////////////////////////////////////////////////////
// Simple application that demonstrates how to use the nled library
//
// All available led interfaces are auto found and initialized
// For every available led display a new pixel (RGB) buffer is created
// UpdateLedDisplays simply fills the buffer with color data and sends it over
//////////////////////////////////////////////////////////////////////////

/**
@brief Changes the colors of the panels using a gradient over x
**/
void UpdateLedDisplays(const LedColorMap& inColorBuffers)
{
	// Notify led interface we're going to send data over
	nled::BeginDisplay();
	std::cout<< "\n";

	// send some color info
	LedColorMap map;
	for(auto d : inColorBuffers)
	{
		int display_id = d.first;
		int display_he = nled::GetDisplayHeight(d.first);
		int display_wi = nled::GetDisplayStride(d.first);

		// Get the pointer to the temp display data
		unsigned char* display_data = d.second;

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
				int   index = (yoff + x) * nled::GetBytesPerLed();
				float scale =  pow(((float)x / (float)(display_wi-1)), 1.0f);

				display_data[index + 0] = cr * scale;
				display_data[index + 1] = cg * scale;
				display_data[index + 2] = cb * scale;
			}
		}

		// Set the data for the display
		nled::SetData(display_id, display_data);
	}

	// Notify the system we're done setting the led data > sends it over
	nled::EndDisplay();
}



/**
@brief Main compute function
**/
int _tmain(int argc, _TCHAR* argv[])
{
	// Initialize random function
	srand(time(nullptr));

	// Initialize all possible led displays
	nled::InitDisplays(1.75f);

	// Get all available displays
	int* display_numbers = nled::GetAvailableDisplayNumbers();
	int  display_count = nled::GetDisplayCount();
	std::cout << "\n";

	// Show some display info and create pixel buffers
	LedColorMap display_buffers;
	
	for(int i=0; i<display_count; i++)
	{
		int display_id = display_numbers[i];
		std::cout << "Available Display: " << display_id << "\n";
		std::cout << "Display Width: "  << nled::GetDisplayStride(display_id) << "\n";
		std::cout << "Display Height: " << nled::GetDisplaySize(display_id) / nled::GetDisplayStride(display_id) << "\n";
		std::cout << "\n";

		// Create display buffer
		display_buffers[display_id] = new unsigned char[nled::GetDisplayByteSize(display_id)];
	}

	// On any key send pixel data to individual panels
	while(true)
	{
		std::cout << "Press any key to change display colors, q to quit";
		std::string return_line;
		getline(std::cin, return_line);
		if(return_line == "q")
			break;

		UpdateLedDisplays(display_buffers);
	}

	// Clear all the display data for our current displays
	for(auto v : display_buffers)
		delete[] v.second;

	// Close all displays (cutting communication)
	nled::ClearDisplays();

	return 0;
}

