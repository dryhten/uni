//-----------------------------------------------------------------------------------------------
//	A3 CG - Labyrinth
//	Mihail Dunaev, 331CC
//	
//	Simple game where you have to find your way out of a maze. 
//
//	main.cpp - main source code
//
//----------------------------------------------------------------------------------------------

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glut32.lib")

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <windows.h>
#include <time.h>
#include <stack>
using namespace std;

#include "glut.h"
#include "Maze.h"
#include "Hero.h"
#include "Portal.h"

// settings
#define PI 3.14159
#define KEY_ENTER 13
#define KEY_ESC 27
int windowWidth = 800,
	windowHeight = 600,
	windowStartX = 200,
	windowStartY = 200; 
const char* windowName = "Labyrinth";

// menus
#define DEFAULT_LIST_SIZE 11
#define PLAYING 0
#define MULTIPLAYING 1
#define MAIN_MENU 2
#define SINGLE_PLAYER_MENU 3
#define MULTIPLAYER_MENU 4
#define HIGHSCORES_BIG_MENU 5
#define SETTINGS_MENU 6
#define CAMPAIGN_MENU 7
#define CUSTOM_MAPS_MENU 8
#define RANDOM_MAP_MENU 9
#define MULTIPLAYER_CUSTOM_MENU 10
#define MULTIPLAYER_RANDOM_MENU 11
#define HERO_MENU 12
#define COLORS_MENU 13
#define SP_CONTROLS 14
#define MP_CONTROLS 15
#define HIGHSCORES_SMALL_MENU 16

// selection
#define NONE 0
#define SINGLE_PLAYER 1
#define MULTIPLAYER 2
#define HIGHSCORES 3
#define SETTINGS 4
#define EXIT 5
#define CAMPAIGN 1
#define CUSTOM_MAPS 2
#define RANDOM_MAP 3
#define MULTIPLAYER_CUSTOM 1
#define MULTIPLAYER_RANDOM 2

// animation
#define MOVE_FORWARD 0
#define MOVE_BACKWARD 1
#define ROTATE_LEFT 2
#define ROTATE_RIGHT 3
int even;
float total, progress, destx, destz;

// global vars - general
Maze *m;
Hero *h1, *h2;
char *heroname1, *heroname2, *current_map_name, *current_map_source;
Portal *p;
int level,
	menu = MAIN_MENU,
	selected = NONE;
bool light = true, wait = false, finished_campaign = false;
long int time_start, time_finish;
int mainWindow, subWindow1, subWindow2;

// campaign
int start_campaign = 0;
bool *locked_level;
int nlevels;
char **level_names;

// custom maps && multiplayer custom && highscores
int start_custom = 0;
int nmaps;
char** custom_map_name,
	** custom_map_source,
	 * custom_map_dir;

// highscores entries
int nplayers;
int *times;
char **players;

// random map
#define DEFAULT_M 30
#define DEFAULT_N 30
#define EASY_DIFFICULTY 0
#define MEDIUM_DIFFICULTY 1
#define HARD_DIFFICULTY 2
#define HARDEST_DIFFICULTY 3
#define DEFAULT_DIFFICULTY HARD_DIFFICULTY
int randm = 30, randn = 30,
	rand_difficulty = DEFAULT_DIFFICULTY;

// multiplayer
#define DEFAULT_MULTIPLAYER_CAMERA TP_CAMERA
#define PLAYER_1 0
#define PLAYER_2 1
int mp_player;

#define BACKGROUND_RED 0
#define BACKGROUND_GREEN 0
#define BACKGROUND_BLUE 0
#define BACKGROUND_ALPHA 1

// functions
void readMap(char* filename);
void readCampaign(void);
void readCustomMaps(char* dir);
void readHighScores(void);
void readHighscoreMap(char* filename);
void generateRandomMap(int M, int N, int difficulty);
void generateRandomMultiplayerMap(int M, int N, int difficulty);
void init(int* argc, char *argv[]); // initializations, bindings + reading from files
void initw(void); // callbacks for subwindows
void mapd2v(int i, int j, float *x, float *y, float *z); // helper function : converting 2D document positions to 3D view positions
void displayText(int x, int y, void* font, char* text);
void display(void);
void displayw1(void); // display for subwindow 1
void displayw2(void); // display for subwindow 2
void setView(Hero *h);
void drawMaze(void);
void drawFloor(void);
void drawHero(Hero *h);
void drawPortal(void);
void drawMainMenu(void);
void drawSinglePlayerMenu(void);
void drawCampaignMenu(void);
void drawCustomMapsMenu(void);
void drawRandomMapMenu(void);
void drawMultiplayerMenu(void);
void drawHighscores(void);
void drawSettingsMenu(void);
void drawHeroMenu(void);
void drawColorsMenu(void);
void drawSPControlsMenu(void);
void drawMPControlsMenu(void);
void displayHighscore(void);
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void keyboardsp(int key, int x, int y);
void idle(void);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void singlePlayerVictory(void);
void multiplayerVictory(void);

int main (int argc, char *argv[]){

	init(&argc, argv);
	generateRandomMultiplayerMap(20,20,60);
	glutMainLoop();

	return 0;
}

void init (int* argc, char *argv[]){

	//init glut
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	//init window
	glutInitWindowSize(windowWidth,windowHeight);
	glutInitWindowPosition(windowStartX,windowStartY);
	mainWindow = glutCreateWindow(windowName);

	//callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardsp);
	glutIdleFunc(idle);

	// lights
	const GLfloat globalAmbientColor4f [] = {0.2, 0.2, 0.2, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientColor4f);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);

	// white light at inf
	GLfloat position4f [] = {1.0, 1.0, 0.5, 0.0};
	GLfloat color4f [] = {1.0, 1.0, 1.0, 1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, color4f);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, color4f);
	glLightfv(GL_LIGHT0, GL_POSITION, position4f);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	//set background
	glClearColor(BACKGROUND_RED,BACKGROUND_GREEN,BACKGROUND_BLUE,BACKGROUND_ALPHA);

	// some ds init
	m = new Maze();
	heroname1 = "Unnamed Hero";
	heroname2 = "Unnamed Hero 2";
	h1 = new Hero(heroname1);
	h2 = new Hero(heroname2);
	h2->color_body_r = 0;
	h2->color_body_g = 0;
	h2->color_body_b = 1;
	h2->color_head_r = 0;
	h2->color_head_g = 0;
	h2->color_head_b = 1;
	p = new Portal();
}

void initw (void){

	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardsp);
	glutIdleFunc(idle);

	// lights
	const GLfloat globalAmbientColor4f [] = {0.2, 0.2, 0.2, 1.0};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientColor4f);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);

	// white light at inf
	GLfloat position4f [] = {1.0, 1.0, 0.5, 0.0};
	GLfloat color4f [] = {1.0, 1.0, 1.0, 1.0};
	glLightfv(GL_LIGHT0, GL_AMBIENT, color4f);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, color4f);
	glLightfv(GL_LIGHT0, GL_POSITION, position4f);
	glEnable(GL_LIGHT0);

	glEnable(GL_DEPTH_TEST);

	glClearColor(BACKGROUND_RED,BACKGROUND_GREEN,BACKGROUND_BLUE,BACKGROUND_ALPHA);
}

void displayw1 (void){

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	setView (h1);
	drawMaze ();
	drawFloor ();
	drawHero (h1);
	drawHero (h2);
	drawPortal ();

	glutSwapBuffers();
}

void displayw2 (void){

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	setView (h2);
	drawMaze ();
	drawFloor ();
	drawHero (h1);
	drawHero (h2);
	drawPortal ();

	glutSwapBuffers();
}

void display(){

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (menu == MAIN_MENU)
		drawMainMenu();

	else if (menu == SINGLE_PLAYER_MENU)
		drawSinglePlayerMenu();

	else if (menu == CAMPAIGN_MENU)
		drawCampaignMenu();

	else if (menu == CUSTOM_MAPS_MENU)
		drawCustomMapsMenu();

	else if (menu == RANDOM_MAP_MENU)
		drawRandomMapMenu();

	else if (menu == MULTIPLAYER_MENU)
		drawMultiplayerMenu();

	else if (menu == HIGHSCORES_BIG_MENU)
		drawHighscores();

	else if (menu == SETTINGS_MENU)
		drawSettingsMenu();

	else if (menu == MULTIPLAYER_CUSTOM_MENU)
		drawCustomMapsMenu();

	else if (menu == MULTIPLAYER_RANDOM_MENU) // might change
		drawRandomMapMenu();

	else if (menu == HERO_MENU)
		drawHeroMenu();

	else if (menu == COLORS_MENU)
		drawColorsMenu();

	else if (menu == SP_CONTROLS)
		drawSPControlsMenu();

	else if (menu == MP_CONTROLS)
		drawMPControlsMenu();

	else if (menu == HIGHSCORES_SMALL_MENU)
		displayHighscore();

	else if (menu == PLAYING){
		setView(h1);
		drawMaze();
		drawFloor();
		drawHero(h1);
		drawPortal();
	}

	else if (menu == MULTIPLAYING){
		//
	}

	glutSwapBuffers();
}

void reshape(int width, int height){

	glViewport(0,0,width,height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45,(float)width/(float)height,0.01,100);
}

