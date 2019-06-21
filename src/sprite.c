#include "../headers/constants.h"
#include "../headers/sprite.h"

// Struct for sprite meta information
typedef struct sprite_metainfo
{
    int width;                  // width in pixels
    int height;                 // height in pixels
    int radius;                 // radius in pixels, for collision checking
    int num_bounds;             // number of bounding boxes
    SDL_Rect* rbounds;          // arrays of bounding boxes (one for each direction), for collision checking
    SDL_Rect* lbounds;          // with origin in the upper-left, given relative to the sprite's xy-position
    int sheet_position;         // y-position of sprite on the sprite sheet
    int* frame_sections;        // array containing the number of animation frames for each sprite action
    int power;                  // how much damage this sprite does in a collision
    int max_hp;                 // the maximum hp of the sprite
    int type;                   // what kind of sprite is this (HUMANOID, SPELL, PARTICLE)
    int id;                     // what sprite is this (FIREBALL, GUY, etc)
}* SpriteInfo;

// Struct for spell meta information
typedef struct spell_metainfo
{
    int action;                 // what is the casting animation for this spell
    int cast_time;              // how long does it take to cast this spell
    int finish_time;            // at what point in the casting animation is the spell launched
    int cooldown;               // how many frames before the spell is available again
    void (*on_launch)(Sprite);  // function that's called when the spell is launched
    void (*on_collide)(Sprite); // function that's called when the spell collides
}* SpellInfo;

// Struct for a currently active sprite
struct sprite
{
    // Meta info
    SpriteInfo meta;            // meta info for this sprite (see above)

    // Positional info
    double x_pos;               // in-game x-coord
    double y_pos;               // in-game y-coord
    double x_vel;               // x-velocity
    double y_vel;               // y-velocity
    bool direction;             // direction currently facing
    int angle;                  // angle of orientation

    // Action info
    int hp;                     // current hp
    int spawning;               // number of frames left in spawn animation
    int colliding;              // number of frames left in collision
    int casting;                // number of frames left to cast spell
    int lifetime;               // number of frames before this sprite dies automatically
    int* cooldowns;             // array of spell cooldowns
    int spell;                  // spell currently in use
    int action;                 // which animation is the sprite in (MOVE, JUMP, etc)
    bool action_change;         // has sprite's action changed to a different one this frame
    double frame;               // which animation frame should be rendered on the sprite sheet
};

// Struct for a linked list of sprites
typedef struct ele
{
    Sprite sp;                  // sprite at this node
    struct ele* next;           // next node
}* SpriteList;

#define NUM_SPRITES 9           // Number of distinct sprites in the game
#define NUM_SPELLS 4            // Number of distinct spells in the game

SDL_Texture* spriteSheet;       // Texture containing all sprites
SpriteList activeSprites;       // Linked list of currently active sprites
SpriteInfo* sprite_info;        // Array of meta info structs for sprites, indexed by identities enum (sprite.h)
SpellInfo* spell_info;          // Array of meta info structs for spells, indexed by identities enum (sprite.h)

Sprite guys[2] = {NULL, NULL};  // Permanent storage for the guy sprites

/* SPRITE CONSTRUCTOR */

// Initialize a sprite with its on-screen location and stats
void spawnSprite(int id, double x, double y, double xv, double yv, bool dir, int angle, int spawning, int life)
{
    // Set sprite fields
    Sprite sp = (Sprite) malloc(sizeof(struct sprite));
    sp->meta = sprite_info[id];
    sp->hp = sp->meta->max_hp;
    sp->angle = angle; sp->direction = dir;
    sp->x_pos = x;     sp->y_pos = y;
    sp->x_vel = xv;    sp->y_vel = yv;
    sp->casting = 0;   sp->colliding = 0;
    sp->spell = 0;     sp->spawning = spawning;
    sp->frame = 0;     sp->action = SPAWN;
    sp->lifetime = life;

    // Only human sprites have cooldowns
    sp->cooldowns = NULL;
    if(sp->meta->type == HUMANOID) sp->cooldowns = (int*) calloc(NUM_SPELLS, sizeof(int));

    // Add sprite to linked list of active sprites
    struct ele* new_sprite = (struct ele*) malloc(sizeof(struct ele));
    new_sprite->sp = sp;
    new_sprite->next = NULL;
    if(activeSprites != NULL) new_sprite->next = activeSprites;
    activeSprites = new_sprite;

    // If sprite is a guy store a reference to him
    if(sp->meta->id == GUY)
    {
        if(!guys[0]) guys[0] = sp;
        else         guys[1] = sp;
    }
}

