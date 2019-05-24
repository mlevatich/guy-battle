#include "../headers/constants.h"
#include "../headers/sprite.h"

// Struct for sprite meta information
typedef struct sprite_metainfo
{
    short width;            // width in pixels
    short height;           // height in pixels
    short radius;           // radius in pixels, for collision checking
    short num_bounds;       // number of bounding boxes
    SDL_Rect* rbounds;      // arrays of bounding boxes (one for each direction), for collision checking
    SDL_Rect* lbounds;      // with origin in the upper-left, given relative to the sprite's xy-position
    short sheet_position;   // y-position of sprite on the sprite sheet
    char* frame_sections;   // array containing the number of animation frames for each sprite action
    short power;            // how much damage this sprite does in a collision
    short max_hp;           // the maximum hp of the sprite
    char type;              // what kind of sprite is this (HUMANOID, SPELL, PARTICLE)
    char id;                // what sprite is this (FIREBALL, GUY, etc)
}* SpriteInfo;

// Struct for spell meta information
typedef struct spell_metainfo
{
    char action;            // what is the casting animation for this spell
    char cast_time;         // how long does it take to cast this spell
    char finish_time;       // at what point in the casting animation is the spell launched
    short cooldown;         // how many frames before the spell is available again
    void (*launch)(Sprite); // function that's called when the spell is launched
}* SpellInfo;

// Struct for a currently active sprite
struct sprite
{
    // Meta info
    SpriteInfo meta;        // meta info for this sprite (see above)

    // Positional info
    double x_pos;           // in-game x-coord
    double y_pos;           // in-game y-coord
    double x_vel;           // x-velocity
    double y_vel;           // y-velocity
    bool direction;         // direction currently facing
    short angle;            // angle of orientation

    // Action info
    short hp;               // current hp
    char spawning;          // number of frames left in spawn animation
    char colliding;         // number of frames left in collision
    char casting;           // number of frames left to cast spell
    short* cooldowns;       // array of spell cooldowns
    char spell;             // spell currently in use
    char action;            // which animation is the sprite in (MOVE, JUMP, etc)
    bool action_change;     // has sprite's action changed to a different one this frame
    double frame;           // which animation frame should be rendered on the sprite sheet
};

// Struct for a linked list of sprites
typedef struct ele
{
    Sprite sp;              // sprite at this node
    struct ele* next;       // next node
}* SpriteList;

#define NUM_SPRITES 4       // Number of distinct sprites in the game
#define NUM_SPELLS 2        // Number of distinct spells in the game

SDL_Texture* spriteSheet;   // Texture containing all sprites
SpriteList activeSprites;   // Linked list of currently active sprites
SpriteInfo* sprite_info;    // Array of meta info structs for sprites, indexed by identities enum (sprite.h)
SpellInfo* spell_info;      // Array of meta info structs for spells, indexed by identities enum (sprite.h)

/* SPRITE CONSTRUCTOR */

// Initialize a sprite with its on-screen location and stats
Sprite spawnSprite(char id, double x, double y, double xv, double yv, int act, bool dir, int angle, char spawning)
{
    // Set sprite fields
    Sprite sp = (Sprite) malloc(sizeof(struct sprite));
    sp->meta = sprite_info[(int)id];
    sp->frame = sp->meta->frame_sections[(int)act];
    sp->hp = sp->meta->max_hp;
    sp->angle = angle; sp->direction = dir;
    sp->x_pos = x;     sp->y_pos = y;
    sp->x_vel = xv;    sp->y_vel = yv;
    sp->casting = 0;   sp->colliding = 0;
    sp->spell = 0;     sp->action = act;
    sp->spawning = spawning;
    sp->action_change = true;

    // Only human sprites have cooldowns
    sp->cooldowns = NULL;
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
    if(sp->action != action) sp->action_change = true;
    sp->action = action;
}

// Teleport a sprite to a different location
void setPosition(Sprite sp, double x, double y)
{
    sp->x_pos = x;
    sp->y_pos = y;
}