void keyboard(unsigned char key, int x, int y){

	if (menu == MAIN_MENU){

		switch(key){
		case 'w':
			if (selected == NONE)
				selected = EXIT;
			else
				selected--;
			glutPostRedisplay();
			break;
		case 's':
			if (selected == EXIT)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		case KEY_ENTER:
			if (selected == SINGLE_PLAYER){
				menu = SINGLE_PLAYER_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == MULTIPLAYER){
				menu = MULTIPLAYER_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == HIGHSCORES){
				readHighScores();
				menu = HIGHSCORES_BIG_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == SETTINGS){
				menu = SETTINGS_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == EXIT)
				exit(0);
			break;
		case KEY_ESC:
			exit(0);
			break;
		default:
			break;
		}
	}
	else if (menu == SINGLE_PLAYER_MENU){
		switch(key){
		case 'w':
			if (selected == NONE)
				selected = RANDOM_MAP;
			else
				selected--;
			glutPostRedisplay();
			break;
		case 's':
			if (selected == RANDOM_MAP)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		case KEY_ESC: 
			menu = MAIN_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER: 
			if (selected == CAMPAIGN){
				readCampaign();
				selected = NONE;
				menu = CAMPAIGN_MENU;
				glutPostRedisplay();
			}
			else if (selected == CUSTOM_MAPS){
				readCustomMaps("cmaps");
				selected = NONE;
				menu = CUSTOM_MAPS_MENU;
				glutPostRedisplay();
			}
			else if (selected == RANDOM_MAP){
				selected = NONE;
				menu = RANDOM_MAP_MENU;
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == CAMPAIGN_MENU){
		switch(key){
		case KEY_ESC:
			menu = SINGLE_PLAYER_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER:
			if (selected != NONE){
				// read data
				level = selected + start_campaign - 1;
				char filename[100];
				sprintf(filename, "levels\\level%d.txt", level);
				current_map_name = new char[20];
				sprintf(current_map_name,"Level %d", level);
				current_map_source = new char[20];
				sprintf(current_map_source, "level%d.txt", level);
				readMap(filename);

				time_start = (long int)time(NULL);
				menu = PLAYING;
				selected = NONE;
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == CUSTOM_MAPS_MENU){
		switch(key){
		case KEY_ESC: 
			// free memory
			start_custom = 0;
			delete(custom_map_name); // lulz
			delete(custom_map_source);
			nmaps = 0;

			menu = SINGLE_PLAYER_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER:{ 
			if (selected != NONE){
				// read data
				level = -1;
				int mapindex = selected + start_custom -1;
				char filename [100];
				sprintf(filename, "cmaps\\%s",custom_map_source[mapindex]);
				current_map_name = new char[20];
				current_map_source = new char[20];
				sprintf(current_map_name,"%s", custom_map_name[mapindex]);
				sprintf(current_map_source,"%s", custom_map_source[mapindex]);
				readMap(filename);

				time_start = (long int)time(NULL);
				menu = PLAYING;
				selected = NONE;
				glutPostRedisplay();
			}
		}
			break;
		default:
			break;
		}
	}
	else if (menu == RANDOM_MAP_MENU){
		switch(key){
		case KEY_ESC:
			menu = SINGLE_PLAYER_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay ();
			break;
		case KEY_ENTER: 
			if (selected == 4){ // start ...
				if (rand_difficulty == EASY_DIFFICULTY)
					generateRandomMap(randm,randn,10);
				else if (rand_difficulty == MEDIUM_DIFFICULTY)
					generateRandomMap(randm,randn,randm);
				else if (rand_difficulty == HARD_DIFFICULTY)
					generateRandomMap(randm,randn,(randm*randn)/5);
				else if (rand_difficulty == HARDEST_DIFFICULTY)
					generateRandomMap(randm,randn,(randm*randn)/2 - (3*randm));
				level = -2;
				selected = NONE;
				menu = PLAYING;
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_MENU){
		switch(key){
		case KEY_ESC:
			menu = MAIN_MENU;
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER:
			if (selected == MULTIPLAYER_CUSTOM){
				readCustomMaps("mmaps");
				menu = MULTIPLAYER_CUSTOM_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == MULTIPLAYER_RANDOM){
				menu = MULTIPLAYER_RANDOM_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_CUSTOM_MENU){
		switch(key){
		case KEY_ESC:
			// free memory
			start_custom = 0;
			delete (custom_map_name); // lulz
			delete (custom_map_source);
			nmaps = 0;

			menu = MULTIPLAYER_MENU;
			selected = NONE;
			glutPostRedisplay ();
			break;
		case KEY_ENTER:
		{ 
			if (selected != NONE){
				
				// read data
				int mapindex = selected + start_custom -1;
				char filename [100];
				sprintf(filename, "mmaps\\%s",custom_map_source[mapindex]);
				readMap(filename);

				// set cams
				h1->camera = DEFAULT_MULTIPLAYER_CAMERA;
				h2->camera = DEFAULT_MULTIPLAYER_CAMERA;

				menu = MULTIPLAYING;
				selected = NONE;
				glutPostRedisplay(); // make it black 

				// create 2 windows -- seems OK
				subWindow1 = glutCreateSubWindow(mainWindow,0,0,windowWidth/2 - 1,windowHeight - 1);
				glutSetWindow(subWindow1);
				glutDisplayFunc(displayw1);
				initw();
				glutPostRedisplay();

				subWindow2 = glutCreateSubWindow(mainWindow,windowWidth/2 + 1,0,windowWidth/2 - 1,windowHeight - 1);
				glutSetWindow(subWindow2);
				glutDisplayFunc(displayw2);
				initw();
				glutPostRedisplay();
			}
		}
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_RANDOM_MENU){
		switch(key){
		case KEY_ESC: 
			menu = MULTIPLAYER_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER: 
			if (selected == 4){ // start ...
				if (rand_difficulty == EASY_DIFFICULTY)
					generateRandomMultiplayerMap(randm,randn,10);
				else if (rand_difficulty == MEDIUM_DIFFICULTY)
					generateRandomMultiplayerMap(randm,randn,randm);
				else if (rand_difficulty == HARD_DIFFICULTY)
					generateRandomMultiplayerMap(randm,randn,(randm*randn)/5);
				else if (rand_difficulty == HARDEST_DIFFICULTY)
					generateRandomMultiplayerMap(randm,randn,(randm*randn)/2 - (3*randm));

				selected = NONE;
				menu = MULTIPLAYING;
				glutPostRedisplay();

				// set cams
				h1->camera = DEFAULT_MULTIPLAYER_CAMERA;
				h2->camera = DEFAULT_MULTIPLAYER_CAMERA;

				// create 2 windows -- seems OK
				subWindow1 = glutCreateSubWindow(mainWindow,0,0,windowWidth/2 - 1,windowHeight - 1);
				glutSetWindow(subWindow1);
				glutDisplayFunc(displayw1);
				initw();
				glutPostRedisplay();

				subWindow2 = glutCreateSubWindow(mainWindow,windowWidth/2 + 1,0,windowWidth/2 - 1,windowHeight - 1);
				glutSetWindow(subWindow2);
				glutDisplayFunc(displayw2);
				initw();
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == SETTINGS_MENU){
		switch(key){
		case KEY_ESC: 
			menu = MAIN_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		case KEY_ENTER:
			if (selected == 1){
				menu = HERO_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == 2){
				menu = COLORS_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == 3){
				menu = SP_CONTROLS;
				selected = NONE;
				glutPostRedisplay();
			}
			else if (selected == 4){
				menu = MP_CONTROLS;
				selected = NONE;
				glutPostRedisplay();
			}
			break;
		default:
			break;
		}
	}
	else if (menu == HERO_MENU || menu == COLORS_MENU || menu == SP_CONTROLS || menu == MP_CONTROLS){
		switch(key){
		case KEY_ESC:
			menu = SETTINGS_MENU;
			selected = NONE;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == HIGHSCORES_BIG_MENU){
		switch(key){
		case KEY_ESC: 
			menu = MAIN_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay ();
			break;
		case KEY_ENTER:{ 
			if (selected != NONE){
				// read data
				int mapindex = selected + start_custom -1;
				char filename [100];
				sprintf(filename, "highscores\\%s",custom_map_source[mapindex]);
				readHighscoreMap(filename);

				menu = HIGHSCORES_SMALL_MENU;
				selected = NONE;
				glutPostRedisplay();
			}
		}
			break;
		default:
			break;
		}
	}
	else if (menu == HIGHSCORES_SMALL_MENU){
		switch(key){
		case KEY_ESC:
		
			// free memory
			// wut wut :/
			nplayers = 0;
			delete(times);
			delete(players);

			menu = HIGHSCORES_BIG_MENU; // back_menu
			selected = NONE;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if ((menu == PLAYING) && (wait == false)){
		switch(key){
		case KEY_ESC:

			// free mem
			delete(m->maze);

			menu = MAIN_MENU;
			selected = 0;
			glutPostRedisplay();
			break;
			
		case 'w': {

			int newi, newj;
			newi = h1->i + (int)h1->fz;
			newj = h1->j + (int)h1->fx;
			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h1->i][h1->j] = '0';
					m->maze[newi][newj] = 'h';
					h1->i = newi;
					h1->j = newj;

					wait = true;
					even = MOVE_FORWARD;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p')
					singlePlayerVictory();
			}
		}
			break;
		case 's': 
			{
			int newi, newj;
			newi = h1->i - (int)h1->fz;
			newj = h1->j - (int)h1->fx;

			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h1->i][h1->j] = '0';
					m->maze[newi][newj] = 'h';
					h1->i = newi;
					h1->j = newj;

					wait = true;
					even = MOVE_BACKWARD;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p')
					singlePlayerVictory();

			}
		}
			break;
		case 'a':
			{ 
				destx = h1->fz; 
				destz = -1 * h1->fx;
				wait = true;
				even = ROTATE_LEFT;
				total = PI/2;
				progress = 0;
			}
			break;
		case 'd':
			{ 
				destx = -1 * h1->fz; 
				destz = h1->fx;
				wait = true;
				even = ROTATE_RIGHT;
				total = PI/2;
				progress = 0;
			}
			break;
		case 'l':
			light = !light;
			if (light == true)
				glEnable (GL_LIGHT0);
			else
				glDisable (GL_LIGHT0);
			glutPostRedisplay();
			break;
		case 'c': 
			if (h1->camera == TD_CAMERA) // LAST_CAMERA, FIRST_
				h1->camera = FP_CAMERA;
			else
				h1->camera++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if ((menu == MULTIPLAYING) && (wait == false)){
		switch(key){
		case KEY_ESC:

			// free mem
			delete (m->maze);

			// delete windows
			glutDestroyWindow(subWindow1);
			glutDestroyWindow(subWindow2);
			
			menu = MAIN_MENU;
			selected = 0;
			glutSetWindow(mainWindow);
			glutPostRedisplay();
			break;
			
		case 'w': { // h1 forward

			int newi, newj;
			newi = h1->i + (int)h1->fz;
			newj = h1->j + (int)h1->fx;

			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h1->i][h1->j] = '0';
					m->maze[newi][newj] = 'h';
					h1->i = newi;
					h1->j = newj;

					wait = true;
					even = MOVE_FORWARD;
					mp_player = PLAYER_1;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p'){
					mp_player = PLAYER_1;
					multiplayerVictory();
				}
			}
		}
			break;
		case 's':
		{
			int newi, newj;
			newi = h1->i - (int)h1->fz;
			newj = h1->j - (int)h1->fx;
			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h1->i][h1->j] = '0';
					m->maze[newi][newj] = 'h';
					h1->i = newi;
					h1->j = newj;

					wait = true;
					even = MOVE_BACKWARD;
					mp_player = PLAYER_1;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p'){
					mp_player = PLAYER_1;
					multiplayerVictory();
				}
			}
		}
			break;
		case 'a':
			{ 
				destx = h1->fz; 
				destz = -1 * h1->fx;
				wait = true;
				even = ROTATE_LEFT;
				mp_player = PLAYER_1;
				total = PI/2;
				progress = 0;
			}
			break;
		case 'd':
			{ 
				destx = -1 * h1->fz; 
				destz = h1->fx;
				wait = true;
				even = ROTATE_RIGHT;
				mp_player = PLAYER_1;
				total = PI/2;
				progress = 0;
			}
			break;
		case '8': { // h2 forward
			int newi, newj;
			newi = h2->i + (int)h2->fz;
			newj = h2->j + (int)h2->fx;
			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h2->i][h2->j] = '0';
					m->maze[newi][newj] = 'h';
					h2->i = newi;
					h2->j = newj;

					wait = true;
					even = MOVE_FORWARD;
					mp_player = PLAYER_2;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p'){
					mp_player = PLAYER_2;
					multiplayerVictory();
				}

			}
		}
			break;
		case '5':
		{
			int newi, newj;
			newi = h2->i - (int)h2->fz;
			newj = h2->j - (int)h2->fx;
			if ((newi >= 0) && (newi < m->M) && (newj >= 0) && (newj < m->N)){
				if (m->maze[newi][newj] == '0'){
					m->maze[h2->i][h2->j] = '0';
					m->maze[newi][newj] = 'h';
					h2->i = newi;
					h2->j = newj;

					wait = true;
					even = MOVE_BACKWARD;
					mp_player = PLAYER_2;
					total = m->size;
					progress = 0;
				}
				else if (m->maze[newi][newj] == 'p'){
					mp_player = PLAYER_2;
					multiplayerVictory();
				}
			}
		}
			break;
		case '4':
			{ 
				destx = h2->fz; 
				destz = -1 * h2->fx;
				wait = true;
				even = ROTATE_LEFT;
				mp_player = PLAYER_2;
				total = PI/2;
				progress = 0;
			}
			break;
		case '6':
			{ 
				destx = -1 * h2->fz; 
				destz = h2->fx;
				wait = true;
				even = ROTATE_RIGHT;
				mp_player = PLAYER_2;
				total = PI/2;
				progress = 0;
			}
			break;
		case 'l':
			light = !light;
			if (light == true){
				glutSetWindow(subWindow1);
				glEnable(GL_LIGHT0);				
				glutSetWindow(subWindow2);
				glEnable(GL_LIGHT0);
			}
			else{
				glutSetWindow(subWindow1);
				glDisable(GL_LIGHT0);				
				glutSetWindow(subWindow2);
				glDisable(GL_LIGHT0);
			}
			glutSetWindow(subWindow1);
			glutPostRedisplay();
			glutSetWindow(subWindow2);
			glutPostRedisplay();
			break;
		case 'c': 
			if (h1->camera == TD_CAMERA){ // LAST_CAMERA, FIRST_
				h1->camera = FP_CAMERA;
				h2->camera = FP_CAMERA;
			}
			else{
				h1->camera++;
				h2->camera++;
			}
			glutSetWindow(subWindow1);
			glutPostRedisplay();
			glutSetWindow(subWindow2);
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
}

void keyboardsp (int key, int x, int y){

	if (menu == MAIN_MENU){
		switch(key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = EXIT;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == EXIT)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == SINGLE_PLAYER_MENU){
		switch(key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = RANDOM_MAP;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == RANDOM_MAP)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == CAMPAIGN_MENU){
		switch(key){
		case GLUT_KEY_UP:{
			int min = nlevels - start_campaign;
			if (min > DEFAULT_LIST_SIZE) 
				min = DEFAULT_LIST_SIZE;
			if (selected == 1){
				if (start_campaign > 0)
					start_campaign--;
				else
					selected = NONE; // --
			}
			else if (selected == NONE){
				for (int i = (nlevels - 1); i >=0; i--)
					if (locked_level[i] == false){
						start_campaign = i - min + 1;
						if (start_campaign < 0)
							start_campaign = 0;
						selected = i - start_campaign + 1;
						break;
					}
			}
			else
				selected--;
			glutPostRedisplay();
		}
			break;
		case GLUT_KEY_DOWN:{
			int min = nlevels - start_campaign;
			if (min > DEFAULT_LIST_SIZE)
				min = DEFAULT_LIST_SIZE;
			if (selected == min){ // am ajuns jos de tot
				if ((start_campaign + min) == nlevels){
					selected = NONE;
					start_campaign = 0;
				}
				else{
					start_campaign++;
					selected--;
				}
			}
			else{
				if (locked_level[selected + start_campaign] == false)
					selected++;
				else{
					selected = NONE;
					start_campaign = 0;
				}
			}
			glutPostRedisplay();
		}
			break;
		default:
			break;
		}
	}
	else if (menu == CUSTOM_MAPS_MENU){
		switch(key){
		case GLUT_KEY_UP:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE) 
				min = DEFAULT_LIST_SIZE;
			if (selected == 1){
				if (start_custom > 0)
					start_custom--;
				else
					selected = NONE; // --
			}
			else if (selected == NONE){
				start_custom = nmaps - min;
				if (start_custom < 0)
					start_custom = 0;
				selected = min;
			}
			else
				selected--;
			glutPostRedisplay();
		}
			break;
		case GLUT_KEY_DOWN:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE)
				min = DEFAULT_LIST_SIZE;
			if (selected == min){ // we reached bottom
				if ((start_custom + min) == nmaps){
					selected = NONE;
					start_custom = 0;
				}
				else
					start_custom++;
			}
			else
				selected++;
			glutPostRedisplay();
		}
			break;
		default:
			break;
		}
	}
	else if (menu == RANDOM_MAP_MENU){
		switch (key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = 4;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == 4)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_MENU){
		switch(key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = MULTIPLAYER_RANDOM;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == MULTIPLAYER_RANDOM)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_CUSTOM_MENU){
		switch(key){
		case GLUT_KEY_UP:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE) 
				min = DEFAULT_LIST_SIZE;
			if (selected == 1){
				if (start_custom > 0)
					start_custom--;
				else
					selected = NONE; // --
			}
			else if (selected == NONE){
				start_custom = nmaps - min;
				if (start_custom < 0)
					start_custom = 0;

				selected = min;
			}
			else
				selected--;
			glutPostRedisplay();
		}
			break;
		case GLUT_KEY_DOWN:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE)
				min = DEFAULT_LIST_SIZE;
			
			if (selected == min){ // we reached bottom
				if ((start_custom + min) == nmaps){
					selected = NONE;
					start_custom = 0;
				}
				else
					start_custom++;
			}
			else
				selected++;
			glutPostRedisplay();
		}
			break;
		default:
			break;
		}
	}
	else if (menu == MULTIPLAYER_RANDOM_MENU){
		switch(key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = 4;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == 4)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == SETTINGS_MENU){
		switch(key){
		case GLUT_KEY_UP:
			if (selected == NONE)
				selected = 4;
			else
				selected--;
			glutPostRedisplay();
			break;
		case GLUT_KEY_DOWN:
			if (selected == 4)
				selected = NONE;
			else
				selected++;
			glutPostRedisplay();
			break;
		default:
			break;
		}
	}
	else if (menu == HIGHSCORES_BIG_MENU){
		switch(key){
		case GLUT_KEY_UP:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE) 
				min = DEFAULT_LIST_SIZE;
			if (selected == 1){
				if (start_custom > 0)
					start_custom--;
				else
					selected = NONE; // --
			}
			else if (selected == NONE){
				start_custom = nmaps - min;
				if (start_custom < 0)
					start_custom = 0;
				selected = min;
			}
			else
				selected--;
			glutPostRedisplay();
		}
			break;
		case GLUT_KEY_DOWN:{
			int min = nmaps - start_custom;
			if (min > DEFAULT_LIST_SIZE)
				min = DEFAULT_LIST_SIZE;

			if (selected == min){ // we reached bottom
				if ((start_custom + min) == nmaps){
					selected = NONE;
					start_custom = 0;
				}
				else
					start_custom++;
			}
			else
				selected++;
			glutPostRedisplay();
		}
			break;
		default:
			break;
		}
	}
}


