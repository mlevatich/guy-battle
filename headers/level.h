/*
 Level control

 This encompasses backgrounds (non-interactive, lightly animated),
 and foregrounds (interactive level ground, platforms, and walls)
 */

// Background / Foreground list
enum levels
{ FOREST, VOLCANO };

// Load all backgrounds and foregrounds
void loadLevels(void);

// Free all backgrounds and foregrounds
void freeLevels(void);

// Render the current background
void renderBackground(void);

// Render the current foreground
void renderForeground(void);

// Switch the level to a new one
void switchLevel(int new_level);

// Animate the background
void moveBackground(void);

// Return the platforms on the current foreground
int* getPlatforms(void);

// Return the walls on the current foreground
int* getWalls(void);

// Returns current level
int getLevel(void);

// Return the starting positions of both guys for the given foreground
int* getStartingPositions(int fg);