// Remove a sprite's velocity
static void stopSprite(Sprite sp)
{
    sp->x_vel = 0;
    sp->y_vel = 0;
}

// Hide a guy in the top right corner of the screen (Guys can't be despawned)
void hideGuy(Sprite guy)
{
    setPosition(guy, SCREEN_WIDTH+20, 0);
    stopSprite(guy);
    guy->hp = 1;
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
    stopSprite(guy1);
    stopSprite(guy2);
    guy1->direction = RIGHT;
    guy2->direction = LEFT;
}

/* GETTERS */

// Get the x coordinate of a sprite's center
static double xCenter(Sprite sp)
{
    return (sp->x_pos + (double)sp->meta->width/2);
}

// Get the y coordinate of a sprite's center
static double yCenter(Sprite sp)
{
    return (sp->y_pos + (double)sp->meta->height/2);
}

// Get which bounding boxes should be used by this sprite
static SDL_Rect* getBounds(Sprite sp)
{
    if(sp->direction == RIGHT) return sp->meta->rbounds;
    return sp->meta->lbounds;
}

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
    int middle = xCenter(sp);
    return (sp->y_pos + sp->meta->height >= platforms[1]) && (middle > platforms[2] && middle < platforms[3]);
}

// Return -1 unless sprite has landed on a platform (including the ground)
static int onPlatform(Sprite sp, int* platforms)
{
    int numPlatforms = platforms[0];
    int middle = xCenter(sp);
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

    // If a sprite is out of hp and has finished its collision animation, it's dead
    if(sp->hp == 0 && sp->colliding == 1) return 1;

    return 0;
}

/* SPRITE ACTIONS */

// Attempt to walk in a direction after a keyboard input
bool walk(Sprite guy, bool direction)
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
        guy->y_vel += -10.1;
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
    spawnSprite(FIREBALL, fire_x_pos, sp->y_pos+28, fire_x_vel, 0, MOVE, sp->direction, 0, 0);
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
        spawnSprite(ICESHOCK, ice_xpos, ice_ypos, side * x_speed, y_speed, MOVE, direction, angle, 0);
        for(int j = 0; j < 4; j++)
        {
            double ptc_x = ice_xpos + (get_rand() - 0.5) * 10;
            double ptc_y = ice_ypos + (get_rand() - 0.5) * 10;
            double ptc_xv = side * (x_speed * get_rand() + 2);
            double ptc_yv = y_speed * get_rand() - x_speed;
            spawnSprite(ICESHOCK_PARTICLE, ptc_x, ptc_y, ptc_xv, ptc_yv, MOVE, direction, 0, 0);
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

// Check if sprites are in the vicinity of one another with easy bounding circle check
static bool boundingCircleCheck(Sprite sp, Sprite other)
{
    // Get x and y distances of sprites from each other
    double x_dist = xCenter(sp) - xCenter(other);
    double y_dist = yCenter(sp) - yCenter(other);

    // Compare the distance squared with the sum of the radii squared
    double distance_squared = x_dist * x_dist + y_dist * y_dist;
    double rad_sum = sp->meta->radius + other->meta->radius;

    // If they're close enough, return true so we can do bounding box check
    if((rad_sum * rad_sum) <= distance_squared) return false;
    return true;
}

// Precisely check if sprites are touching by comparing their arrays of bounding boxes
static bool boundingBoxesCheck(Sprite sp, Sprite other)
{
    // Nested for loop to compare each box of sp with each box of other
    SDL_Rect* b1 = getBounds(sp);
    SDL_Rect* b2 = getBounds(other);
    for(int i = 0; i < sp->meta->num_bounds; i++)
    {
        int x1 = b1[i].x + sp->x_pos;
        int y1 = b1[i].y + sp->y_pos;
        for(int j = 0; j < other->meta->num_bounds; j++)
        {
            // AABB
            int x2 = b2[j].x + other->x_pos;
            int y2 = b2[j].y + other->y_pos;
            if((x1 < x2 + b2[j].w && x1 + b1[i].w > x2) && (y1 < y2 + b2[j].h && y1 + b1[i].h > y2))
            {
                return true;
            }
        }
    }
    return false;
}

// Process a collision between two sprites
static void applyCollision(Sprite sp, Sprite other)
{
    // All sprites take damage from collisions
    sp->hp = fmax(0, sp->hp - other->meta->power);

    // Get which direction the collision is coming from
    int direction = convert(other->x_pos > sp->x_pos);

    // Humans are knocked back by collisions
    if(sp->meta->type == HUMANOID)
    {
        sp->colliding = 20;
        sp->x_vel = -5*direction;
        sp->y_vel = -3;
        sp->casting = 0;
    }

    // Spells are slowed down on collision
    if(sp->meta->type == SPELL)
    {
        sp->colliding = 20;
        sp->x_vel *= 0.05;
        sp->y_vel *= 0.05;
        sp->casting = 0;
    }
}

// Detect and handle all collisions between sprites in this frame
void spriteCollisions()
{
    // Iterate over all active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        // Particles don't collide with other sprites
        Sprite sp = cursor->sp;
        if(sp->meta->type == PARTICLE) continue;

        // Colliding sprites can't collide again
        if(sp->colliding) continue;

        // Otherwise, need to check for a collision against every other sprite
        for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
        {
            // Colliding sprites can't collide again
            Sprite other = cursor->sp;
            if(other->colliding) continue;

            // Sprites don't collide with themselves
            if(sp == other) continue;

            // Particles don't collide with other sprites
            if(other->meta->type == PARTICLE) continue;

            // Humans don't collide with other humans
            if(other->meta->type == HUMANOID && sp->meta->type == HUMANOID) continue;

            // Bounding circle check – if two sprites aren't even close to each other, don't bother
            if(!boundingCircleCheck(sp, other)) continue;

            // If circle check passes, do more precise bounding box array check
            if(!boundingBoxesCheck(sp, other)) continue;

            // Apply the effects of the collision to both sprites
            applyCollision(sp, other);
            applyCollision(other, sp);
        }
    }
}

