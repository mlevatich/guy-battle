/*
 Sprite control

 A sprite is a spell, particle, or player character. Sprite control thus
 includes physics, animation, collision, and generally the bulk of the
 game's code.
 */

// Sprite list - doubles as the spell list, so spells must come first
enum identities
{ FIREBALL, ICESHOCK, ROCKFALL, DARKEDGE, ICESHOCK_P1, ROCKFALL_P1, ROCKFALL_P2, DARKEDGE_P1, GUY };

// Possible action states for a sprite
enum action_types
{ SPAWN, MOVE, COLLIDE, IDLE, JUMP, CAST_FIREBALL, CAST_ICESHOCK, CAST_ROCKFALL, CAST_DARKEDGE, DIE };

// Sprite types
enum types
{ HUMANOID, PARTICLE, SPELL };

// Allow main to pass around Guy sprites
typedef struct sprite* Sprite;

// Spawn (construct) a sprite with the given fields
void spawnSprite(int id, double x, double y, double xv, double yv, bool dir, int angle, int spawning);

// Hide a guy in the top right corner of the map
void hideGuy(int guy);

// Reset the health and cooldowns and position of a guy
void resetGuy(int guy, int x, int y);

// Get a guy's health remaining
int getHealth(int guy);

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(int guy);

// Attempt to walk in a direction after a keyboard input
bool walk(int guy, bool direction);

// Attempt to jump after a keyboard input
bool jump(int guy);

// Attempt to cast a spell after a keyboard input
bool cast(int guy, int spell);

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