void idle(){
	
	if ((wait == true) && (menu == PLAYING)){
		if (even == MOVE_FORWARD){
			float delta = (m->size)/100;
			progress = progress + delta;

			if (progress < total){
				h1->x = h1->x + h1->fx*delta;
				h1->z = h1->z + h1->fz*delta;
			}
			else{
				progress = progress - delta;
				delta = total - progress;
				h1->x = h1->x + h1->fx*delta;
				h1->z = h1->z + h1->fz*delta;
				wait = false;
			}
		}
		else if (even == MOVE_BACKWARD){
			float delta = (m->size)/100;
			progress = progress + delta;
			if (progress < total){
				h1->x = h1->x - h1->fx*delta;
				h1->z = h1->z - h1->fz*delta;
			}
			else{
				progress = progress - delta;
				delta = total - progress;
				h1->x = h1->x - h1->fx*delta;
				h1->z = h1->z - h1->fz*delta;
				wait = false;
			}
		}
		else if (even == ROTATE_LEFT){
			float oldfx = h1->fx, oldfz = h1->fz,
				  delta = 0.01;
			progress = progress + delta;
			if (progress < total){
				h1->fx = (oldfx * cos(delta)) + (oldfz * sin(delta));
				h1->fz = (oldfz * cos(delta)) - (oldfx * sin(delta));
			}
			else{
				h1->fx = destx;
				h1->fz = destz;
				wait = false;
			}
		}
		else if (even == ROTATE_RIGHT){
			float oldfx = h1->fx, oldfz = h1->fz,
				  delta = 0.01;
			progress = progress + delta;
			if (progress < total){
				h1->fx = (oldfx * cos(delta)) - (oldfz * sin(delta));
				h1->fz = (oldfz * cos(delta)) + (oldfx * sin(delta));
			}
			else{
				h1->fx = destx;
				h1->fz = destz;
				wait = false;
			}
		}
		glutPostRedisplay();
	}
	else if ((wait == true) && (menu == MULTIPLAYING)){

		Hero *h;
		if (mp_player == PLAYER_1)
			h = h1;
		else if (mp_player == PLAYER_2)
			h = h2;

		if (even == MOVE_FORWARD){
			float delta = (m->size)/100;
			progress = progress + delta;

			if (progress < total){
				h->x = h->x + h->fx*delta;
				h->z = h->z + h->fz*delta;
			}
			else{
				progress = progress - delta;
				delta = total - progress;
				h->x = h->x + h->fx*delta;
				h->z = h->z + h->fz*delta;
				wait = false;
			}
		}
		else if (even == MOVE_BACKWARD){
			float delta = (m->size)/100;
			progress = progress + delta;
			
			if (progress < total){
				h->x = h->x - h->fx*delta;
				h->z = h->z - h->fz*delta;
			}
			else{
				progress = progress - delta;
				delta = total - progress;
				h->x = h->x - h->fx*delta;
				h->z = h->z - h->fz*delta;
				wait = false;
			}
		}
		else if (even == ROTATE_LEFT){
			float oldfx = h->fx, oldfz = h->fz,
				  delta = 0.01;
			progress = progress + delta;

			if (progress < total){
				h->fx = (oldfx * cos(delta)) + (oldfz * sin(delta));
				h->fz = (oldfz * cos(delta)) - (oldfx * sin(delta));
			}
			else{
				h->fx = destx;
				h->fz = destz;
				wait = false;
			}
		}
		else if (even == ROTATE_RIGHT){
			float oldfx = h->fx, oldfz = h->fz,
				  delta = 0.01;
			progress = progress + delta;

			if (progress < total){
				h->fx = (oldfx * cos(delta)) - (oldfz * sin(delta));
				h->fz = (oldfz * cos(delta)) + (oldfx * sin(delta));
			}
			else{
				h->fx = destx;
				h->fz = destz;
				wait = false;
			}
		}
		glutSetWindow(subWindow1);
		glutPostRedisplay();
		glutSetWindow(subWindow2);
		glutPostRedisplay();
	}
}

