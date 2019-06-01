#include "../headers/constants.h"
#include "../headers/level.h"

// Struct for background information
typedef struct background
{
    // Meta info about the background
    SDL_Texture* image;         // texture for this background
    int width;                  // image width in pixels
    int height;                 // image height in pixels
    int x_init;                 // initial x rendering position
    int y_init;                 // initial y rendering position
    double xv_init;             // initial x velocity
    double yv_init;             // initial y velocity
    int drift_type;             // how does this background move (SCROLL, DRIFT)

    // State info about the background
    double x;                   // current x position
    double y;                   // current y position
    double x_vel;               // current x velocity
    double y_vel;               // current y velocity

}* Background;

// Struct for foreground information
typedef struct foreground
{
    SDL_Texture* image;         // texture for this foreground
    int* platforms;             // { pf1_y, pf1_x1, pf1_x2, pf2_y, ... } pf1 is the ground by convention
    int* walls;                 // { wall1_x, wall1_y1, wall1_y2, wall2_x, ... }
    int* starting_positions;    // { guy1_x, guy1_y, guy2_x, guy2_y }
}* Foreground;

// Types of background behavior
enum drift_types
{ SCROLL, DRIFT };

#define NUM_BACKGROUNDS 2       // Number of existing backgrounds
#define NUM_FOREGROUNDS 2       // Number of existing foregrounds

Background* backgrounds = NULL; // Array of existing backgrounds
Foreground* foregrounds = NULL; // Array of existing foregrounds

int currentBackground = FOREST; // Current background
int currentForeground = FOREST; // Current foreground

/* SETTERS */

// Switch the level to a new one
void switchLevel(int new_level)
{
    currentBackground = new_level;
    currentForeground = new_level;
}

/* GETTERS */

// Returns current level
int getLevel()
{
    return currentForeground;
}

// Returns the platforms on the current foreground
int* getPlatforms()
{
    return foregrounds[currentForeground]->platforms;
}

// Returns the walls on the current foreground
int* getWalls()
{
    return foregrounds[currentForeground]->walls;
}

// Returns starting position of the guys on the current foreground
int* getStartingPositions(int fg)
{
    return foregrounds[fg]->starting_positions;
}

/* PER FRAME UPDATES */

// Animate the background
void moveBackground()
{
    // Update position of the current background according to its velocity
    Background bg = backgrounds[currentBackground];
    bg->x += bg->x_vel;
    bg->y += bg->y_vel;

    if(bg->drift_type == DRIFT)
    {
        // Drifting backgrounds bounce when they reach the image's edge
        int reset_to = 0;
        if(bg->x >= (reset_to = bg->width - SCREEN_WIDTH) || bg->x <= (reset_to = 0))
        {
            bg->x_vel *= -1;
            bg->x = reset_to;
        }
        if(bg->y >= (reset_to = bg->height - SCREEN_HEIGHT) || bg->y <= (reset_to = 0))
        {
            bg->y_vel *= -1;
            bg->y = reset_to;
        }
    }
    else
    {
        // Scrolling backgrounds reset so they appear to loop infinitely
        if(bg->x >= bg->width || bg->x <= bg->width * -1) bg->x = 0;
    }
}

// Render the current background
static void renderBackground()
{
    // Draw the background at it's current position
    Background bg = backgrounds[currentBackground];
    SDL_Rect quad = {(int) bg->x * -1, (int) bg->y * -1, bg->width, bg->height};
    SDL_RenderCopy(renderer, bg->image, NULL, &quad);

    // If the background scrolls, we may need to render it twice to create the illusion of looping
    if(bg->drift_type == SCROLL && (bg->x + SCREEN_WIDTH > bg->width || bg->x < 0))
    {
        quad.x = (int)bg->x * -1 + convert(bg->x > 0) * bg->width;
        quad.y = (int)bg->y * -1;
        quad.w = bg->width;
        quad.h = bg->height;
        SDL_RenderCopy(renderer, bg->image, NULL, &quad);
    }
}

// Render the current foreground
static void renderForeground()
{
    SDL_RenderCopy(renderer, foregrounds[currentForeground]->image, NULL, NULL);
}

// Render the current level
void renderLevel()
{
    renderBackground();
    renderForeground();
}

/* DATA ALLOCATION / INITIALIZATION */

