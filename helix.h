#ifndef HELIX_H
#define HELIX_H

#include <string.h>
#include <sprintf.h>
#include <consts.h>
#include <stdio.h>
#include <graphics.h>
#include <types.h>
#include <math.h>
#include <floatimpl.h>

#include "data.h"

#define SCREEN_W 320
#define SCREEN_H 240

#define SPRITES_MAX 12
#define ENEMIES_MAX 12
#define SPRITE_MIN_RENDER_DISTANCE 0.2f

#define ABS(N) ((N<0)?(-N):(N))

typedef struct {
    float posX;		// World position
	float posY;
	float dirX;		// World direction (normalized)
	float dirY;
	float planeX;	// Camera plane
	float planeY;
} Camera;

typedef struct {
	char up : 1;		// UP
	char down : 1;		// DOWN
	char right : 1;		// RIGHT
	char left : 1;		// LEFT
	char shoot : 1;		// SPACE
	char restart : 1;	// R
	char quit : 1;		// ESC
} Input;

typedef struct
{
	float x;		// World location
	float y;
	int visible; 	// Render this sprite
    int animIndex;	// Which sprite to render from the bitmap set
	int texture;	// Starting index of bitmap set
	int animTimer;	// Timer for switching animation frames
} Sprite;

typedef struct
{
	Sprite* sprite;	// Sprite object that the enemy will be using
	int hp;			// Health
	int active;		// Process enemy behaviour
	int isStatic;	// Ignore enemy movement AI
	int moving;		// Is the enemy currently moving
} Enemy;

void init_camera();												// Reset main camera state
void init_enemies();											// Reset enemy states
void player_update();											// Handle player movement and shooting
void input_update();											// Get input from keyboard
void enemy_update();											// Animate enemy sprites and handle enemy AI
void renderer_update(Camera *);									// Render a camera to buffer
void draw_tutorial();											// Draw tutorial text
void transfer_buffer();											// Copy stored image buffer to gpu
void draw_pixel(uint8_t*, int, int, uint8_t);					// Draw a single pixel
void clear_buffer();                     						// Clear image buffer to color 0
void draw_bitmap(uint8_t*, uint8_t*, int, int, int, uint8_t);	// Draw a bitmap directly on screen
void wall_raycaster(Camera *);          						// Render walls
void sprite_raycaster(Camera *);        						// Render sprites
void sort_sprites(int*, float*, Camera*, int);					// Sort sprites by camera distance using quick sort

#endif