void readMap (char* filename){

	// citire fisier
	ifstream input;
	input.open(filename);

	input >> m->M;
	input >> m->N;
	m->maze = new char*[m->M];
	for (int i = 0; i < m->M; i++)
		m->maze[i] = new char[m->N];

	for (int i = 0; i < m->M; i++)
		for (int j = 0; j < m->N; j++){
			input >> m->maze[i][j];
			if (m->maze[i][j] == 's'){
				float x,y,z;
				mapd2v(i,j,&x,&y,&z);
				h1->x = x;
				h1->y = y;
				h1->z = z;
				h1->i = i;
				h1->j = j;
				h1->fx = 0;
				h1->fy = 0;
				h1->fz = 1;
				h1->ux = 0;
				h1->uy = 1;
				h1->uz = 0;
				m->maze[i][j] = 'h';
			}
			else if (m->maze[i][j] == 'q'){
				float x,y,z;
				mapd2v(i,j,&x,&y,&z);
				h2->x = x;
				h2->y = y;
				h2->z = z;
				h2->i = i;
				h2->j = j;
				h2->fx = 0;
				h2->fy = 0;
				h2->fz = -1;
				h2->ux = 0;
				h2->uy = 1;
				h2->uz = 0;
				m->maze[i][j] = 'h';
			}
			else if (m->maze[i][j] == 'p'){
				float x,y,z;
				mapd2v(i,j,&x,&y,&z);
				p->x = x;
				p->y = y;
				p->z = z;
				p->i = i;
				p->j = j;
			}
		}
	input.close();
}


void displayText (int x, int y, void *font, char *text) {

	// setting proj matrix
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);

	// setting model-view
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();	
	glLoadIdentity();

	// position
	glRasterPos2i(x, y);

	char *c;
	for (c = text; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);
	}

	// restore matrix
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void setView (Hero *h){

	if (h->camera == FP_CAMERA){
		
		// a bit forward (m->size/2) to avoid seeing own body
		gluLookAt (h->x + (h->fx*(m->size/2)), h->y + (h->fy*(m->size/2)), h->z + (h->fz*(m->size/2)),
				   h->x + (h->fx*m->size), h->y + (h->fy*m->size), h->z + (h->fz*m->size),//h->x + h->fx, h->y + h->fy, h->z + h->fz,
				   h->ux, h->uy, h->uz); // TODO
	}
	else if (h->camera == TP_CAMERA){

		// 2 units up, 3 units back
		float x = h->x, y = h->y, z = h->z;

		// 3 units back
		x = x - (h->fx*3*m->size);
		y = y - (h->fy*3*m->size);
		z = z - (h->fz*3*m->size);

		// 2 units up
		x = x + (h->ux*2*m->size);
		y = y + (h->uy*2*m->size);
		z = z + (h->uz*2*m->size);

		gluLookAt(x, y, z,
				   h->x, h->y, h->z,
				   h->ux, h->uy, h->uz); // TODO
	}

	else if (h->camera == TD_CAMERA){

		float max, size1, size2;
		size1 = m->M * m->size;
		size2 = m->N * m->size;

		if (size1 > size2)
			max = size1;
		else
			max = size2;
		
		float xm, ym, zm; // center of map
		mapd2v(m->M/2, m->N/2, &xm, &ym, &zm);
		gluLookAt(xm, ym + (1.5*max) + 2, zm, xm, ym, zm, 0, 0, 1);
	}
}

