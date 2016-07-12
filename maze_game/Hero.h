#pragma once
#include <cstring>

//----------------------------------------------------------------------------------------------
// Settings
#define DEFAULT_HERO_SIZE 1

#define DEFAULT_HERO_BODY_RED 1
#define DEFAULT_HERO_BODY_GREEN 0
#define DEFAULT_HERO_BODY_BLUE 0

#define DEFAULT_HERO_HEAD_RED 1
#define DEFAULT_HERO_HEAD_GREEN 0
#define DEFAULT_HERO_HEAD_BLUE 0

#define DEFAULT_HERO_NOSE_RED 1
#define DEFAULT_HERO_NOSE_GREEN 1
#define DEFAULT_HERO_NOSE_BLUE 0

#define FP_CAMERA 0
#define TP_CAMERA 1
#define TD_CAMERA 2

#define DEFAULT_CAMERA TD_CAMERA

//----------------------------------------------------------------------------------------------



class Hero{

public:

	char* name;
	float x,y,z,
		  fx,fy,fz,
		  ux,uy,uz;
	int i, j;
	float size;
	float color_body_r, color_body_g, color_body_b,
		  color_head_r, color_head_g, color_head_b,
		  color_nose_r, color_nose_g, color_nose_b;
	int camera;

	Hero(char* name);
	~Hero(void);
};

