/*
 Level control

 This encompasses backgrounds (non-interactive, lightly animated),
 and foregrounds (interactive level ground, platforms, and walls)
 */

// Number of existing backgrounds and foregrounds
#define NUM_BACKGROUNDS 2
#define NUM_FOREGROUNDS 2

// Background / Foreground list
enum levels
{ FOREST, VOLCANO };

// Switch the level to a new one
void switchLevel(int new_level);

// Returns current level
int getLevel(void);

// Return the platforms on the current foreground
int* getPlatforms(void);

// Return the walls on the current foreground
int* getWalls(void);

// Return the starting positions of both guys for the given foreground
int* getStartingPositions(int fg);

// Animate the background
void moveBackground(void);

// Render the current level
void renderLevel(void);

// Load all backgrounds and foregrounds
void loadLevels(void);

// Free all backgrounds and foregrounds
void freeLevels(void);