void drawMaze (void){

	float zmin = -1 * ((m->M*m->size)/2),
		  xmin = -1 * ((m->N*m->size)/2),
		  x, z, y, delta = m->size;

	x = xmin;
	z = zmin;
	y = 0;

	// lights
	GLfloat diffuse4f[] = {m->color_br/2, m->color_bg/2, m->color_bb/2, 1.0};
	GLfloat ambient4f[] = {m->color_br/2, m->color_bg/2, m->color_bb/2, 1.0};
	GLfloat specular4f[] = {0.0, 0.0, 0.0, 0.0};
	GLfloat shininess = 40.0; // useless ?

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	for (int i = 0; i < m->M; i++){
		for (int j = 0; j < m->N; j++){
			if (m->maze[i][j] == '1'){
				glPushMatrix();
				glColor3f(m->color_br, m->color_bg, m->color_bb);
				glTranslatef(x,y,z);
				glutSolidCube(m->size);
				glPopMatrix();
			}
			x = x + delta;
		}
		z = z + delta;
		x = xmin;
	}
}

void drawFloor (void){

	float zmin = -1 * ((m->M*m->size)/2),
		  xmin = -1 * ((m->N*m->size)/2),
		  x, z, y, delta = m->size, minidelta = m->size/2;
	
	z = zmin;
	x = xmin;
	y = -1 * minidelta;

	// lights
	GLfloat diffuse4f[] = {m->color_fr/2, m->color_fg/2, m->color_fb/2, 1.0};
	GLfloat ambient4f[] = {m->color_fr/2, m->color_fg/2, m->color_fb/2, 1.0};
	GLfloat specular4f[] = {0.0, 0.0, 0.0, 0.0};
	GLfloat shininess = 40.0;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	glPushMatrix();
	glBegin(GL_QUADS); // TODO 
	for (int i = 0; i < m->M; i++){
		for (int j = 0; j < m->N; j++){
			glColor3f(m->color_fr, m->color_fg, m->color_fb);
			glVertex3f(x-minidelta, y, z-minidelta);
			glVertex3f(x+minidelta, y, z-minidelta);
			glVertex3f(x+minidelta, y, z+minidelta);
			glVertex3f(x-minidelta, y, z+minidelta);
			x = x + delta;
		}
		z = z + delta;
		x = xmin;
	}
	glEnd();
	glPopMatrix();
}

void drawHero (Hero *h){

	float x,y,z;

	// lights-body
	GLfloat diffuse4f[] = {h->color_body_r/2, h->color_body_g/2, h->color_body_b/2, 1.0};
	GLfloat ambient4f[] = {h->color_body_r/2, h->color_body_g/2, h->color_body_b/2, 1.0};
	GLfloat specular4f[] = {0.1, 0.1, 0.1, 1.0};
	GLfloat shininess = 40.0;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	// body
	glColor3f(h->color_body_r, h->color_body_g, h->color_body_b);
	
	x = h->x;
	y = h->y - (h->size/4);
	z = h->z;

	glPushMatrix();
	glTranslatef(x,y,z);
	glutSolidCube(h->size/2);
	glPopMatrix();

	// lights-head
	GLfloat diffuse4f2[] = {h->color_head_r/2, h->color_head_g/2, h->color_head_b/2, 1.0};
	GLfloat ambient4f2[] = {h->color_head_r/2, h->color_head_g/2, h->color_head_b/2, 1.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f2);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	// head
	glColor3f(h->color_head_r, h->color_head_g, h->color_head_b);
	
	x = h->x;// - (h->fx*h->size/8);
	y = h->y + (h->size/6);
	z = h->z;// - (h->fz*h->size/8);

	glPushMatrix();
	glTranslatef(x,y,z);
	glutSolidSphere (h->size/6,20,20);
	glPopMatrix();

	// lights-nose
	GLfloat diffuse4f3[] = {h->color_nose_r/2, h->color_nose_g/2, h->color_nose_b/2, 1.0};
	GLfloat ambient4f3[] = {h->color_nose_r/2, h->color_nose_g/2, h->color_nose_b/2, 1.0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f3);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f3);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	// nose
	glColor3f(h->color_nose_r, h->color_nose_g, h->color_nose_b);
	
	x = h->x; //+ (h->fx*h->size/16);
	y = h->y + (h->size/6);
	z = h->z;// + (h->fz*h->size/16);

	glPushMatrix();
	glTranslatef(x,y,z);

	float angle;
	
	if (h->fx == 1)
		angle = 90;
	else if (h->fx == -1)
		angle = 270;
	else if (h->fz == -1)
		angle = 180;
	else if (h->fz == 1)
		angle = 0;
	else { // classic method
		float alpha = asin (h->fx);
		if (h->fz < 0)
			alpha = PI - alpha;
		angle = (alpha * 180) / PI;
	}

	glRotatef(angle,0.0,1.0,0.0);
	glutSolidCone(h->size/8,h->size/4,20,20);
	glPopMatrix();
}

void drawPortal (void){

	// lights
	GLfloat diffuse4f [] = {p->color_r/2, p->color_g/2, p->color_b/2, 1.0};
	GLfloat ambient4f [] = {p->color_r/2, p->color_g/2, p->color_b/2, 1.0};
	GLfloat specular4f [] = {0.5, 0.5, 0.5, 1.0};
	GLfloat shininess = 100.0;

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient4f);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular4f);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

	glColor3f(p->color_r, p->color_r, p->color_r); // no lights

	glPushMatrix();
	glTranslatef(p->x,p->y,p->z);
	glutSolidSphere(m->size/4,20,20);
	glPopMatrix();
}

void drawMainMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	if (selected == SINGLE_PLAYER){
		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-50,windowHeight/2+40,GLUT_BITMAP_HELVETICA_18, "Single Player");
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-50,windowHeight/2+40,GLUT_BITMAP_HELVETICA_18, "Single Player");
	if (selected == MULTIPLAYER){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-40,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "Multiplayer");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-40,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "Multiplayer");
	if (selected == HIGHSCORES){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-42,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Highscores");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-42,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Highscores");
	if (selected == SETTINGS){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-30,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "Settings");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-30,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "Settings");
	
	if (selected == EXIT){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-13,windowHeight/2-40,GLUT_BITMAP_HELVETICA_18, "Exit");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-13,windowHeight/2-40,GLUT_BITMAP_HELVETICA_18, "Exit");

	glEnable (GL_LIGHTING);
}

void drawSinglePlayerMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	if (selected == CAMPAIGN){

		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-34,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "Campaign");
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-34,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "Campaign");

	if (selected == CUSTOM_MAPS){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-50,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Custom Maps");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-50,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Custom Maps");

	if (selected == RANDOM_MAP){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-25,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "Random");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-25,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "Random");

	glEnable(GL_LIGHTING);	
}

void readCampaign (void){

	char filename[] = "levels\\control.txt"; 

	// read file
	ifstream input;
	input.open(filename);
	string str;
	int lock;
	
	input >> nlevels;
	level_names = new char*[nlevels];
	locked_level = new bool[nlevels];

	for (int i = 0 ; i < nlevels; i++){
		input >> str;
		input >> level;
		input >> lock;
		level_names[i] = new char[str.size()];
		sprintf(level_names[i], "%s %d", str.c_str() , level);
		
		if (lock == 0)
			locked_level[i] = false;
		else
			locked_level[i] = true;
	}
	input.close();
}

void drawCampaignMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// how many levels are displayed - min (11, (nlevels-start))
	int min = nlevels - start_campaign;

	if ( min > DEFAULT_LIST_SIZE )
		min = DEFAULT_LIST_SIZE;
	
	// start from x height
	int x = (min-1)*10;
	for (int i = 0; i < min; i++){
		int index = start_campaign + i;
		char text[100];

		if (locked_level[index] == false){
			sprintf(text, "%s", level_names[index]);
			if (selected == (i+1))
				glColor3f(1.0,1.0,1.0); // white
			else
				glColor3f(1.0,1.0,0.0); // yellow
		}
		else{
			sprintf(text, "%s - Locked", level_names[index]);
			glColor3f(0.3,0.3,0.3); // gray
		}
		displayText(windowWidth/2 - 30,windowHeight/2 + x,GLUT_BITMAP_HELVETICA_18, text);
		x = x - 20;
	}
	glEnable(GL_LIGHTING);	
}

void readCustomMaps (char* dir){

	// deschidem lista
	char filename[100]; // = "cmaps\\list.txt"; 
	sprintf(filename, "%s\\list.txt", dir);
	custom_map_dir = dir;

	// citire fisier
	ifstream input;
	input.open(filename);
	string str;
	
	input >> nmaps;
	getline(input,str); // get rid of '\n'
	custom_map_name = new char*[nmaps];
	custom_map_source = new char*[nmaps];
	int index = 0;

	while (true){
		getline(input,str);
		if (input.eof() == true)
			break;

		custom_map_name[index] = new char[str.size()];
		sprintf(custom_map_name[index],"%s",str.c_str());

		getline(input,str);
		custom_map_source[index] = new char[str.size()];
		sprintf(custom_map_source[index],"%s",str.c_str());
		index++;
	}

	// display will start with first entry
	start_custom = 0;
	input.close();
}

void drawCustomMapsMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	int min = nmaps - start_custom;
	if ( min > DEFAULT_LIST_SIZE )
		min = DEFAULT_LIST_SIZE;
	
	// start from x height
	int x = (min-1)*10;
	for (int i = 0; i < min; i++){
		int index = start_custom + i;
		if (selected == (i+1))
			glColor3f(1.0,1.0,1.0); // white
		else
			glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/2 - 50,windowHeight/2 + x,GLUT_BITMAP_HELVETICA_18, custom_map_name[index]);
		x = x - 20;
	}
	glEnable(GL_LIGHTING);
}

