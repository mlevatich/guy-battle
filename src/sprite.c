#include "../headers/constants.h"
#include "../headers/sprite.h"

// Struct for sprite meta information
typedef struct sprite_metainfo
{
    short width;
    short height;
    short sheet_position;
    char* frame_sections;
    char type;
    char id;
}* SpriteInfo;

// Struct for spell meta information
typedef struct spell_metainfo
{
    char action;
    char cast_time;
    char finish_time;
    short cooldown;
    void (*launch)(Sprite);
}* SpellInfo;

// Struct for a currently active sprite
struct sprite
{
    // Meta info
    SpriteInfo meta;

    // Positional info
    double x_pos;
    double y_pos;
    double x_vel;
    double y_vel;
    bool direction;
    short angle;

    // Action info
    char hp;
    char colliding;
    char casting;
    short* cooldowns;
    char spell;
    char action;
    bool action_change;
    double frame;
};

// Struct for a linked list of sprites
typedef struct ele
{
    Sprite sp;
    struct ele* next;
}* SpriteList;

#define NUM_SPRITES 4       // Number of distinct sprites in the game
#define NUM_SPELLS 2        // Number of distinct spells in the game

SDL_Texture* spriteSheet;   // Texture containing all sprites
SpriteList activeSprites;   // Linked list of currently active sprites
SpriteInfo* sprite_info;    // Array of meta info structs for sprites, indexed by identities enum (sprite.h)
SpellInfo* spell_info;      // Array of meta info structs for spells, indexed by identities enum (sprite.h)

/* SPRITE CONSTRUCTORS */

// Initialize a sprite with its on-screen location and stats
Sprite spawnSprite(char id, char hp, double x, double y, double x_vel, double y_vel, int act, int dir, int angle)
{
    // Set sprite fields
    Sprite sp = (Sprite) malloc(sizeof(struct sprite));
    sp->meta = sprite_info[(int)id];
    sp->frame = sp->meta->frame_sections[(int)act];
    sp->direction = dir; sp->angle = angle;
    sp->hp = hp;         sp->cooldowns = NULL;
    sp->x_pos = x;       sp->y_pos = y;
    sp->x_vel = x_vel;   sp->y_vel = y_vel;
    sp->casting = 0;     sp->colliding = 0;
    sp->spell = 0;       sp->action = act;
    if(sp->meta->type == HUMANOID) sp->cooldowns = (short*) calloc(NUM_SPELLS, sizeof(short));

    // Add sprite to linked list of active sprites
    struct ele* new_sprite = (struct ele*) malloc(sizeof(struct ele));
    new_sprite->sp = sp;
    new_sprite->next = NULL;
    if(activeSprites != NULL) new_sprite->next = activeSprites;
    activeSprites = new_sprite;
    return sp;
}

/* SETTERS */

// Set a sprite's action
static void setAction(Sprite sp, int action)
{
    sp->action_change = (sp->action != action);
    sp->action = action;
}

// Teleport a sprite to a different location
void setPosition(Sprite sp, double x, double y)
{
    sp->x_pos = x;
    sp->y_pos = y;
}

// Hide a guy in the top right corner of the screen (Guys can't be despawned)
void hideGuy(Sprite guy)
{
    setPosition(guy, SCREEN_WIDTH+20, 0);
    guy->x_vel = 0;
    guy->y_vel = 0;
}

// Reset the fields of the Guys after a match ends
void resetGuys(Sprite guy1, Sprite guy2)
{
    guy1->hp = 100;
    guy2->hp = 100;
    for(int i = 0; i < NUM_SPELLS; i++)
    {
        guy1->cooldowns[i] = 0;
        guy2->cooldowns[i] = 0;
    }
    guy1->x_vel = 0;
    guy1->y_vel = 0;
    guy2->x_vel = 0;
    guy2->y_vel = 0;
    guy1->direction = RIGHT;
    guy2->direction = LEFT;
}