/* SETTERS */

// Set a sprite's action
static void setAction(Sprite sp, int action)
{
    if(sp->action != action) sp->action_change = true;
    sp->action = action;
}

// Teleport a sprite to a different location
static void setPosition(Sprite sp, double x, double y)
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
void hideGuy(int guy)
{
    setPosition(guys[guy], SCREEN_WIDTH+20, 0);
    stopSprite(guys[guy]);
    guys[guy]->hp = 1;
}

// Reset the fields of the Guys after a match ends
void resetGuy(int guy, int x_pos, int y_pos)
{
    guys[guy]->hp = 100;
    guys[guy]->hp = 100;
    for(int i = 0; i < NUM_SPELLS; i++) guys[guy]->cooldowns[i] = 0;
    setPosition(guys[guy], x_pos, y_pos);
    stopSprite(guys[guy]);
    if(guy) guys[guy]->direction = LEFT;
    else    guys[guy]->direction = RIGHT;
}

/* GETTERS */

// Get an array of percentages of a guy's cooldowns
double* getCooldowns(int guy)
{
    // Make sure the malloc'd array is still collected even if the Guy doesn't exist
    double* cooldown_percentages = (double*) malloc(sizeof(double) * (NUM_SPELLS + 1));
    if(!guys[guy]) return cooldown_percentages;

    // Get cooldown percentages
    for(int i = 0; i < NUM_SPELLS; i++)
    {
        cooldown_percentages[i] = guys[guy]->cooldowns[i] / (double) spell_info[i]->cooldown;
    }

    // Hack to denote an end of the array
    cooldown_percentages[NUM_SPELLS] = -1;
    return cooldown_percentages;
}

// Get a guy's health remaining
int getHealth(int guy)
{
    // Make sure something is returned even if the Guy doesn't exist
    if(!guys[guy]) return 0;
    return guys[guy]->hp;
}

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

    // If a sprite has run out of lifetime, it's dead
    if(sp->lifetime == 1) return 1;

    return 0;
}

/* SPRITE EVENTS */

// In 1-player mode, process AI decisions
void takeCPUAction()
{
    // Human player
    int player = 0;
    Sprite player_guy = guys[player];

    // Cpu player
    int cpu = 1;
    Sprite cpu_guy = guys[cpu];

    // Walk towards player, but maintain a healthy distance
    int towards_player = cpu_guy->x_pos < player_guy->x_pos;
    if(fabs(cpu_guy->x_pos - player_guy->x_pos) >= 150) walk(cpu, towards_player);

    // Generally face the player
    if(cpu_guy->action == IDLE) cpu_guy->direction = towards_player;

    // Randomly jump
    if(get_rand() <= 0.003) jump(cpu);

    // Randomly cast spells
    if(get_rand() <= 0.015) cast(cpu, (int) (get_rand() * NUM_SPELLS));
}

// Attempt to walk in a direction after a keyboard input
bool walk(int guy, bool direction)
{
    // Guy can only walk if he's not casting or colliding (can still move left/right in midair)
    if(!(guys[guy]->casting || guys[guy]->colliding))
    {
        // Update velocity and direction facing based on direction of walk
        if(direction == LEFT)
        {
            guys[guy]->x_vel = fmax(guys[guy]->x_vel - 0.4, -4.0);
        }
        else
        {
            guys[guy]->x_vel = fmin(guys[guy]->x_vel + 0.4, 4.0);
        }
        guys[guy]->direction = direction;
        return 1;
    }
    return 0;
}

// Attempt to jump after a keyboard input
bool jump(int guy)
{
    // Guy can only jump if he's not casting, colliding, or jumping
    if(!(guys[guy]->casting || guys[guy]->colliding) && guys[guy]->action != JUMP)
    {
        guys[guy]->y_vel += -10.1;
        return 1;
    }
    return 0;
}