void drawRandomMapMenu (void){

	// no lights
	glDisable(GL_LIGHTING);
	
	char text[100];

	// M
	if (selected == 1)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow
	sprintf(text, "M = %d", randm);
	displayText(windowWidth/2 - 30,windowHeight/2 + 40,GLUT_BITMAP_HELVETICA_18, text);

	// N
	if (selected == 2)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	sprintf(text, "N = %d", randn);
	displayText(windowWidth/2 - 30,windowHeight/2 + 20,GLUT_BITMAP_HELVETICA_18, text);

	// difficulty
	if (selected == 3)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	if (rand_difficulty == EASY_DIFFICULTY)
		sprintf(text, "Difficulty = Easy");
	else if (rand_difficulty == MEDIUM_DIFFICULTY)
		sprintf(text, "Difficulty = Medium");
	if (rand_difficulty == HARD_DIFFICULTY)
		sprintf(text, "Difficulty = Hard");
	if (rand_difficulty == HARDEST_DIFFICULTY)
		sprintf(text, "Difficulty = Hardest");

	displayText(windowWidth/2 - 30,windowHeight/2,GLUT_BITMAP_HELVETICA_18, text);

	// start
	if (selected == 4)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 - 30,windowHeight/2 - 40,GLUT_BITMAP_HELVETICA_18, "Start");
	glEnable(GL_LIGHTING);	
}

void generateRandomMap (int M, int N, int difficulty){

	bool **visited;

	m->M = M;
	m->N = N;

	m->maze = new char*[m->M];
	visited = new bool*[m->M];

	for (int i = 0; i < m->M; i++){
		m->maze[i] = new char[m->N];
		visited[i] = new bool[m->N];
	}

	// initialize with val = 1
	for (int i = 0; i < m->M; i++)
		for (int j = 0; j < m->N; j++){
			m->maze[i][j] = '1';
			visited[i][j] = false;
		}

	// start pos
	srand((unsigned)time(0));
	int random = rand() % N;
	m->maze[0][random] = 'h';
	visited[0][random] = true;

	float x,y,z;
	mapd2v(0,random,&x,&y,&z);

	h1->x = x;
	h1->y = y;
	h1->z = z;
	h1->i = 0;
	h1->j = random;
	h1->fx = 0;
	h1->fy = 0;
	h1->fz = 1;
	h1->ux = 0;
	h1->uy = 1;
	h1->uz = 0;

	// add first fixed neigh
	stack<int> si;
	stack<int> sj;
	stack<int> soldi;
	stack<int> soldj;

	// M should be > 2
	si.push(2);
	sj.push(random);
	soldi.push(0);
	soldj.push(random);
	visited[2][random] = true;

	while (si.empty() == false){
		int i = si.top();
		si.pop();
		int j = sj.top();
		sj.pop();
		int oldi = soldi.top();
		soldi.pop();
		int oldj = soldj.top();
		soldj.pop();

		// fill with 0 from (oldi,oldj) to (i,j)
		if (oldi < i){ // right
			m->maze[oldi+1][oldj] = '0';
			m->maze[oldi+2][oldj] = '0';
			difficulty = difficulty - 2;
		}
		else if (oldi > i){
			m->maze[oldi-1][oldj] = '0';
			m->maze[oldi-2][oldj] = '0';
			difficulty = difficulty - 2;
		}
		else if (oldj < j){
			m->maze[oldi][oldj+1] = '0';
			m->maze[oldi][oldj+2] = '0';
			difficulty = difficulty - 2;
		}
		else if (oldj > j){
			m->maze[oldi][oldj-1] = '0';
			m->maze[oldi][oldj-2] = '0';
			difficulty = difficulty - 2;
		}
		if ((difficulty == 0) || (difficulty == -1)){ // once
			m->maze[i][j] = 'p';
			p->i = i;
			p->j = j;
			float x,y,z;
			mapd2v(i,j,&x,&y,&z);
			p->x = x;
			p->z = z;
			p->y = y;
		}

		// we look at (i,j) neigh
		int directions[] = {0,1,2,3};

		// knuth shuffle
		for (int k = 3; k > 0; k--){
			int h = rand() % (k+1);
			int aux = directions[k];
			directions[k] = directions[h];
			directions[h] = aux;
		}

		for (int k = 0; k < 4; k++){
			if ((directions[k] == 0) && ((i - 2) > 0) && (m->maze[i-2][j] == '1') && (visited[i-2][j] == false)){ // up
				si.push (i-2);
				sj.push (j);
				soldi.push (i);
				soldj.push (j);
				visited[i-2][j] = true;
			}
			else if ((directions[k] == 1) && ((i + 2) < (m->M - 1)) && (m->maze[i+2][j] == '1') && (visited[i+2][j] == false)){ // down
				si.push (i+2);
				sj.push (j);
				soldi.push (i);
				soldj.push (j);
				visited[i+2][j] = true;
			}
			else if ((directions[k] == 2) && ((j + 2) < (m->N - 1)) && (m->maze[i][j+2] == '1') && (visited[i][j+2] == false)){ // right
				si.push (i);
				sj.push (j+2);
				soldi.push (i);
				soldj.push (j);
				visited[i][j+2] = true;
			}
			else if ((directions[k] == 3) && ((j - 2) > 0) && (m->maze[i][j-2] == '1') && (visited[i][j-2] == false)){ // left
				si.push (i);
				sj.push (j-2);
				soldi.push (i);
				soldj.push (j);
				visited[i][j-2] = true;
			}
		}
	}
}

void generateRandomMultiplayerMap (int M, int N, int difficulty){

	bool **visited;

	m->M = M;
	m->N = N;
	m->maze = new char* [m->M];
	visited = new bool* [m->M];

	for (int i = 0; i < m->M; i++){
		m->maze[i] = new char [m->N];
		visited[i] = new bool [m->N];
	}

	// initialize with val = 1
	for (int i = 0; i < m->M; i++)
		for (int j = 0; j < m->N; j++){
			m->maze[i][j] = '1';
			visited[i][j] = false;
		}

	// start pos
	srand((unsigned)time(0));
	int random = rand() % N;
	m->maze[0][random] = 'h';
	visited[0][random] = true;

	float x,y,z;
	mapd2v(0,random,&x,&y,&z);

	// h1
	h1->x = x;
	h1->y = y;
	h1->z = z;
	h1->i = 0;
	h1->j = random;
	h1->fx = 0;
	h1->fy = 0;
	h1->fz = 1;
	h1->ux = 0;
	h1->uy = 1;
	h1->uz = 0;

	// h2
	mapd2v(m->M-1,random,&x,&y,&z);
	h2->x = x;
	h2->y = y;
	h2->z = z;
	h2->i = m->M-1;
	h2->j = random;
	h2->fx = 0;
	h2->fy = 0;
	h2->fz = -1;
	h2->ux = 0;
	h2->uy = 1;
	h2->uz = 0;

	// add first fixed neigh
	stack<int> si;
	stack<int> sj;
	stack<int> soldi;
	stack<int> soldj;

	// M ar trebui sa fie > 2
	si.push(2);
	sj.push(random);
	soldi.push(0);
	soldj.push(random);
	visited[2][random] = true;

	while (si.empty() == false){
		int i = si.top();
		si.pop();
		int j = sj.top();
		sj.pop();
		int oldi = soldi.top();
		soldi.pop();
		int oldj = soldj.top();
		soldj.pop();

		// fill with 0 from (oldi,oldj) to (i,j)
		if (oldi < i){ // right
			m->maze[oldi+1][oldj] = '0';
			m->maze[oldi+2][oldj] = '0';
		}
		else if (oldi > i){
			m->maze[oldi-1][oldj] = '0';
			m->maze[oldi-2][oldj] = '0';
		}
		else if (oldj < j){
			m->maze[oldi][oldj+1] = '0';
			m->maze[oldi][oldj+2] = '0';
		}
		else if (oldj > j){
			m->maze[oldi][oldj-1] = '0';
			m->maze[oldi][oldj-2] = '0';
		}
		if ((difficulty == 0) || (difficulty == -1)){ // TODO : delete
			m->maze[i][j] = 'p';
			p->i = i;
			p->j = j;
			float x,y,z;
			mapd2v(i,j,&x,&y,&z);
			p->x = x;
			p->z = z;
			p->y = y;
		}

		// ne uitam la toti vecinii lui (i,j)
		int directions[] = {0,1,2,3};

		// knuth shuffle
		for (int k = 3; k > 0; k--){

			int h = rand() % (k+1);
			int aux = directions[k];
			directions[k] = directions[h];
			directions[h] = aux;
		}

		for (int k = 0; k < 4; k++){
			if ((directions[k] == 0) && ((i - 2) > 0) && (m->maze[i-2][j] == '1') && (visited[i-2][j] == false)){ // up
				si.push(i-2);
				sj.push(j);
				soldi.push(i);
				soldj.push(j);
				visited[i-2][j] = true;
			}
			else if ((directions[k] == 1) && ((i + 2) <= (m->M/2)) && (m->maze[i+2][j] == '1') && (visited[i+2][j] == false)){ // down - until 1/2
				si.push(i+2);
				sj.push(j);
				soldi.push(i);
				soldj.push(j);
				visited[i+2][j] = true;
			}
			else if ((directions[k] == 2) && ((j + 2) < (m->N - 1)) && (m->maze[i][j+2] == '1') && (visited[i][j+2] == false)){ // right
				si.push(i);
				sj.push(j+2);
				soldi.push(i);
				soldj.push(j);
				visited[i][j+2] = true;
			}
			else if ((directions[k] == 3) && ((j - 2) > 0) && (m->maze[i][j-2] == '1') && (visited[i][j-2] == false)){ // left
				si.push(i);
				sj.push(j-2);
				soldi.push(i);
				soldj.push(j);
				visited[i][j-2] = true;
			}
		}
	}

	// copy in the bottom part
	for (int i = 0; i <= (m->M/2); i++)
		for (int j = 0; j < m->N; j++){
			int opi = m->M - i - 1;
			m->maze[opi][j] = m->maze[i][j];
		}

	// portal
	int aux = m->M/2;
	bool port = false;
	for (int i = m->N/2; i < m->N; i++){
		if ((m->maze[aux][i] == '0') && (port == false)){
			m->maze[aux][i] = 'p';
			p->i = aux;
			p->j = i;
			float x,y,z;
			mapd2v(aux,i,&x,&y,&z);
			p->x = x;
			p->z = z;
			p->y = y;
			port = true;
		}
	}
	if (port == false){
		for (int i = 0; i < m->N/2; i++){
			if ((m->maze[aux][i] == '0') && (port == false)){
				m->maze[aux][i] = 'p';
				p->i = aux;
				p->j = i;
				float x,y,z;
				mapd2v(aux,i,&x,&y,&z);
				p->x = x;
				p->z = z;
				p->y = y;
				port = true;
			}
		}
	}
}

void drawMultiplayerMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	if (selected == MULTIPLAYER_CUSTOM){
		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-44,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "Custom Map");
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-44,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "Custom Map");

	if (selected == MULTIPLAYER_RANDOM){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-25,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "Random");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-25,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "Random");
	glEnable(GL_LIGHTING);
}

void readHighScores (void){

	// open list
	char filename[] = "highscores\\list.txt"; 

	// read file
	ifstream input;
	input.open(filename);
	string str;
	
	input >> nmaps;
	getline(input,str); // get rid of '\n'
	custom_map_name = new char*[nmaps];
	custom_map_source = new char*[nmaps];
	int index = 0;

	while (true){
		getline(input,str);
		if (input.eof() == true)
			break;

		custom_map_name[index] = new char[str.size()];
		sprintf(custom_map_name[index],"%s",str.c_str());
		getline(input,str);
		custom_map_source[index] = new char[str.size()];
		sprintf(custom_map_source[index],"%s",str.c_str());
		index++;
	}

	// display will start with first entry
	start_custom = 0;
	input.close();
}

void drawHighscores (void){

	// no lights
	glDisable(GL_LIGHTING);

	int min = nmaps - start_custom;
	if ( min > DEFAULT_LIST_SIZE )
		min = DEFAULT_LIST_SIZE;
	
	// start from x height x
	int x = (min-1)*10;

	for (int i = 0; i < min; i++){
		int index = start_custom + i;
		if (selected == (i+1))
			glColor3f(1.0,1.0,1.0); // white
		else
			glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/2 - 50,windowHeight/2 + x,GLUT_BITMAP_HELVETICA_18, custom_map_name[index]);
		x = x - 20;
	}
	glEnable(GL_LIGHTING);	
}

void readHighscoreMap (char* filename){

	// read file
	ifstream input;
	input.open(filename);
	string str;
	
	input >> nplayers;
	players = new char*[nplayers];
	times = new int[nplayers];
	getline(input,str); // get rid of '\n'
	
	int index = 0;

	while (true){
		getline (input,str);
		if (input.eof() == true)
			break;

		players[index] = new char[str.size()];
		sprintf(players[index],"%s",str.c_str());

		input >> times[index];
		getline(input, str);
		index++;
	}

	// display will start with first entry
	start_custom = 0;
	input.close();
}

void displayHighscore (void){

	// no lights
	glDisable(GL_LIGHTING);

	// start from x height
	int x = 100;
	char text[100];

	glColor3f(1.0,1.0,0.0); // yellow

	for (int i = 0; i < nplayers; i++){
		sprintf(text, "%d. %s", (i+1), players[i]);
		displayText(windowWidth/2 - 150,windowHeight/2 + x,GLUT_BITMAP_HELVETICA_18, text);
		sprintf(text, "%d s", times[i]);
		displayText(windowWidth/2 + 150,windowHeight/2 + x,GLUT_BITMAP_HELVETICA_18, text);
		x = x - 20;
	}
	glEnable(GL_LIGHTING);	
}


void drawSettingsMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	if (selected == 1){ // hero
		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-13,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "Hero");
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-13,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "Hero");
	if (selected == 2){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-20,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "Colors");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-20,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "Colors");

	if (selected == 3){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-80,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "Single Player Controls");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-80,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "Single Player Controls");

	if (selected == 4){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-70,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "Multiplayer Controls");
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-70,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "Multiplayer Controls");
	
	glEnable(GL_LIGHTING);
}

void drawHeroMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	char text[100];
	sprintf(text, "Name : %s", heroname1);

	if (selected == 1){ // hero
		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-80,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-80,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Red : %.2f", h1->color_body_r);
	if (selected == 2){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-80,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-80,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Green : %.2f", h1->color_body_g);
	if (selected == 3){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-80,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-80,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Blue : %.2f", h1->color_body_b);
	if (selected == 4){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-80,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-80,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, text);
	
	glEnable(GL_LIGHTING);	
}

void drawColorsMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	char text[100];

	sprintf(text, "Wall Red : %.2f", m->color_br);
	if (selected == 1){ 
		// white
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2+50,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else 
		displayText(windowWidth/2-120,windowHeight/2+50,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Wall Green : %.2f", m->color_bg);
	if (selected == 2){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-120,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, text);

	glColor3f(m->color_br, m->color_bg, m->color_bb);
	displayText(windowWidth/2+120,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "Sample");
	glColor3f(1.0,1.0,0.0);

	sprintf(text, "Wall Blue : %.2f", m->color_bb);
	if (selected == 3){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-120,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Floor Red : %.2f", m->color_fr);
	if (selected == 4){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-120,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, text);

	sprintf(text, "Floor Green : %.2f", m->color_fg);
	if (selected == 5){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-120,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, text);

	glColor3f(m->color_fr, m->color_fg, m->color_fb);
	displayText(windowWidth/2+120,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "Sample");
	glColor3f(1.0,1.0,0.0);

	sprintf(text, "Floor Blue : %.2f", m->color_fb);
	if (selected == 6){
		glColor3f(1.0,1.0,1.0);
		displayText(windowWidth/2-120,windowHeight/2-50,GLUT_BITMAP_HELVETICA_18, text);
		glColor3f(1.0,1.0,0.0);
	}
	else
		displayText(windowWidth/2-120,windowHeight/2-50,GLUT_BITMAP_HELVETICA_18, text);

	glEnable(GL_LIGHTING);
}

void drawSPControlsMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow - all of them
	glColor3f(1.0,1.0,0.0);

	int axisleft = -100, axisright = 100;

	if (selected == 1)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2+80,GLUT_BITMAP_HELVETICA_18, "Return to Main Menu :");
	displayText(windowWidth/2 + axisright,windowHeight/2+80,GLUT_BITMAP_HELVETICA_18, "ESC"); // key_esc

	if (selected == 2)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2+60,GLUT_BITMAP_HELVETICA_18, "Move Forward :");
	displayText(windowWidth/2 + axisright,windowHeight/2+60,GLUT_BITMAP_HELVETICA_18, "w");

	if (selected == 3)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2+40,GLUT_BITMAP_HELVETICA_18, "Move Backward :");
	displayText(windowWidth/2 + axisright,windowHeight/2+40,GLUT_BITMAP_HELVETICA_18, "s");

	if (selected == 4)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "Rotate Left :");
	displayText(windowWidth/2 + axisright,windowHeight/2+20,GLUT_BITMAP_HELVETICA_18, "a");

	if (selected == 5)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Rotate Right :");
	displayText(windowWidth/2 + axisright,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "d");

	if (selected == 6)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "Change Camera :");
	displayText(windowWidth/2 + axisright,windowHeight/2-20,GLUT_BITMAP_HELVETICA_18, "c");

	if (selected == 7)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axisleft,windowHeight/2-40,GLUT_BITMAP_HELVETICA_18, "Turn On/Off Light :");
	displayText(windowWidth/2 + axisright,windowHeight/2-40,GLUT_BITMAP_HELVETICA_18, "l");
	glEnable(GL_LIGHTING);	
}