// Assign background fields
static Background initBackground(const char* path, int drift, int w, int h, int x, int y, double x_vel, double y_vel)
{
    // Make space for this background and load its texture
    Background this_background = (Background) malloc(sizeof(struct background));
    this_background->image = loadTexture(path);

    // Assign positional data to the background
    this_background->width = w;     this_background->height = h;
    this_background->x_init = x;        this_background->y_init = y;
    this_background->x = x;             this_background->y = y;
    this_background->xv_init = x_vel;   this_background->yv_init = y_vel;
    this_background->x_vel = x_vel;     this_background->y_vel = y_vel;

    // Set up drifting/movement for this background
    this_background->drift_type = drift;
    if(drift == SCROLL)
    {
        this_background->y_vel = 0;
        this_background->yv_init = 0;
    }

    // Return the new background struct
    return this_background;
}

// Assign foreground fields
static Foreground initForeground(const char* path, int* platforms, int* walls, int* starts)
{
    // Make space for this foreground and load its texture
    Foreground this_foreground = (Foreground) malloc(sizeof(struct foreground));
    this_foreground->image = loadTexture(path);

    // Assign position data to foreground and return it
    this_foreground->platforms = platforms;
    this_foreground->walls = walls;
    this_foreground->starting_positions = starts;
    return this_foreground;
}

// Load all backgrounds and foregrounds into memory
void loadLevels()
{
    // Make space for backgrounds and foregrounds
    backgrounds = (Background*) malloc(NUM_BACKGROUNDS * sizeof(Background));
    foregrounds = (Foreground*) malloc(NUM_FOREGROUNDS * sizeof(Foreground));
    int numPlatforms; int numWalls;

    // Initialize backgrounds
    backgrounds[FOREST] = initBackground("art/forest_background.bmp", SCROLL, 1400, SCREEN_HEIGHT, 0, 0, 0.1, 0);
    backgrounds[VOLCANO] = initBackground("art/volcano_background.bmp", DRIFT, 1400, 1000, 200, 150, 0.1, -0.1);

    // Initialize foregrounds

    // Forest platforms
    numPlatforms = 5;
    int* forest_platforms = (int*) malloc(sizeof(int) * (numPlatforms*3 + 1));
    memcpy
    (
        forest_platforms,
        (int[]) { numPlatforms, 660, 0, 1024, 250, 60, 154, 575, 300, 724, 250, 870, 964, 100, 1024, 1124 },
        sizeof(int) * (numPlatforms*3 + 1)
    );

    // Forest walls
    numWalls = 2;
    int* forest_walls = (int*) malloc(sizeof(int) * (numWalls*3 + 1));
    memcpy
    (
        forest_walls,
        (int[]) { numWalls, 60, 0, SCREEN_HEIGHT, 964, 0, SCREEN_HEIGHT },
        sizeof(int) * (numWalls*3 + 1)
    );

    // Forest Guy starting spots
    int* forest_starts = (int*) malloc(sizeof(int) * 4);
    memcpy(forest_starts, (int[]) { 100, 190, 896, 190 }, sizeof(int) * 4);
    foregrounds[FOREST] = initForeground("art/forest_foreground.bmp", forest_platforms, forest_walls, forest_starts);

    // Volcano platforms
    numPlatforms = 4;
    int* volcano_platforms = (int*) malloc(sizeof(int) * (numPlatforms*3 + 1));
    memcpy
    (
        volcano_platforms,
        (int[]) { numPlatforms, 452, 200, 824, 352, 224, 324, 352, 700, 800, 100, 1024, 1124 },
        sizeof(int) * (numPlatforms*3 + 1)
    );

    // Volcano walls
    numWalls = 0;
    int* volcano_walls = (int*) malloc(sizeof(int) * (numWalls*3 + 1));
    memcpy
    (
        volcano_walls,
        (int[]) { numWalls },
        sizeof(int) * (numWalls*3 + 1)
    );

    // Volcano Guy starting spots
    int* volcano_starts = (int*) malloc(sizeof(int) * 4);
    memcpy(volcano_starts, (int[]){250, 290, 747, 290}, sizeof(int) * 4);
    foregrounds[VOLCANO] = initForeground("art/volcano_foreground.bmp", volcano_platforms, volcano_walls, volcano_starts);
}

/* DATA UNLOADING */

// Free a background from memory
static void freeBackground(Background bg)
{
    SDL_DestroyTexture(bg->image);
    free(bg);
}

// Free a foreground from memory
static void freeForeground(Foreground fg)
{
    SDL_DestroyTexture(fg->image);
    free(fg->platforms);
    free(fg->walls);
    free(fg->starting_positions);
    free(fg);
}

// Free all backgrounds and foregrounds
void freeLevels()
{
    for(int i = 0; i < NUM_BACKGROUNDS; i++) freeBackground(backgrounds[i]);
    free(backgrounds);

    for(int i = 0; i < NUM_FOREGROUNDS; i++) freeForeground(foregrounds[i]);
    free(foregrounds);
}
