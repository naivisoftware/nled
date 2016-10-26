#pragma once

/**
@brief Low Level C Style interface to send LED data over serial ports
**/

//////////////////////////////////////////////////////////////////////////

namespace nled
{
	//////////////////////////////////////////////////////////////////////////
	// Initialization
	//////////////////////////////////////////////////////////////////////////

	/**
	Initializes all the available LED interfaces
	**/
	void InitDisplays(float inGammaValue);

	/**
	@brief Clears all available displays
	**/
	void ClearDisplays();

	/**
	@brief Returns if a display exists or not
	**/
	bool DisplayExists(int inDisplayNumber);

	//////////////////////////////////////////////////////////////////////////
	// Display Controls
	//////////////////////////////////////////////////////////////////////////

	/**
	@brief Returns the number of available LED panels
	**/
	int GetDisplayCount();

	/**
	@brief Returns the number of panel pixels, -1 if display isn't valid
	**/
	int GetDisplaySize(int inDisplayNumber);

	/**
	@brief Returns the number of bytes for a panel, -1 id display number isn't valid
	**/
	int GetDisplayByteSize(int inDisplayNumber);

	/**
	@brief Returns the max number of bytes for all available displays
	**/
	int GetMaxDisplayByteSize();

	/**
	@brief Returns the amount of bytes per led (3 -> RGB)
	**/
	int GetBytesPerLed();

	/**
	@brief Returns the stride of the display, -1 if display number isn't valid
	**/
	int GetDisplayStride(int inDisplayNumber);

	/**
	@brief Returns the height of the display
	**/
	int GetDisplayHeight(int inDisplayNumber);

	/**
	@brief Returns the total amount of pixels
	**/
	int GetTotalDisplaySize();

	/**
	@brief Returns the total led buffer size
	**/
	int GetTotalDisplayByteSize();

	/**
	@brief Returns all the available unique display numbers

	The length of the array = GetDisplayCount(), where the pointer points to the first element in the array
	Note that this function is only valid after calling InitDisplays
	**/
	int* GetAvailableDisplayNumbers();

	//////////////////////////////////////////////////////////////////////////
	// Sending
	//////////////////////////////////////////////////////////////////////////

	/**
	@brief Sets the data for a single display
	**/
	void SetData(int inDisplayIndex, unsigned char* inData);

	/**
	@brief Returns the data for a single display
	**/
	unsigned char* GetData(int inDisplayIndex);

	/**
	@brief Converts display data and sends it to the hardware
	**/
	void EndDisplay();
}

