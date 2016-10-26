#pragma once

/**
@brief LedGetConfig

Returns the total number of available led panels as an int.
After that: the panel id (unique identifier), total number of leds, total buffer size,
height and width. 

The data would look like this: [(total)6][2,900, 2700, 30, 30][3,900, 2700, 30, 30]..[]
**/
#define NLED_ID_GETCONFIG 0		

/**
@brief Allows setting of display specific data
After calling the command the server expects a unique panel identifier and an unsigned char array
Syntax: [PanelIdx;][Display Data]
**/
#define NLED_ID_DRAW_PANEL 1	

/**
@brief Allows setting of data for all led displays in one go

This function will cycle over every unique display id (in order as received by LedGetConfig).
The total amount of bytes this command pulls is similar to the entire byte buffer size.
**/
#define NLED_ID_DRAW_ALL 2		

/**
@brief Call this to flush the set display data to led hardware
**/
#define NLED_ID_FLUSH 3	

/**
@brief Sets debug mode time out (0 = off, 1 = on)
**/
#define NLED_SET_DEBUG_MODE 4