/* GETTERS */

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(Sprite guy)
{
    // Make sure the malloc'd array is still collected even if the Guy doesn't exist
    double* cooldown_percentages = (double*) malloc(sizeof(double) * (NUM_SPELLS + 1));
    if(!guy) return cooldown_percentages;

    // Get cooldown percentages
    for(int i = 0; i < NUM_SPELLS; i++)
    {
        cooldown_percentages[i] = (double) guy->cooldowns[i] / (double) spell_info[i]->cooldown;
    }

    // Hack to denote an end of the array
    cooldown_percentages[NUM_SPELLS] = -1;
    return cooldown_percentages;
}

// Get a guy's health remaining
int getHealth(Sprite guy)
{
    // Make sure something is returned even if the Guy doesn't exist
    if(!guy) return 0;
    return guy->hp;
}

// Return true if a sprite is touching the ground
static bool onGround(Sprite sp, int* platforms)
{
    int middle = (sp->x_pos + (double)sp->meta->width/2);
    return (sp->y_pos + sp->meta->height >= platforms[1]) && (middle > platforms[2] && middle < platforms[3]);
}

// Return -1 unless sprite has landed on a platform (including the ground)
static int onPlatform(Sprite sp, int* platforms)
{
    int numPlatforms = platforms[0];
    int middle = (sp->x_pos + (double)sp->meta->width/2);
    for(int i = 1; i < numPlatforms*3 + 1; i += 3)
    {
        // Platform land check - AABB and a positive y-velocity
        if(sp->y_vel >= 0 && fabs(platforms[i] - (sp->y_pos + sp->meta->height)) <= fabs(sp->y_vel)
        && middle > platforms[i+1] && middle < platforms[i+2])
        {
            // Return a new position for the sprite such that it is directly on the platform
            return platforms[i] - sp->meta->height;
        }
    }
    return -1;
}

// Return -1 unless sprite is touching a wall
static int touchingWall(Sprite sp, int* walls)
{
    int numWalls = walls[0];
    for(int i = 1; i < numWalls*3 + 1; i += 3)
    {
        // AABB check - if it passes, there's a wall collision
        if(walls[i] < sp->x_pos + sp->meta->width && walls[i] > sp->x_pos
        && walls[i+1] < sp->y_pos + sp->meta->height && walls[i+2] > sp->y_pos)
        {
            // Determine which side of the wall was collided with and return a new position
            // for the sprite such that it would no longer be inside the wall
            if(fabs(walls[i] - sp->x_pos) < fabs(walls[i] - (sp->x_pos + sp->meta->width)))
            {
                return walls[i];
            }
            else
            {
                return walls[i] - sp->meta->width;
            }
        }
    }
    return -1;
}

// Checks if an active sprite is dead and needs to be unloaded
static bool isDead(Sprite sp)
{
    // If a sprite is too far off screen, it's dead
    double x = sp->x_pos;
    double y = sp->y_pos;
    if(x < -500 || x > SCREEN_WIDTH+500 || y <= -500 || y >= SCREEN_HEIGHT+100) return 1;

    // If a spell has finished its collision animation, it's dead
    if((sp->meta->type == SPELL || sp->meta->type == PARTICLE) && sp->colliding == 1) return 1;

    // If a Guy sprite has finished the death animation, it's dead
    // TODO

    return 0;
}

/* SPRITE ACTIONS */

// Attempt to walk in a direction after a keyboard input
bool walk(Sprite guy, int direction)
{
    // Guy can only walk if he's not casting or colliding (can still move left/right in midair)
    if(!(guy->casting || guy->colliding))
    {
        // Update velocity and direction facing based on direction of walk
        if(direction == LEFT)
        {
            guy->x_vel = fmax(guy->x_vel - 0.4, -4.0);
        }
        else
        {
            guy->x_vel = fmin(guy->x_vel + 0.4, 4.0);
        }
        guy->direction = direction;
        if (guy->action != JUMP) setAction(guy, MOVE);
        return 1;
    }
    return 0;
}

