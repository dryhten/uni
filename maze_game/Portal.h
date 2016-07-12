#pragma once


//----------------------------------------------------------------------------------------------
// Settings
#define DEFAULT_HERO_SIZE 1

#define DEFAULT_PORTAL_RED 1
#define DEFAULT_PORTAL_GREEN 1
#define DEFAULT_PORTAL_BLUE 0

//----------------------------------------------------------------------------------------------


class Portal{

public:

	int i,j;
	float x,y,z;
	float color_r, color_g, color_b;

	Portal(void);
	~Portal(void);
};

