/*
 Information needed by more than one module
 */

#include <SDL2/SDL.h>
#include <SDL2_mixer/SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// Debug mode
extern bool debug;

// Screen setup
extern int SCREEN_WIDTH;
extern int SCREEN_HEIGHT;
extern SDL_Window* window;
extern SDL_Renderer* renderer;

// Frame rate constants
extern double FRAME_INCREMENT;
extern int MAX_FPS;
extern double MS_PER_FRAME;

// Music and sound effects
extern Mix_Music* main_theme;

// Index values that multiple modules need
extern int numSprites;
extern int numSpells;
extern int numBackgrounds;
extern int numForegrounds;
extern int numElements;

// Directions
enum directions
{ LEFT, RIGHT, UP, DOWN };

// Texture loading
SDL_Texture* loadTexture(const char* path);

// Macros
double max(double x, double y);
double min(double x, double y);
double get_rand(void);
int convert(bool c);