// Attempt to jump after a keyboard input
bool jump(Sprite guy)
{
    // Guy can only jump if he's not casting, colliding, or jumping
    if(!(guy->casting || guy->colliding) && guy->action != JUMP)
    {
        guy->y_vel += -10;
        setAction(guy, JUMP);
        return 1;
    }
    return 0;
}

// Attempt to cast a spell after a keyboard input
bool cast(Sprite guy, int spell)
{
    // Guy can only cast a spell if it's off cooldown and he's not casting, colliding, or jumping
    if(!(guy->casting || guy->colliding) && !guy->cooldowns[spell] && guy->action != JUMP)
    {
        guy->casting = spell_info[spell]->cast_time;
        guy->spell = spell;
        setAction(guy, spell_info[spell]->action);
        return 1;
    }
    return 0;
}

// Action function for launching a fireball (stored as fxn ptr in spellInfo)
static void Fireball(Sprite sp)
{
    // Starting position and velocity of the fireball
    double fire_x_pos = sp->x_pos + convert(sp->direction) * 20;
    double fire_x_vel = convert(sp->direction) * 1;

    // Spawn the fireball and return score change
    spawnSprite(FIREBALL, 1, fire_x_pos, sp->y_pos+28, fire_x_vel, 0, MOVE, sp->direction, 0);
}

// Action function for launching an iceshock (stored as fxn ptr in spellInfo)
static void Iceshock(Sprite sp)
{
    for(double i = 0; i < 4; i++)
    {
        // Inner missile or outer missile speed/displacement
        double x_speed = 0, y_speed = -6, x_dist = 0, y_dist = 0;
        if(i == 0 || i == 3)
        {
            x_speed = 5;
            x_dist = 15;
            y_dist = 5;
        }
        else
        {
            x_speed = 2.5;
            x_dist = 5;
            y_dist = 15;
        }

        // Starting orientation/side-of-caster of the missile
        int direction = (i > 1.5);
        int side = convert(direction);
        int angle = (short) (57.296 * atan(y_speed / (side * x_speed)));

        // Starting position of the missile
        double ice_xpos = (side*x_dist)+sp->x_pos+sp->meta->width/4-3;
        double ice_ypos = sp->y_pos-y_dist;

        // Spawn one missile and four small particles around it
        spawnSprite(ICESHOCK, 1, ice_xpos, ice_ypos, side * x_speed, y_speed, MOVE, direction, angle);
        for(int j = 0; j < 4; j++)
        {
            double ptc_x = ice_xpos + (get_rand() - 0.5) * 10;
            double ptc_y = ice_ypos + (get_rand() - 0.5) * 10;
            double ptc_xv = side * (x_speed * get_rand() + 2);
            double ptc_yv = y_speed * get_rand() - x_speed;
            spawnSprite(ICESHOCK_PARTICLE, 1, ptc_x, ptc_y, ptc_xv, ptc_yv, MOVE, direction, 0);
        }
    }
}

/* PER FRAME UPDATES */

// If a sprite is ready to launch a casted spell, launch it
static void launchSpell(Sprite sp)
{
    // Only human sprites can cast spells
    if(sp->meta->type == HUMANOID)
    {
        // Iterate over all possible spells
        int spell = sp->spell;
        for(int i = 0; i < NUM_SPELLS; i++)
        {
            // Set cooldown and launch the spell if sprite has finished its casting animation
            if(spell == i && sp->casting == spell_info[spell]->finish_time)
            {
                sp->cooldowns[spell] = spell_info[spell]->cooldown;
                spell_info[spell]->launch(sp);
            }
        }
    }
}

// All sprites launch any spells they are ready to launch
void launchSpells()
{
    // Iterate over active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        launchSpell(cursor->sp);
    }
}