void drawMPControlsMenu (void){

	// no lights
	glDisable(GL_LIGHTING);

	// yellow
	glColor3f(1.0,1.0,0.0);

	int axis1 = -300, axis2 = -100, axis3 = 100;
	
	displayText(windowWidth/2 + axis2,windowHeight/2+70,GLUT_BITMAP_HELVETICA_18, "Player 1");
	displayText(windowWidth/2 + axis3,windowHeight/2+70,GLUT_BITMAP_HELVETICA_18, "Player 2");
	
	if (selected == 1)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axis1,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "Move Forward :");
	displayText(windowWidth/2 + axis2,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "w");
	displayText(windowWidth/2 + axis3,windowHeight/2+30,GLUT_BITMAP_HELVETICA_18, "8");

	if (selected == 2)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axis1,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "Move Backward :");
	displayText(windowWidth/2 + axis2,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "s");
	displayText(windowWidth/2 + axis3,windowHeight/2+10,GLUT_BITMAP_HELVETICA_18, "5");

	if (selected == 3)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axis1,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "Rotate Left :");
	displayText(windowWidth/2 + axis2,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "a");
	displayText(windowWidth/2 + axis3,windowHeight/2-10,GLUT_BITMAP_HELVETICA_18, "4");

	if (selected == 4)
		glColor3f(1.0,1.0,1.0); // white
	else
		glColor3f(1.0,1.0,0.0); // yellow

	displayText(windowWidth/2 + axis1,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "Rotate Right :");
	displayText(windowWidth/2 + axis2,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "d");
	displayText(windowWidth/2 + axis3,windowHeight/2-30,GLUT_BITMAP_HELVETICA_18, "6");
	glEnable(GL_LIGHTING);	
}


void mapd2v (int i, int j, float *x, float *y, float *z){

	float zmin = -1 * ((m->M*m->size)/2), // i = 0
		  xmin = -1 * ((m->N*m->size)/2), // j = 0
		  delta = delta = m->size;

	*x = xmin + (j*delta);
	*y = 0;
	*z = zmin + (i*delta);
}

void singlePlayerVictory (void){

	// show gz
	time_finish = time(NULL);
	glDisable(GL_LIGHTING);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1.0,1.0,0.0); // yellow
	displayText(windowWidth/2 - 30,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "You Won!");
	glutSwapBuffers();

	// wait 2s
	Sleep(2000);

	// unlock if campaign
	if ((level >= 0) && (level < 9) && (locked_level[level+1] == true)){

	char text[100];
	sprintf(text, "You have unlocked level %d !", (level + 1));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1.0,1.0,0.0); // yellow
	displayText(windowWidth/2 - 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, text);
	glutSwapBuffers(); // useless ?

	Sleep(2000);

	char filename[] = "levels\\control.txt"; 
	ifstream input;
	input.open(filename);
	string str;
	int lock;
	
	input >> nlevels;
	level_names = new char*[nlevels];
	locked_level = new bool[nlevels];

	for (int i = 0 ; i < nlevels; i++){
		int level2;
		input >> str;
		input >> level2;
		input >> lock;
		level_names[i] = new char[str.size()];
		sprintf(level_names[i], "%s %d", str.c_str() , level2);
		
		if (lock == 0)
			locked_level[i] = false;
		else
			locked_level[i] = true;
		}

		input.close();
		locked_level[level+1] = false;
		ofstream output(filename);
						
		sprintf(text, "%d\n", nlevels);
		output << text;
						
		for (int i = 0; i < nlevels; i++){
			if (locked_level[i] == true)
				sprintf(text, "Level %d 1\n", i);
			else
				sprintf(text, "Level %d 0\n", i);
			output << text;
		}

		output.close();
		nlevels = 0;
	}

	if ((level == 9) && (finished_campaign == false)){
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/2 - 130,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "Congratulation ! You've finished the Campaign !");
		glutSwapBuffers(); 
		finished_campaign = true;
		Sleep(2000);
	}

	// update highscore - if map is not random
	if (level != -2){
		char filename[] = "highscores\\list.txt"; 
		bool exists = false;
		char sourcename[100];

		// read file
		ifstream input;
		input.open(filename);
		string str;
	
		input >> nmaps;
		getline(input,str); // get rid of '\n'
		custom_map_name = new char*[nmaps];
		custom_map_source = new char*[nmaps];
		int index = 0;

		while (true){
			getline(input,str);
			if (input.eof() == true)
				break;
			
			custom_map_name[index] = new char [str.size()];
			sprintf(custom_map_name[index],"%s",str.c_str());

			getline(input,str);
			custom_map_source[index] = new char [str.size()];
			sprintf(custom_map_source[index],"%s",str.c_str());

			// check if file has current map
			if (strcmp(custom_map_source[index], current_map_source) == 0)
				exists = true;

			index++;
		}

		input.close();
		if (exists == true){
			long int timep = time_finish - time_start;
			char filename2[100];
			char text[100];
			sprintf(filename2, "highscores\\%s", current_map_source);
			ifstream input;
			input.open(filename2);
			string str;
	
			input >> nplayers;
			players = new char*[nplayers];
			times = new int[nplayers];
			getline(input,str); // get rid of '\n'
	
			index = 0;
			while (true){
				getline(input,str);
				if (input.eof() == true)
					break;

				players[index] = new char[str.size()];
				sprintf(players[index],"%s",str.c_str());

				input >> times[index];
				getline(input, str);
				index++;
			}
			
			input.close();
			ofstream output(filename2);
							
			int N = nplayers;
			if (N < 10)
				N++;
			
			sprintf(text, "%d\n", N);
			output << text;
			index = 0;
			bool added = false;
			int aux = (int)timep;
							
			// re-add
			for (int i = 0; i < N; i++){
				if (index == nplayers){ // add last, if < 10
					sprintf(text,"%s\n",h1->name);
					output << text;
					sprintf(text,"%d\n",aux); // ld ??
					output << text;

					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glColor3f(1.0,1.0,0.0); // yellow
					sprintf(text, "Your rank on map %s is %d !", current_map_name, (i+1));
					displayText(windowWidth/2 - 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, text);
					glutSwapBuffers(); 
					Sleep(2000);
				}

				else if ((aux < times[index]) && (added == false)){
					// add time to file
					sprintf(text,"%s\n",h1->name);
					output << text;
					sprintf(text,"%d\n",aux); // ld ??
					output << text;

					// show rank
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glColor3f(1.0,1.0,0.0); // yellow
					sprintf(text, "Your rank on map %s is %d !", current_map_name, (i+1));
					displayText(windowWidth/2 - 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, text);
					glutSwapBuffers(); 
					Sleep(2000);
					added = true;
				}

				else{
					sprintf(text,"%s\n",players[index]);
					output << text;
					sprintf(text,"%ld\n",times[index]);
					output << text;
					index++;
				}
			}
			output.close();
			nplayers = 0;
		}
		else{ // add
			nmaps++;
			ofstream output(filename);
			char text[100];
			char filename2[100];

			sprintf(text, "%d\n", nmaps);
			output << text;
			for (int i = 0; i < (nmaps-1); i++){
				sprintf(text, "%s\n", custom_map_name[i]);
				output << text;
				sprintf(text, "%s\n", custom_map_source[i]);
				output << text;
			}

			sprintf(text, "%s\n", current_map_name);
			output << text;
			sprintf(text, "%s\n", current_map_source);
			output << text;

			output.close();
			nmaps = 0;

			// create new file
			sprintf(filename2, "highscores\\%s", current_map_source);
			output.open(filename2);
			sprintf(text, "1\n");
			output << text;
			sprintf(text, "%s\n", h1->name);
			output << text;
			time_finish = time_finish - time_start;
			sprintf(text, "%ld\n", time_finish);
			output << text;
			output.close();

			// show info
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glColor3f(1.0,1.0,0.0); // yellow
			sprintf(text, "Your rank is 1 on %s map !", current_map_name);
			displayText(windowWidth/2 - 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, text);
			glutSwapBuffers(); 
			Sleep(2000);
		}
	}

	// go back to main menu
	glEnable(GL_LIGHTING);
	menu = MAIN_MENU;
	selected = NONE; // useless
	glutPostRedisplay();
}

void multiplayerVictory (void){
	
	if (mp_player == PLAYER_1){ // p1 has won

		// win
		glutSetWindow(subWindow1);
		glDisable(GL_LIGHTING);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/4 + 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "You Won!");
		glutSwapBuffers();
					
		// lose
		glutSetWindow(subWindow2);
		glDisable(GL_LIGHTING);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/4 + 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "You Lost!");
		glutSwapBuffers();
	}

	else if (mp_player == PLAYER_2){ // p2 has won

		// win
		glutSetWindow(subWindow2);
		glDisable(GL_LIGHTING);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/4 + 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "You Won!");
		glutSwapBuffers();
					
		// lose
		glutSetWindow(subWindow1);
		glDisable(GL_LIGHTING);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor3f(1.0,1.0,0.0); // yellow
		displayText(windowWidth/4 + 100,windowHeight/2,GLUT_BITMAP_HELVETICA_18, "You Lost!");
		glutSwapBuffers();
	}
	
	Sleep(3000);
	glEnable(GL_LIGHTING);

	// free mem
	delete(m->maze);

	// delete windows
	glutDestroyWindow(subWindow1);
	glutDestroyWindow(subWindow2);
			
	menu = MAIN_MENU;
	selected = 0;
	glutSetWindow(mainWindow);
	glutPostRedisplay();
}