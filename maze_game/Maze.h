#pragma once
#include <fstream>
#include <cstdio>
using namespace std;

//----------------------------------------------------------------------------------------------
// Settings

// dim
#define UNIT_SIZE 1

// colors
#define COLOR_BLOCK_R (0.6f) 
#define COLOR_BLOCK_G (0.6f)
#define COLOR_BLOCK_B (0.6f)
#define COLOR_FLOOR_R (0.9f) 
#define COLOR_FLOOR_G (0.6f)
#define COLOR_FLOOR_B (0.3f)

//----------------------------------------------------------------------------------------------

class Maze {

public:
	// the maze = matrix of chars MxN
	char** maze;
	int M, N;
	float size;

	// colors
	float color_br, color_bg, color_bb,
		  color_fr, color_fg, color_fb;


	Maze(void);
	~Maze(void);
};