// Detect and handle collisions in this frame for a sprite
static void checkCollision(Sprite sp, int* platforms, int* walls)
{
    // Convenience variables
    int type = sp->meta->type;
    int wall = touchingWall(sp, walls);
    int floor = onPlatform(sp, platforms);
    int ground = onGround(sp, platforms);

    // Different sprite types handle collisions differently
    switch(type)
    {
        case HUMANOID:
            // Humans are stopped by walls
            if(wall != -1)
            {
                sp->x_vel = 0;
                sp->x_pos = wall;
            }

            // (Falling) humans are stopped by platforms
            if(floor != -1)
            {
                sp->y_vel = 0;
                sp->y_pos = floor;
                if(sp->action == JUMP) setAction(sp, MOVE);
            }
            else
            {
                // If a sprite isn't touching a platform, it's jumping
                if(sp->action != JUMP) setAction(sp, JUMP);
            }

            // Humans take damage and knockback from spells (and destroy the spell)
            // if(!sp->colliding && TODO)
            // {
            //     sp->colliding = 13;
            //     sp->x_vel = -3;
            //     sp->y_vel = -2;
            //     setAction(sp, COLLIDE);
            // }
            break;

        case SPELL:
            // Spells collide with ground and walls (and humans, but this is handled above)
            if(!sp->colliding && (ground || wall != -1))
            {
                // Spells enter collision animation and are slowed down on collision
                sp->colliding = 20;
                sp->x_vel *= 0.05;
                sp->y_vel *= 0.05;
                setAction(sp, COLLIDE);
            }
            break;

        case PARTICLE:
            // Particles collide with ground and walls
            if(!sp->colliding && (ground || wall != -1))
            {
                // Particles die immediately on collision
                sp->colliding = 2;
                setAction(sp, COLLIDE);
            }
            break;
    }
}

// Check for and handle collisions for all active sprites
void checkCollisions(int* platforms, int* walls)
{
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        checkCollision(cursor->sp, platforms, walls);
    }
}

// Update the animation frame (picture that gets drawn) for a sprite
static void updateAnimationFrame(Sprite sp)
{
    // Sprite proceeds through animation frames faster during certain actions
    double increment = ANIMATION_SPEED * 0.1;
    if(sp->action == JUMP || sp->action == COLLIDE) increment *= 1.5;
    if(sp->action == CAST_FIREBALL || sp->action == CAST_ICESHOCK) increment *= 2.5;
    sp->frame += increment;

    // If the sprite's action has just changed, reset to first animation frame of that action
    if(sp->action_change)
    {
        sp->frame = sp->meta->frame_sections[(int)sp->action];
        if(sp->action == MOVE && sp->meta->id == GUY) sp->frame++;
        sp->action_change = false;
    }

    // Wraparound to first animation frame of an action if we reach the last frame for that action
    if(sp->frame >= sp->meta->frame_sections[sp->action+1])
    {
        sp->frame = sp->meta->frame_sections[(int)sp->action];
    }
}

// Update the animation frame which is drawn for all active sprites
void updateAnimationFrames()
{
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        updateAnimationFrame(cursor->sp);
    }
}