// Attempt to cast a spell after a keyboard input
bool cast(int guy, int spell)
{
    // Guy can only cast a spell if it's off cooldown and he's not casting, colliding, or jumping
    if(!(guys[guy]->casting || guys[guy]->colliding) && !guys[guy]->cooldowns[spell] && guys[guy]->action != JUMP)
    {
        guys[guy]->casting = spell_info[spell]->cast_time;
        guys[guy]->spell = spell;

        // For rockfall, guy should face in the direction of the other guy
        if(spell == ROCKFALL) guys[guy]->direction = (guys[guy]->x_pos <= guys[(int)!guy]->x_pos);
        return 1;
    }
    return 0;
}

// Generic actions for when any spell collides with something (always slows down and dies)
static void collideSpell(Sprite sp)
{
    sp->colliding = 20;
    sp->hp = 0;
    sp->x_vel *= 0.05;
    sp->y_vel *= 0.05;
}

// Action function for launching a fireball (stored as fxn ptr in spellInfo)
static void launchFireball(Sprite sp)
{
    // Starting position and velocity of the fireball
    double fire_x_pos = sp->x_pos + convert(sp->direction) * 20;
    double fire_x_vel = convert(sp->direction) * 1;

    // Spawn the fireball
    spawnSprite(FIREBALL, fire_x_pos, sp->y_pos+28, fire_x_vel, 0, sp->direction, 0, 0, 0);
}

// Action function for launching an iceshock (stored as fxn ptr in spellInfo)
static void launchIceshock(Sprite sp)
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
        int angle = (int) (57.296 * atan(y_speed / (side * x_speed)));

        // Starting position of the missile
        double ice_xpos = (side*x_dist)+sp->x_pos+sp->meta->width/4-3;
        double ice_ypos = sp->y_pos-y_dist;

        // Spawn one missile and four small particles around it
        spawnSprite(ICESHOCK, ice_xpos, ice_ypos, side * x_speed, y_speed, direction, angle, 0, 0);
        for(int j = 0; j < 4; j++)
        {
            double ptc_x = ice_xpos + (get_rand() - 0.5) * 10;
            double ptc_y = ice_ypos + (get_rand() - 0.5) * 10;
            double ptc_xv = side * (x_speed * get_rand() + 2);
            double ptc_yv = y_speed * get_rand() - x_speed;
            spawnSprite(ICESHOCK_P1, ptc_x, ptc_y, ptc_xv, ptc_yv, direction, 0, 0, 0);
        }
    }
}

// Action function for launching rockfall (stored as fxn ptr in spellInfo)
static void launchRockfall(Sprite sp)
{
    // Get position of the other guy
    int other_guy_idx = (sp == guys[0]);
    Sprite other_guy = guys[other_guy_idx];

    // Set starting position of rock
    int x = xCenter(other_guy) - sprite_info[ROCKFALL]->width / 2;
    x = fmin(fmax(x, 60), 964 - sprite_info[ROCKFALL]->width); // Avoid spawning inside trees on forest map
    int y = other_guy->y_pos - 250;

    // Spawn the rock
    spawnSprite(ROCKFALL, x, y, 0, -1, RIGHT, 0, 20, 0);
}

// Action function for a rockfall collision (stored as fxn ptr in spellInfo)
static void collideRockfall(Sprite sp)
{
    // Set collided and slow the sprite down
    collideSpell(sp);

    // Spawn particles
    for(int i = 0; i < 8; i++)
    {
        int x_dir = convert(i < 4);
        double x = xCenter(sp);
        double y = yCenter(sp);
        double xv = x_dir * sp->y_vel;
        double yv = sp->y_vel * -2;
        int a = get_rand();
        spawnSprite(ROCKFALL_P1, x+(get_rand()-0.5)*40, y, xv + x_dir*5*get_rand(), yv-7*get_rand(), 0, a, 0, 0);
        spawnSprite(ROCKFALL_P2, x+(get_rand()-0.5)*40, y, xv + x_dir*5*get_rand(), yv-7*get_rand(), 0, a, 0, 0);
        spawnSprite(ROCKFALL_P2, x+(get_rand()-0.5)*40, y, xv + x_dir*5*get_rand(), yv-7*get_rand(), 0, a, 0, 0);
    }
    return;
}

