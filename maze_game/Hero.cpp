#include "Hero.h"


Hero::Hero(char* name){

	this->name = name;
	this->size = DEFAULT_HERO_SIZE;
	this->color_body_r = DEFAULT_HERO_BODY_RED;
	this->color_body_g = DEFAULT_HERO_BODY_GREEN;
	this->color_body_b = DEFAULT_HERO_BODY_BLUE;
	this->color_head_r = DEFAULT_HERO_HEAD_RED;
	this->color_head_g = DEFAULT_HERO_HEAD_GREEN;
	this->color_head_b = DEFAULT_HERO_HEAD_BLUE;
	this->color_nose_r = DEFAULT_HERO_NOSE_RED;
	this->color_nose_g = DEFAULT_HERO_NOSE_GREEN;
	this->color_nose_b = DEFAULT_HERO_NOSE_BLUE;
	this->camera = DEFAULT_CAMERA;
}

Hero::~Hero(void){
}