// Calculate physics and update position/velocity/orientation for a sprite
static void moveSprite(Sprite sp)
{
    // Update the sprite's position
    sp->x_pos += sp->x_vel;
    sp->y_pos += sp->y_vel;

    // Update the sprite's velocity and orientation (the physics are different for different spells)
    switch(sp->meta->id)
    {
        case FIREBALL:
            // Fireball accelerates, isn't affected by gravity,
            if(!sp->colliding) sp->x_vel += convert(sp->x_vel > 0.0f)*0.1;

            // Fireball faces in the direction of x-velocity (LEFT and RIGHT are in an enum so this works)
            sp->direction = (sp->x_vel > 0);
            break;

        case ICESHOCK:
        case ICESHOCK_PARTICLE:
            // Iceshock is affected by gravity and air resistance
            if(!sp->colliding) sp->y_vel += 0.3;
            sp->x_vel += convert(sp->x_vel < 0.0f) * 0.03;
            sp->direction = (sp->x_vel > 0);

            // Iceshock faces in the direction of xy-velocity
            sp->angle = (short) (57.296 * atan(sp->y_vel / sp->x_vel));
            break;

        default:
            // Update x velocity
            if(fabs(sp->x_vel) <= 0.3 && sp->action != JUMP && !sp->casting && !sp->colliding)
            {
                // Stop sprite completely if speed is low enough
                sp->x_vel = 0;
                setAction(sp, IDLE);
            }
            else
            {
                // Otherwise apply friction / air resistance
                sp->x_vel += convert(sp->x_vel < 0.0f)*0.15;
            }

            // Update y velocity (terminal velocity of 50)
            sp->y_vel = fmin(sp->y_vel + 0.5, 50);
            break;
    }
}

// Calculate physics and update position and orientation for all active sprites
void moveSprites()
{
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        moveSprite(cursor->sp);
    }
}

// Advance timed variables for this sprite
static void advanceTimer(Sprite sp)
{
    // Update casting time
    if(sp->casting)
    {
        sp->casting--;
        if(sp->casting == 0) setAction(sp, MOVE);
    }

    // Update cooldowns
    if(sp->meta->type == HUMANOID)
    {
        for(int i = 0; i < NUM_SPELLS; i++)
        {
            if(sp->cooldowns[i]) sp->cooldowns[i]--;
        }
    }

    // Update collision time
    if(sp->colliding)
    {
        sp->colliding--;
        if(sp->colliding == 0) setAction(sp, MOVE);
    }
}

// Advance timed sprite variables which update every frame
void advanceTimers()
{
    // Iterate over active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        advanceTimer(cursor->sp);
    }
}

// Render a sprite from the sprite sheet to the screen
static void renderSprite(Sprite sp)
{
    // Make sure sprite is facing the proper direction
    SDL_RendererFlip flipType = SDL_FLIP_NONE;
    if (sp->direction == LEFT) flipType = SDL_FLIP_HORIZONTAL;

    // Grab the sprite at it's current frame from the spritesheet
    SDL_Rect clip = {sp->meta->width * (int) sp->frame, sp->meta->sheet_position, sp->meta->width, sp->meta->height};

    // Draw the sprite at its current x and y position
    SDL_Rect renderQuad = {(int)sp->x_pos, (int)sp->y_pos, sp->meta->width, sp->meta->height};
    SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, flipType);
}

// Render all active sprites to the screen
void renderSprites()
{
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        renderSprite(cursor->sp);
    }
}

/* DATA ALLOCATION / INITIALIZATION */

// Assign meta info fields for a spell
static SpellInfo initSpell(char action, char cast_time, char finish_time, short cooldown, void (*launch_fxn)(Sprite))
{
    SpellInfo this_spell = (SpellInfo) malloc(sizeof(struct spell_metainfo));
    this_spell->action = action;
    this_spell->cast_time = cast_time;
    this_spell->finish_time = finish_time;
    this_spell->cooldown = cooldown;
    this_spell->launch = launch_fxn;
    return this_spell;
}

// Assign meta info fields for a sprite
static SpriteInfo initSprite(char id, char type, short w, short h,  short sheet_pos, char* frame_sections)
{
    SpriteInfo this_sprite = (SpriteInfo) malloc(sizeof(struct sprite_metainfo));
    this_sprite->id = id;
    this_sprite->type = type;
    this_sprite->width = w;
    this_sprite->height = h;
    this_sprite->sheet_position = sheet_pos;
    this_sprite->frame_sections = frame_sections;
    return this_sprite;
}