// Action function for launching darkedge (stored as fxn ptr in spellInfo)
static void launchDarkedge(Sprite sp)
{
    // base positions and velocity of spears
    double x_pos = sp->x_pos - (!sp->direction * 33);
    double y_pos = sp->y_pos - 45;

    double x_vel = 0.1 * convert(sp->direction);
    double y_vel = 0.025;

    // Spawn four dark spears above caster
    for(int i = 0; i < 4; i++)
    {
        int angle = (int) (57.296 * atan(y_vel / x_vel));
        spawnSprite(DARKEDGE, x_pos, y_pos - i*45, x_vel, y_vel, sp->direction, angle, 33, 0);
    }
    return;
}

/* PER FRAME UPDATES */

// If a human sprite is ready to launch a casted spell, launch it
static void launchSpell(Sprite sp)
{
    // Set cooldown and launch the spell if sprite has finished its casting animation
    int spell = sp->spell;
    if(sp->casting == spell_info[spell]->finish_time)
    {
        sp->cooldowns[spell] = spell_info[spell]->cooldown;
        spell_info[spell]->on_launch(sp);
    }
}

// Human sprites launch any spells they are ready to launch
void launchSpells()
{
    // Iterate over active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        if(cursor->sp->meta->type == HUMANOID) launchSpell(cursor->sp);
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
    int direction = convert(xCenter(other) > xCenter(sp));

    // Humans are knocked back by collisions, and spellcasts are cancelled
    if(sp->meta->type == HUMANOID)
    {
        sp->colliding = 20;
        sp->x_vel = -5 * direction;
        sp->y_vel = -3;
        sp->casting = 0;
    }

    // Spells have specialized collision handlers
    if(sp->meta->type == SPELL) spell_info[sp->meta->id]->on_collide(sp);
}