// Detect and handle terrain collisions in this frame for a sprite
static void terrainCollision(Sprite sp, int* platforms, int* walls)
{
    // Precomputation
    int type = sp->meta->type;
    int touching_wall = touchingWall(sp, walls);
    int on_platform = onPlatform(sp, platforms);
    int on_ground = onGround(sp, platforms);

    // Different sprite types handle terrain collisions differently
    switch(type)
    {
        case HUMANOID:
            // Humans are stopped by walls
            if(touching_wall != -1)
            {
                sp->x_vel = 0;
                sp->x_pos = touching_wall;
            }

            // (Falling) humans are stopped by platforms
            if(on_platform != -1)
            {
                sp->y_vel = 0;
                sp->y_pos = on_platform;
            }
            break;

        case SPELL:
            // Spells collide with ground and walls
            if(!sp->colliding && (on_ground || touching_wall != -1))
            {
                // Spells are slowed down and killed on collision
                sp->hp = 0;
                sp->colliding = 20;
                sp->x_vel *= 0.05;
                sp->y_vel *= 0.05;
            }
            break;

        case PARTICLE:
            // Particles collide with ground and walls
            if(!sp->colliding && (on_ground || touching_wall != -1))
            {
                // Particles die immediately on collision
                sp->hp = 0;
                sp->colliding = 2;
            }
            break;
    }
}

// Check for and handle terrain collisions for all active sprites
void terrainCollisions(int* platforms, int* walls)
{
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        terrainCollision(cursor->sp, platforms, walls);
    }
}

// Update which animation action the sprite is currently in based on its state
static void updateAction(Sprite sp)
{
    if(sp->spawning)                                        setAction(sp, SPAWN);
    else if(sp->colliding)                                  setAction(sp, COLLIDE);
    else if(sp->casting)                                    setAction(sp, spell_info[(int)sp->spell]->action);
    else if(sp->x_vel == 0 && sp->y_vel == 0)               setAction(sp, IDLE);
    else if(sp->meta->type == HUMANOID && sp->y_vel != 0)   setAction(sp, JUMP);
    else                                                    setAction(sp, MOVE);
}