// Fill the meta info lists with complete meta info for each sprite in the game
void loadSpriteInfo()
{
    // Load the spritesheet texture into memory
    spriteSheet = loadTexture("art/Spritesheet.bmp");

    // Make space for meta info structs
    sprite_info = (SpriteInfo*)malloc(sizeof(SpriteInfo)*NUM_SPRITES);
    spell_info =(SpellInfo*)malloc(sizeof(SpellInfo)*NUM_SPELLS);

    // Initialize meta info for sprites
    char* f1=(char*)malloc(sizeof(char)*3); memcpy(f1, (char[]){0, 2, 5}, 3);
    sprite_info[FIREBALL] = initSprite(FIREBALL, SPELL, 23, 10, 60, f1);

    char* f2=(char*)malloc(sizeof(char)*3); memcpy(f2, (char[]){0, 2, 5}, 3);
    sprite_info[ICESHOCK] = initSprite(ICESHOCK, SPELL, 23, 10, 70, f2);

    char* f3=(char*)malloc(sizeof(char)*3); memcpy(f3, (char[]){0, 2, 2}, 3);
    sprite_info[ICESHOCK_PARTICLE] = initSprite(ICESHOCK_PARTICLE, PARTICLE, 5, 5, 80, f3);

    char* f4=(char*)malloc(sizeof(char)*7); memcpy(f4, (char[]){0, 4, 5, 10, 14, 22, 30}, 7);
    sprite_info[GUY] = initSprite(GUY, HUMANOID, 28, 60, 0, f4);

    // Initialize meta info for spells
    spell_info[FIREBALL] = initSpell(CAST_FIREBALL, 32, 8, 120, Fireball);
    spell_info[ICESHOCK] = initSpell(CAST_ICESHOCK, 32, 8, 300, Iceshock);
}

/* DATA UNLOADING */

// Free a sprite
static void freeSprite(struct ele* e)
{
    if(e->sp->meta->type == HUMANOID)
    {
        free(e->sp->cooldowns);
    }
    free(e->sp);
    free(e);
}

// Free any active sprites which have died
bool unloadSprites()
{
    // Iterate over active sprites
    struct ele* prev = NULL;
    bool game_over = false;
    for(struct ele* cursor = activeSprites; cursor != NULL;)
    {
        // Check if the sprite is dead
        if(isDead(cursor->sp))
        {
            if(cursor->sp->meta->id == GUY)
            {
                // If the dead sprite is a Guy, don't unload it, just signal game over
                game_over = true;
                hideGuy(cursor->sp);
                if(cursor->sp->y_pos >= SCREEN_HEIGHT+100)
                {
                    setPosition(cursor->sp, SCREEN_WIDTH+20, 0);
                }
            }
            else
            {
                // Otherwise remove the sprite from the active sprites and free it
                if(prev == NULL)
                {
                    prev = cursor;
                    cursor = cursor->next;
                    freeSprite(prev);
                    activeSprites = cursor;
                    prev = NULL;
                }
                else
                {
                    struct ele* e = cursor;
                    cursor = cursor->next;
                    freeSprite(e);
                    prev->next = cursor;
                }
            }
        }
        else
        {
            // If the sprite isn't dead, continue
            prev = cursor;
            cursor = cursor->next;
        }
    }
    return game_over;
}

// Free all active sprites
void freeActiveSprites()
{
    for(struct ele* cursor = activeSprites; cursor != NULL;)
    {
        struct ele* e = cursor;
        cursor = cursor->next;
        freeSprite(e);
    }
}

// Free all sprite and spell meta info
void freeSpriteInfo()
{
    // Free sprite metainfo
    for(int i = 0; i < NUM_SPRITES; i++)
    {
        free(sprite_info[i]->frame_sections);
        free(sprite_info[i]);
    }
    free(sprite_info);

    // Free spell metainfo
    for(int i = 0; i < NUM_SPELLS; i++)
    {
        free(spell_info[i]);
    }
    free(spell_info);

    // Free the sprite sheet texture
    SDL_DestroyTexture(spriteSheet);
}