// Detect and handle all collisions between sprites in this frame
void spriteCollisions()
{
    // Iterate over all active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        // Colliding sprites, spawning sprites, and particles don't interact
        Sprite sp = cursor->sp;
        if(sp->meta->type == PARTICLE || sp->colliding || sp->spawning) continue;

        // Otherwise, need to check for a collision against every other sprite
        for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
        {
            // Colliding sprites, spawning sprites, and particles don't interact
            Sprite other = cursor->sp;
            if(other->meta->type == PARTICLE || other->colliding || other->spawning) continue;

            // Sprites don't collide with themselves and humans don't collide with other humans
            if(sp == other || (other->meta->type == HUMANOID && sp->meta->type == HUMANOID)) continue;

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
    int touching_wall = touchingWall(sp, walls);
    int on_platform = onPlatform(sp, platforms);
    int on_ground = onGround(sp, platforms);

    // Different sprite types handle terrain collisions differently
    switch(sp->meta->type)
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
            if(!sp->colliding && !sp->spawning && (on_ground || touching_wall != -1))
            {
                // Spells have specialized collision handlers
                spell_info[sp->meta->id]->on_collide(sp);
            }
            break;

        case PARTICLE:
            // Particles collide with ground and walls
            if(!sp->colliding && (on_ground || touching_wall != -1))
            {
                // Particles die immediately on terrain contact
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
    double xv = sp->x_vel;
    double yv = sp->y_vel;
    int type = sp->meta->type;
    if(type == HUMANOID && sp->hp == 0)             setAction(sp, DIE);
    else if(sp->spawning)                           setAction(sp, SPAWN);
    else if(sp->colliding)                          setAction(sp, COLLIDE);
    else if(sp->casting)                            setAction(sp, spell_info[sp->spell]->action);
    else if(type == HUMANOID && xv == 0 && yv == 0) setAction(sp, IDLE);
    else if(type == HUMANOID && yv != 0)            setAction(sp, JUMP);
    else                                            setAction(sp, MOVE);
}

// Update the animation frame (picture that gets drawn) for a sprite
static void updateAnimationFrame(Sprite sp)
{
    // Update which action the sprite is currently taking based on its state
    updateAction(sp);

    // Sprite proceeds through animation frames faster during certain actions
    int a = sp->action;
    double increment = ANIMATION_SPEED * 0.1;
    if(a == JUMP || a == COLLIDE || a == SPAWN) increment *= 1.5;
    if(a >= CAST_FIREBALL) increment *= 2.5;
    sp->frame += increment;

    // If the sprite's action has just changed, reset to first animation frame of that action
    if(sp->action_change)
    {
        sp->frame = sp->meta->frame_sections[a];
        if(a == MOVE && sp->meta->id == GUY) sp->frame++;
        sp->action_change = false;
    }

    // Wraparound to first animation frame of an action if we reach the last frame for that action
    if(sp->frame >= sp->meta->frame_sections[a+1])
    {
        sp->frame = sp->meta->frame_sections[a];
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
            if(!sp->colliding) sp->x_vel += convert(sp->x_vel > 0) * 0.15;

            // Fireball faces in the direction of x-velocity (LEFT and RIGHT are in an enum so this works)
            sp->direction = (sp->x_vel >= 0);
            break;

        case ICESHOCK:
        case ICESHOCK_P1:
            // Iceshock is affected by gravity and air resistance
            if(!sp->colliding) sp->y_vel += 0.3;
            sp->x_vel += convert(sp->x_vel < 0.0f) * 0.03;

            // Iceshock faces in the direction of xy-velocity
            sp->direction = (sp->x_vel >= 0);
            sp->angle = (int) (57.296 * atan(sp->y_vel / sp->x_vel));
            break;

        case ROCKFALL:
            // Rockfall falls quickly after it's done spawning
            if(!sp->colliding && !sp->spawning) sp->y_vel += 1.2;

            // Rockfall rotates slowly as it falls
            sp->direction = (sp->x_vel >= 0);
            sp->angle += 2;
            if(sp->colliding) sp->angle = 0;
            break;

        case ROCKFALL_P1:
        case ROCKFALL_P2:
            // Rockfall particles rotate and fall
            sp->direction = (sp->x_vel >= 0);
            sp->angle += 5;
            sp->y_vel += 0.3;
            break;

        case DARKEDGE:
            // Darkedge accelerates over time and spawns a particle trail
            if(!sp->colliding && !sp->spawning)
            {
                sp->x_vel += convert(sp->x_vel > 0) * 0.2;
                sp->y_vel += 0.05;

                // Darkedge spawns many particles randomly behind it, giving the appearance of
                // a particle trail
                if(get_rand() <= fabs(sp->x_vel) * 0.1)
                {
                    double x = sp->x_pos + (!sp->direction * 60);
                    double y = sp->y_pos + (get_rand() - 0.2) * 20;
                    double xv = (0.5 * sp->x_vel) + (get_rand() - 0.5) / 2;
                    double yv = (0.5 * sp->y_vel) + (get_rand() - 0.5) / 2;
                    spawnSprite(DARKEDGE_P1, x, y, xv, yv, RIGHT, 0, 0, 10);
                }
            }

            // Darkedge faces in the direction of xy-velocity
            sp->direction = (sp->x_vel >= 0);
            sp->angle = (int) (57.296 * atan(sp->y_vel / sp->x_vel));
            break;

        case DARKEDGE_P1:
            // Darkedge particles wobble around randomly
            if(get_rand() <= 0.05)
            {
                sp->x_vel = (get_rand() - 0.5) / 2;
                sp->y_vel = (get_rand() - 0.5) / 2;
            }
            sp->angle += 5;
            break;

        case GUY:
            // Update x velocity (friction / air resistance)
            if(fabs(sp->x_vel) <= 0.3) sp->x_vel = 0;
            else                       sp->x_vel += convert(sp->x_vel < 0.0f)*0.1;

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
static void advanceTime(Sprite sp)
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

    // Update sprite lifetime
    if(sp->lifetime) sp->lifetime--;
}

// Advance timed sprite variables which update every frame
void advanceTimers()
{
    // Iterate over active sprites
    for(struct ele* cursor = activeSprites; cursor != NULL; cursor = cursor->next)
    {
        advanceTime(cursor->sp);
    }
}

// Render a sprite's bounding boxes on top of the sprite (only in debug)
static void renderBounds(Sprite sp)
{
    // For each box, render 4 lines to create the rectangle
    SDL_Rect* bounds = getBounds(sp);
    for(int i = 0; i < sp->meta->num_bounds; i++)
    {
        // Line 1
        SDL_Rect box = bounds[i];
        SDL_Rect clip = {739, 77, box.w, 1};
        SDL_Rect renderQuad = {(int)sp->x_pos + box.x, (int)sp->y_pos + box.y, box.w, 1};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, 0, NULL, SDL_FLIP_NONE);

        // Line 2
        renderQuad = (SDL_Rect) {(int)sp->x_pos + box.x, (int)sp->y_pos + box.y + box.h, box.w, 1};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, 0, NULL, SDL_FLIP_NONE);

        // Line 3
        clip = (SDL_Rect) {739, 77, 1, box.h};
        renderQuad = (SDL_Rect) {(int)sp->x_pos+ box.x, (int)sp->y_pos + box.y, 1, box.h};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, 0, NULL, SDL_FLIP_NONE);

        // Line 4
        renderQuad = (SDL_Rect) {(int)sp->x_pos + box.x + box.w, (int)sp->y_pos + box.y, 1, box.h};
        SDL_RenderCopyEx(renderer, spriteSheet, &clip, &renderQuad, 0, NULL, SDL_FLIP_NONE);
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
    if(debug && sp->meta->type != PARTICLE)
    {
        renderBounds(sp);
        clip = (SDL_Rect) {743, 81, 3, 3};
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
    // If there are no rbounds, there are no lbounds either
    if(!rbounds) return NULL;

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
static SpellInfo initSpell(int act, int cast, int finish, int cd,
                           void (*launch)(Sprite), void (*collide)(Sprite))
{
    SpellInfo this_spell = (SpellInfo) malloc(sizeof(struct spell_metainfo));
    this_spell->action = act;
    this_spell->cast_time = cast;
    this_spell->finish_time = finish;
    this_spell->cooldown = cd;
    if(debug) this_spell->cooldown = 0; // no cooldowns in debug mode
    this_spell->on_launch = launch;
    this_spell->on_collide = collide;
    return this_spell;
}

// Assign meta info fields for a sprite
static SpriteInfo initSprite(int id, int type, int power, int hp, int width, int height,
                             int sheet_pos, int* fs, int num_bounds, SDL_Rect* bounds)
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
    sprite_info = (SpriteInfo*) malloc(sizeof(SpriteInfo)*NUM_SPRITES);
    spell_info =(SpellInfo*) malloc(sizeof(SpellInfo)*NUM_SPELLS);

    // Initialize meta info for spells
    spell_info[FIREBALL] = initSpell(CAST_FIREBALL, 32, 8, 120, launchFireball, collideSpell);
    spell_info[ICESHOCK] = initSpell(CAST_ICESHOCK, 32, 8, 240, launchIceshock, collideSpell);
    spell_info[ROCKFALL] = initSpell(CAST_ROCKFALL, 40, 40, 360, launchRockfall, collideRockfall);
    spell_info[DARKEDGE] = initSpell(CAST_DARKEDGE, 44, 24, 480, launchDarkedge, collideSpell);

    // HUMANS

    int numBounds = 2;

    // Sprite metadata: Guy
    int* fs = (int*) malloc(sizeof(int) * 11);
    memcpy(fs, (int[]) {0, 0, 4, 5, 10, 14, 22, 30, 40, 51, 56}, sizeof(int) * 11);
    SDL_Rect* bounds = malloc(sizeof(SDL_Rect) * numBounds);
    bounds[0] = (SDL_Rect) {9, 6, 15, 14};
    bounds[1] = (SDL_Rect) {10, 24, 10, 35};
    sprite_info[GUY] = initSprite(GUY, HUMANOID, 10, 100, 28, 60, 0, fs, numBounds, bounds);

    // SPELLS

    numBounds = 1;

    // Sprite metadata: Fireball
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 2, 5}, sizeof(int) * 4);
    bounds = malloc(sizeof(SDL_Rect) * numBounds);
    bounds[0] = (SDL_Rect) {6, 2, 12, 6};
    sprite_info[FIREBALL] = initSprite(FIREBALL, SPELL, 15, 1, 23, 10, 60, fs, numBounds, bounds);

    // Sprite metadata: Iceshock
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 2, 5}, sizeof(int) * 4);
    bounds = malloc(sizeof(SDL_Rect) * numBounds);
    bounds[0] = (SDL_Rect) {6, 1, 13, 7};
    sprite_info[ICESHOCK] = initSprite(ICESHOCK, SPELL, 25, 1, 23, 10, 70, fs, numBounds, bounds);

    // Sprite metadata: Rockfall
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 3, 4, 7}, sizeof(int) * 4);
    bounds = malloc(sizeof(SDL_Rect) * numBounds);
    bounds[0] = (SDL_Rect) {5, 5, 90, 90};
    sprite_info[ROCKFALL] = initSprite(ROCKFALL, SPELL, 35, 1, 100, 100, 85, fs, numBounds, bounds);

    // Sprite metadata: Darkedge
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 5, 8, 11}, sizeof(int) * 4);
    bounds = malloc(sizeof(SDL_Rect) * numBounds);
    bounds[0] = (SDL_Rect) {5, 11, 50, 9};
    sprite_info[DARKEDGE] = initSprite(DARKEDGE, SPELL, 25, 1, 60, 30, 215, fs, numBounds, bounds);

    // TODO:
    // Walk animation should start walking on first frame, and also just be better
    // Better AI
    // Casting spells gives points

    // Sprite metadata: Arcstorm

    // Increment NUM_SPRITES by 2 and NUM_SPELLS by 1
    // Add ARCSTORM and ARCSTORM_P1 to sprite enum
    // Add CAST_ARCSTORM to action enum
    // Add SDL_SCANCODE in main so guys can cast arcstorm

    // Spell info: Cast time, finish time, cooldown, action functions
    // write arcstorm launch and collide functions

    // Frame sections
    // Bounding boxes
    // Sprite info: Power, hp, width, height, sheet position
    // update GUY frame sections with CAST_ARCSTORM frames

    // Sprite info for the particle

    // define physics for ARCSTORM and ARCSTORM_P1 in moveSprite

    // Draw ARCSTORM spawn, move, and collide animations
    // Draw ARCSTORM_P1 move animation
    // Draw ARCSTORM spell icon in toolbar
    // Draw CAST_ARCSTORM animation for GUY

    // PARTICLES

    numBounds = 0;
    bounds = NULL;

    // Sprite metadata: Iceshock Particle
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 2, 2}, sizeof(int) * 4);
    sprite_info[ICESHOCK_P1] = initSprite(ICESHOCK_P1, PARTICLE, 0, 1, 5, 5, 80, fs, numBounds, bounds);

    // Sprite metadata: Rockfall Particle 1
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 1, 1}, sizeof(int) * 4);
    sprite_info[ROCKFALL_P1] = initSprite(ROCKFALL_P1, PARTICLE, 0, 1, 25, 25, 185, fs, numBounds, bounds);

    // Sprite metadata: Rockfall Particle 2
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 2, 2}, sizeof(int) * 4);
    sprite_info[ROCKFALL_P2] = initSprite(ROCKFALL_P2, PARTICLE, 0, 1, 5, 5, 210, fs, numBounds, bounds);

    // Sprite metadata: Darkedge Particle 1
    fs = (int*) malloc(sizeof(int) * 4);
    memcpy(fs, (int[]) {0, 0, 2, 2}, sizeof(int) * 4);
    sprite_info[DARKEDGE_P1] = initSprite(DARKEDGE_P1, PARTICLE, 0, 1, 5, 5, 245, fs, numBounds, bounds);
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
int unloadSprites()
{
    // Iterate over active sprites
    struct ele* prev = NULL;
    int game_over = 0;
    for(struct ele* cursor = activeSprites; cursor != NULL;)
    {
        // Check if the sprite is dead
        if(isDead(cursor->sp))
        {
            if(cursor->sp->meta->id == GUY)
            {
                // If the dead sprite is a Guy, just hide it and signal game over
                if(cursor->sp == guys[0])
                {
                    hideGuy(0);
                    game_over = 1;
                }
                else
                {
                    hideGuy(1);
                    game_over = 2;
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