// Update the animation frame (picture that gets drawn) for a sprite
static void updateAnimationFrame(Sprite sp)
{
    // Update which action the sprite is currently in based on its state
    updateAction(sp);

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

            // Iceshock faces in the direction of xy-velocity
            sp->direction = (sp->x_vel > 0);
            sp->angle = (short) (57.296 * atan(sp->y_vel / sp->x_vel));
            break;

        default:
            // Update x velocity
            if(fabs(sp->x_vel) <= 0.3)
            {
                // Stop sprite completely if speed is low enough
                sp->x_vel = 0;
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
    if(sp->casting) sp->casting--;

    // Update spawning animation duration
    if(sp->spawning) sp->spawning--;

    // Update cooldowns
    if(sp->meta->type == HUMANOID)
    {
        for(int i = 0; i < NUM_SPELLS; i++)
        {
            if(sp->cooldowns[i]) sp->cooldowns[i]--;
        }
    }

    // Update collision time
    if(sp->colliding) sp->colliding--;
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

// Render a sprite's bounding boxes on top of the sprite (only in DEBUG_MODE)
static void renderBounds(Sprite sp)
{
    // For each box, render 4 lines to create the rectangle
    SDL_Rect* bounds = getBounds(sp);
    for(int i = 0; i < sp->meta->num_bounds; i++)
    {
        // Line 1
        SDL_Rect box = bounds[i];
        SDL_Rect clip = {344, 72, box.w, 1};
        SDL_Rect renderQuad = {(int)sp->x_pos + box.x, (int)sp->y_pos + box.y, box.w, 1};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, SDL_FLIP_NONE);

        // Line 2
        renderQuad = (SDL_Rect) {(int)sp->x_pos + box.x, (int)sp->y_pos + box.y + box.h, box.w, 1};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, SDL_FLIP_NONE);

        // Line 3
        clip = (SDL_Rect) {344, 72, 1, box.h};
        renderQuad = (SDL_Rect) {(int)sp->x_pos+ box.x, (int)sp->y_pos + box.y, 1, box.h};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, SDL_FLIP_NONE);

        // Line 4
        renderQuad = (SDL_Rect) {(int)sp->x_pos + box.x + box.w, (int)sp->y_pos + box.y, 1, box.h};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, sp->angle, NULL, SDL_FLIP_NONE);
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

    // In debug mode, render bounding boxes and sprite positions
    if(DEBUG_MODE && sp->meta->type != PARTICLE)
    {
        renderBounds(sp);
        clip = (SDL_Rect) {366, 67, 3, 3};
        renderQuad = (SDL_Rect) {(int)sp->x_pos, (int)sp->y_pos, 3, 3};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, 0, NULL, SDL_FLIP_NONE);
    }
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

// Reflect bounding boxes across the y-axis of a sprite to create lbounds
static SDL_Rect* reflectBounds(SDL_Rect* rbounds, int num_bounds, int width)
{
    // Iterate over all given bounds
    SDL_Rect* lbounds = malloc(sizeof(SDL_Rect) * num_bounds);
    double sprite_center = width / 2.0;
    for(int i = 0; i < num_bounds; i++)
    {
        // Width, height, and y-coordinate don't change
        SDL_Rect rbox = rbounds[i];
        lbounds[i].w = rbox.w;
        lbounds[i].h = rbox.h;
        lbounds[i].y = rbox.y;

        // reflect box i onto lbounds[i]
        double box_center = rbox.x + rbox.w / 2.0;
        double translation = 2 * (sprite_center - box_center);
        lbounds[i].x = rbox.x + (int) translation;
    }
    return lbounds;
}

