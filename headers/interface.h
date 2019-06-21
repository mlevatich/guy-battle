/*
 Interface control

 The interface includes toolbar elements (health bar, cooldown meters, logo, etc) and
 text elements (selectable menu options and non-selectable text displayed to the screen).
 */

// List of game states
enum modes
{ OPENING, TITLE, CONTROLS, STAGE_SELECT, VS, AI, PAUSE, GAME_OVER_VS, GAME_OVER_AI };

// Move the text selection arrow
int hover(int mode, int direction);

// Change the score
void setScore(int new_score);

// Add points to score
void updateScore(int points);

// Render all of the current mode's toolbar and text elements to the screen
void renderInterface(int mode, long long frame, int guy_hp, int guy2_hp, double* guy_cds, double* guy2_cds);

// Load the toolbar texture, toolbar elements, and selection text into memory
void loadInterface(void);

// Free the toolbar elements, toolbar texture, and selection text from memory
void freeInterface(void);
