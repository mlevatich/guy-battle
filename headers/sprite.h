/*
 Sprite control

 A sprite is a spell, particle, or player character. Sprite control thus
 includes physics, animation, collision, and generally the bulk of the
 game's code.
 */

// Sprite list - doubles as the spell list, so spells must come first
enum identities
{ FIREBALL, ICESHOCK, ICESHOCK_PARTICLE, GUY };

// Possible action states for a sprite
enum action_types
{ SPAWN, MOVE, COLLIDE, IDLE, JUMP, CAST_FIREBALL, CAST_ICESHOCK, DIE };

// Sprite types
enum types
{ HUMANOID, PARTICLE, SPELL };

// Allow main to pass around Guy sprites
typedef struct sprite* Sprite;

// Spawn (construct) a sprite with the given fields
Sprite spawnSprite(char id, double x, double y, double xv, double yv, int act, bool dir, int angle, char spawning);

// Teleport a sprite to a different location
void setPosition(Sprite sp, double x, double y);

// Hide a guy in the top right corner of the map
void hideGuy(Sprite guy);

// Reset the health and cooldowns of the guys after a match ends
void resetGuys(Sprite guy, Sprite guy2);

// Get a guy's health remaining
int getHealth(Sprite guy);

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(Sprite guy);

// Attempt to walk in a direction after a keyboard input
bool walk(Sprite guy, bool direction);

// Attempt to jump after a keyboard input
bool jump(Sprite guy);

// Attempt to cast a spell after a keyboard input
bool cast(Sprite guy, int spell);

// Check if its time to spawn new spells, and spawn them, returning the change in score
void launchSpells(void);

// Check for and handle terrain collisions for all active sprites
void terrainCollisions(int* platforms, int* walls);

// Check for and handle collisions between all active sprites
void spriteCollisions();

// Update the animation frame which is drawn for all active sprites
void updateAnimationFrames(void);

// Update position/orientation/velocity of all active sprites
void moveSprites(void);

// Advance timed sprite variables which update every frame
void advanceTimers(void);

// Render all active sprites to the screen
void renderSprites(void);

// Load sprite and spell data
void loadSpriteInfo(void);

// Unload any active sprites which have died
bool unloadSprites(void);

// Destroy all active sprites
void freeActiveSprites(void);

// Free sprite and spell data
void freeSpriteInfo(void);
