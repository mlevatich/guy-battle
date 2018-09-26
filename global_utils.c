#include "global_utils.h"

// Screen setup
int SCREEN_WIDTH = 1024;
int SCREEN_HEIGHT = 768;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Frame rate constants
double FRAME_INCREMENT = 0.1;
int MAX_FPS = 60;
double MS_PER_FRAME = 16.666;

// Music and sound effects
Mix_Music *main_theme = NULL;

// Debug mode
bool debug = false;

// Texture loading
SDL_Texture* loadTexture(const char* path)
{
    // Create a surface from path to bitmap file
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loaded = SDL_LoadBMP(path);
    
    // Create a texture from the surface
    newTexture = SDL_CreateTextureFromSurface(renderer, loaded);
    SDL_FreeSurface(loaded);
    return newTexture;
}

// Max of two numbers
double max(double x, double y)
{
    if(x > y) return x; return y;
}

// Min of two numbers
double min(double x, double y)
{
    if(x > y) return y; return x;
}

// Random number between 0 and 1
double get_rand()
{
    return (double)rand()/(double)RAND_MAX;
}

// Convert (0,1) to (-1,1)
int convert(bool c)
{
    return (c - (c == 0));
}
