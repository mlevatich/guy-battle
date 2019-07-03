#include "../headers/constants.h"
#include "../headers/sprite.h"
#include "../headers/level.h"
#include "../headers/interface.h"

// TODOS

// Draw ARCSURGE move frames and fix bounding box
// Walk animation should start walking on first frame, and also just be better
// Touch up logo
// Volcano background/foreground

// looping music
// Sound effects

// Linux support
// Code cleanup / cmd args / version

// Debug mode is off by default
bool debug = false;

// Window and renderer, used by all modules
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

// Load SDL and initialize the window, renderer, audio, and data
bool loadGame()
{
    // Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;

    // Initialize sound
    if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return false;

    // Create window
    window = SDL_CreateWindow("GUY BATTLE", 20, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if(!window) return false;

    // Create renderer for window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) return false;

    // Initialize renderer color and image loading
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    // Load level backgrounds and foregrounds
    loadLevels();

    // Load meta information for sprites
    loadSpriteInfo();

    // Load UI elements
    loadInterface();

    return true;
}

// Free all resources and quit SDL
void quitGame()
{
    // Free sprite metainfo
    freeSpriteInfo();

    // Free remaining active sprites
    freeActiveSprites();

    // Free backgrounds and foregrounds
    freeLevels();

    // Free UI elements
    freeInterface();

    // Free renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    renderer = NULL;
    window = NULL;

    // Free SDL
    SDL_Quit();
}

// Helper function to load an SDL texture
SDL_Texture* loadTexture(const char* path)
{
    // Create a surface from path to bitmap file
    SDL_Texture* newTexture = NULL;
    SDL_Surface* loaded = SDL_LoadBMP(path);

    // Create a texture from the surface
    newTexture = SDL_CreateTextureFromSurface(renderer, loaded);
    SDL_FreeSurface(loaded);
    return newTexture;
}

// Helper function to cast a spell and update the score on success
bool sCast(int guy, int spell)
{
    bool succ = cast(guy, spell);
    if(succ) updateScore(spell + 1);
    return succ;
}

// Helper function to set the level and teleport guys
void setLevel(int level, int mode)
{
    switchLevel(level);
    int* starts = getStartingPositions(level);
    resetGuy(0, starts[0], starts[1]);
    resetGuy(1, starts[2], starts[3]);
}

// Helper function to reset the game to title screen
void resetGame(int* mode, int* selection, int* vs_or_ai)
{
    *mode = TITLE;
    *selection = VS;
    *vs_or_ai = VS;
    setScore(0);
    setLevel(FOREST, TITLE);
}

int main(int argc, char** argv)
{
    // Set debug mode if given as command line arg
    if(argc > 1 && !strcmp(argv[1], "-debug")) debug = true;

    // Load game
    if(!loadGame())
    {
        fprintf(stderr,"Error: Initialization Failed\n");
        return 1;
    }

    // Load music and sound effects
    Mix_Music* main_theme = Mix_LoadMUS("sound/twilight_of_the_guys.wav");

    // Track what mode the game is in, and what menu selection is hovered
    int mode = OPENING;
    int selection = VS;
    int vs_or_ai = VS;

    // Track how many frames have passed since the game started
    long long frame = 0;

    // In debug mode, spawn the guys immediately
    if(debug)
    {
        int* starts = getStartingPositions(getLevel());
        spawnSprite(GUY, starts[0], starts[1], 0, 0, RIGHT, 0, 0, 0);
        spawnSprite(GUY, starts[2], starts[3], 0, 0, LEFT, 0, 0, 0);
        mode = TITLE;
    }

    // Game loop
    bool quit = false;
    SDL_Event e;
    while(!quit)
    {
        // Track how long this frame takes
        int start_time = SDL_GetTicks();

        // Some events occur at specific points in the opening scene
        // End opening and give control to the player after 375 frames
        if(mode == OPENING && !debug)
        {
            int* starts = getStartingPositions(getLevel());
            // if(frame == 10) Mix_PlayMusic(main_theme, -1);
            if(frame == 100) spawnSprite(GUY, starts[0], -100, 0, 0, RIGHT, 0, 0, 0);
            if(frame == 225) spawnSprite(GUY, starts[2], -100, 0, 0, LEFT, 0, 0, 0);
            if(frame == 375) mode = TITLE;
        }

        // Process any SDL events that have happened since last frame
        while(SDL_PollEvent(&e) != 0)
        {
            // No need to process further events if an exit signal was received
            if(e.type == SDL_QUIT) quit = true;

            // Process key presses as game mode changes / menu selections
            if(e.type == SDL_KEYDOWN)
            {
                int key = e.key.keysym.sym;
                switch(mode)
                {
                    case TITLE:
                        // Select VS, AI, or controls and hit enter
                        if(key == SDLK_RETURN)
                        {
                            if(selection == CONTROLS)
                            {
                                mode = selection;
                            }
                            else
                            {
                                mode = STAGE_SELECT;
                                vs_or_ai = selection;
                            }
                        }
                        else if(key == SDLK_UP)
                        {
                            selection = hover(mode, UP);
                        }
                        else if(key == SDLK_DOWN)
                        {
                            selection = hover(mode, DOWN);
                        }
                        break;

                    case STAGE_SELECT:
                        // Select Volcano or Forest and hit enter, or esc to title
                        if(key == SDLK_RETURN)
                        {
                            mode = vs_or_ai;
                        }
                        else if(key == SDLK_ESCAPE)
                        {
                            resetGame(&mode, &selection, &vs_or_ai);
                        }
                        else if(key == SDLK_UP)
                        {
                            selection = hover(mode, UP);
                            if(getLevel() != selection) setLevel(FOREST, vs_or_ai);
                        }
                        else if(key == SDLK_DOWN)
                        {
                            selection = hover(mode, DOWN);
                            if(getLevel() != selection) setLevel(VOLCANO, vs_or_ai);
                        }
                        break;

                    case AI:
                        // Hit esc to pause during single player
                        if(key == SDLK_ESCAPE) mode = PAUSE;
                        break;

                    case CONTROLS:
                        // Hit esc or enter to leave controls page
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) mode = TITLE;
                        break;

                    case PAUSE:
                        // Hit esc or enter to unpause while paused
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) mode = AI;
                        break;

                    case GAME_OVER_VS:
                    case GAME_OVER_AI:
                        // Hit esc or enter to return to the title screen
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN) resetGame(&mode, &selection, &vs_or_ai);
                        break;

                    default:
                        // In 2-player mode or during the opening, player can't access anything
                        break;
                }
            }
        }

        if(mode != PAUSE)
        {
            // Process key presses as actions in battle
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            bool succ = 0;
            int guy = 0;
            if(mode == VS)
            {
                // Input for guy 0
                if(!succ && keys[SDL_SCANCODE_5])                          succ = cast(guy, ARCSURGE);
                if(!succ && keys[SDL_SCANCODE_4])                          succ = cast(guy, DARKEDGE);
                if(!succ && keys[SDL_SCANCODE_3])                          succ = cast(guy, ROCKFALL);
                if(!succ && keys[SDL_SCANCODE_2])                          succ = cast(guy, ICESHOCK);
                if(!succ && keys[SDL_SCANCODE_1])                          succ = cast(guy, FIREBALL);
                if(!succ && keys[SDL_SCANCODE_D])                          succ = jump(guy);
                if(!succ && keys[SDL_SCANCODE_X] && !keys[SDL_SCANCODE_V]) succ = walk(guy, LEFT);
                if(!succ && keys[SDL_SCANCODE_V] && !keys[SDL_SCANCODE_X]) succ = walk(guy, RIGHT);

                // Input for guy 1
                succ = 0;
                guy = 1;
                if(!succ && keys[SDL_SCANCODE_P])                                 succ = cast(guy, ARCSURGE);
                if(!succ && keys[SDL_SCANCODE_O])                                 succ = cast(guy, DARKEDGE);
                if(!succ && keys[SDL_SCANCODE_I])                                 succ = cast(guy, ROCKFALL);
                if(!succ && keys[SDL_SCANCODE_U])                                 succ = cast(guy, ICESHOCK);
                if(!succ && keys[SDL_SCANCODE_Y])                                 succ = cast(guy, FIREBALL);
                if(!succ && keys[SDL_SCANCODE_UP])                                succ = jump(guy);
                if(!succ && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) succ = walk(guy, LEFT);
                if(!succ && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) succ = walk(guy, RIGHT);
            }
            else if(mode == AI)
            {
                // Input for guy 0
                if(!succ && keys[SDL_SCANCODE_5])                                 succ = sCast(guy, ARCSURGE);
                if(!succ && keys[SDL_SCANCODE_4])                                 succ = sCast(guy, DARKEDGE);
                if(!succ && keys[SDL_SCANCODE_3])                                 succ = sCast(guy, ROCKFALL);
                if(!succ && keys[SDL_SCANCODE_2])                                 succ = sCast(guy, ICESHOCK);
                if(!succ && keys[SDL_SCANCODE_1])                                 succ = sCast(guy, FIREBALL);
                if(!succ && keys[SDL_SCANCODE_UP])                                succ = jump(guy);
                if(!succ && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) succ = walk(guy, LEFT);
                if(!succ && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) succ = walk(guy, RIGHT);

                // Decisions for CPU Guy
                takeCPUAction();
            }

            // Move the background
            moveBackground();

            // Update positions, velocities, and orientations of all sprites
            moveSprites();

            // Check for and handle collisions with terrain or other sprites
            terrainCollisions(getPlatforms(), getWalls());
            spriteCollisions();

            // Spawn any new spells that people are casting
            launchSpells();

            // Update values on timed sprite variables (spell cooldowns, casting / collision durations, etc)
            advanceTimers();

            // Unload dead sprites and check for dead guys
            int signal = unloadSprites();
            if(signal)
            {
                // In VS mode, if either guy dies, the game ends. In AI mode, if the cpu guy dies,
                // a new guy is spawned and play continues.
                if(mode == VS) mode = GAME_OVER_VS;
                else if (signal == 1) mode = GAME_OVER_AI;
                else
                {
                    int* starts = getStartingPositions(getLevel());
                    resetGuy(1, starts[2], -100);
                    updateScore(123);
                }
            }

            // Update the animation frame which is drawn for all sprites
            updateAnimationFrames();
        }

        // Render changes to screen
        SDL_RenderClear(renderer);
        renderLevel();
        renderSprites();
        renderInterface(mode, frame, getHealth(0), getHealth(1), getCooldowns(0), getCooldowns(1));
        SDL_RenderPresent(renderer);

        // Cap framerate at MAX_FPS
        double ms_per_frame = 1000.0 / MAX_FPS;
        if(debug) ms_per_frame *= 3;
        int sleep_time = ms_per_frame - (SDL_GetTicks() - start_time);
        if(sleep_time > 0) SDL_Delay(sleep_time);
        frame++;
    }

    // Free music and sound effects
    Mix_FreeMusic(main_theme);
    Mix_Quit();

    // Free all resources and exit game
    quitGame();
    return 0;
}
