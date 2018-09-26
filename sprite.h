/*
 Sprite control
 
 A sprite is a spell, particle, or player character. Sprite control thus
 includes physics, animation, collision, and generally the bulk of the
 game's code.
 */

// Sprite list - doubles as the spell list, so spells must come first
enum identities
{ FIREBALL, ICESHOCK, ICESHOCK_PARTICLE, GUY };

// Allow main to pass around Guy sprites
typedef struct sprite* Sprite;

// Load the sprite sheet texture into memory
void loadSpriteSheet(void);

// Load sprite and spell meta info
void loadSpriteInfo(void);

// Free the sprite sheet texture from memory
void freeSpriteSheet(void);

// Free sprite and spell meta info
void freeMetaInfo(void);

// Free all sprites
void freeSprite_all(void);

// Render all active sprites to the screen
void renderSprite_all(void);

// Check collisions for all active sprites
void detectCollision_all(int* platforms, int* walls);

// Change the action state for all active sprites
void changeAction_all(void);

// Move all active sprites
void moveSprite_all(int* platforms, int* walls);

// Unload any active sprites which have died
void unloadSprites(void);

// Spawn (activate) a sprite at the given coordiantes with the given velocity
Sprite spawnSprite(char id, char hp, double x, double y, double x_vel, double y_vel, int direction);

// Teleport a sprite to a different location
void teleportSprite(Sprite sp, double x, double y);

// Check if its time to spawn new spells, and spawn them
void launchSpells(Sprite guy, int* score);

// Advance cooldown and casting timers
void advanceGuyTimers(Sprite guy);

// Advance collision timers
void advanceCollisions(void);

// Attempt to cast a spell after a keyboard input
bool cast_spell(Sprite guy, int spell);

// Attempt to walk in a direction after a keyboard input
bool walk(Sprite guy, int direction);

// Attempt to jump after a keyboard input
bool jump(Sprite guy);

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(Sprite guy);

// Get a guy's health remaining
int getHealth(Sprite guy);

// Reset the health and cooldowns of the guys after a match ends
void resetGuys(Sprite guy, Sprite guy2);