// Assign meta info fields for a spell
static SpellInfo initSpell(char action, char cast_time, char finish_time, short cooldown, void (*launch_fxn)(Sprite))
{
    SpellInfo this_spell = (SpellInfo) malloc(sizeof(struct spell_metainfo));
    this_spell->action = action;
    this_spell->cast_time = cast_time;
    this_spell->finish_time = finish_time;
    this_spell->cooldown = cooldown;
    if(DEBUG_MODE) this_spell->cooldown = 0; // no cooldowns in debug mode
    this_spell->launch = launch_fxn;
    return this_spell;
}

// Assign meta info fields for a sprite
static SpriteInfo initSprite(char id, char type, short power, short hp, short width, short height,
                             short sheet_pos, char* fs, short num_bounds, SDL_Rect* bounds)
{
    SpriteInfo this_sprite = (SpriteInfo) malloc(sizeof(struct sprite_metainfo));
    this_sprite->id = id;
    this_sprite->type = type;
    this_sprite->power = power;
    this_sprite->max_hp = hp;
    this_sprite->width = width;
    this_sprite->height = height;
    this_sprite->radius = sqrt((width*width/4.0) + (height*height/4.0));
    this_sprite->num_bounds = num_bounds;
    this_sprite->rbounds = bounds;
    this_sprite->lbounds = reflectBounds(bounds, num_bounds, width);
    this_sprite->sheet_position = sheet_pos;
    this_sprite->frame_sections = fs;
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

    // Initialize meta info for spells
    spell_info[FIREBALL] = initSpell(CAST_FIREBALL, 32, 8, 120, Fireball);
    spell_info[ICESHOCK] = initSpell(CAST_ICESHOCK, 32, 8, 300, Iceshock);

    // Frame sections, bounding boxes, and other metadata for Fireball sprite
    char* fs1 = (char*) malloc(sizeof(char) * 3);
    memcpy(fs1, (char[]) {0, 0, 2, 5}, 4);

    SDL_Rect* bounds1 = malloc(sizeof(SDL_Rect) * 1);
    bounds1[0] = (SDL_Rect) {6, 2, 12, 6};

    sprite_info[FIREBALL] = initSprite(FIREBALL, SPELL, 20, 1, 23, 10, 60, fs1, 1, bounds1);

    // Frame sections, bounding boxes, and other metadata for Iceshock sprite
    char* fs2 = (char*) malloc(sizeof(char) * 3);
    memcpy(fs2, (char[]) {0, 0, 2, 5}, 4);

    SDL_Rect* bounds2 = malloc(sizeof(SDL_Rect) * 1);
    bounds2[0] = (SDL_Rect) {6, 1, 13, 7};

    sprite_info[ICESHOCK] = initSprite(ICESHOCK, SPELL, 30, 1, 23, 10, 70, fs2, 1, bounds2);

    // Frame sections and other metadata for iceshock particle sprite
    char* fs3 = (char*) malloc(sizeof(char) * 3);
    memcpy(fs3, (char[]) {0, 0, 2, 2}, 4);

    SDL_Rect* bounds3 = NULL;

    sprite_info[ICESHOCK_PARTICLE] = initSprite(ICESHOCK_PARTICLE, PARTICLE, 0, 1, 5, 5, 80, fs3, 0, bounds3);

    // Frame sections, bounding boxes, and other metadata for Guy sprite
    char* fs4 = (char*) malloc(sizeof(char) * 7);
    memcpy(fs4, (char[]) {0, 0, 4, 5, 10, 14, 22, 30}, 8);

    SDL_Rect* bounds4 = malloc(sizeof(SDL_Rect) * 2);
    bounds4[0] = (SDL_Rect) {9, 6, 15, 14};
    bounds4[1] = (SDL_Rect) {10, 24, 10, 35};

    sprite_info[GUY] = initSprite(GUY, HUMANOID, 10, 100, 28, 60, 0, fs4, 2, bounds4);
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
                // If the dead sprite is a Guy, just hide it and signal game over
                hideGuy(cursor->sp);
                game_over = true;
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
        if(sprite_info[i]->type != PARTICLE)
        {
            free(sprite_info[i]->rbounds);
            free(sprite_info[i]->lbounds);
        }
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
