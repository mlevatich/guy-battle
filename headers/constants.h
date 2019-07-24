/*
 Constants used by multiple modules
 */

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// Screen size
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768

// Frame rate constants
#define ANIMATION_SPEED 1
#define MAX_FPS 60

// Cardinal directions
enum directions
{ LEFT, RIGHT, UP, DOWN };

// Get random number in [0, 1)
static inline double get_rand() { return (double) rand() / (double) RAND_MAX; }

// Convert (0,1) to (-1,1)
static inline int convert(bool c) { return (c - (c == 0)); }

// External constants initialized in main.c
// Rendering, display, texture loading
extern SDL_Window* window;
extern SDL_Renderer* renderer;
SDL_Texture* loadTexture(const char* path);

// In debug mode, the framerate is lowered, the opening scene is skipped, there are no cooldowns,
// music is muted, and sprite origins and bounding boxes are rendered
extern bool debug;
