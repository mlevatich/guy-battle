#include "../headers/constants.h"
#include "../headers/sprite.h"
#include "../headers/level.h"
#include "../headers/interface.h"

// Window and renderer, used by all modules
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

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

    // Load sprite sheet
    loadSpriteSheet();

    // Load level backgrounds and foregrounds
    loadLevels();

    // Load meta information for sprites
    loadSpriteInfo();

    // Load interface
    loadInterface();

    return true;
}

void quitGame()
{
    // Free sprite metainfo
    freeMetaInfo();

    // Free remaining active sprites
    freeSprites();

    // Free sprite sheet
    freeSpriteSheet();

    // Free backgrounds and foregrounds
    freeLevels();

    // Free interface
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

// Helper function to set the level and teleport guys
void setLevel(Sprite guy1, Sprite guy2, int level, int mode)
{
    switchLevel(level);
    int* starts = getStartingPositions(level);
    teleportSprite(guy1,  starts[0], starts[1]);
    if(mode != AI) teleportSprite(guy2, starts[2], starts[3]);
}

// Helper function to reset the game to title screen
void resetGame(Sprite guy1, Sprite guy2, int* mode, int* selection, int* vs_or_ai, int* score)
{
    *mode = TITLE;
    *selection = VS;
    *vs_or_ai = VS;
    *score = 0;
    setLevel(guy1, guy2, FOREST, TITLE);
    resetGuys(guy1, guy2);
}

int main(int argc, char* args[])
{
    // Load game
    if(!loadGame())
    {
        fprintf(stderr,"Error: Initialization Failed\n");
        return 1;
    }

    // Load music and sound effects
    Mix_Music* main_theme = Mix_LoadMUS("sound/twilight_of_the_gods.wav");

    // State variables
    int mode = OPENING;
    int selection = VS;
    int vs_or_ai = VS;
    long long frame = 0;
    int score = 0;
    int* starts = getStartingPositions(getLevel());
    Sprite guy1 = NULL;
    Sprite guy2 = NULL;

    // Game loop
    bool quit = false;
    SDL_Event e;
    while(!quit)
    {
        // Track how long all of the computations take
        int start_time = SDL_GetTicks();

        // Some events occur at specific frames in the opening cutscene
        // End opening and give control to the player after 375 frames
        if(frame == 10) Mix_PlayMusic(main_theme, -1);
        if(frame == 100) guy1 = spawnSprite(GUY, 100, starts[0], starts[1]-300, 0, 0, RIGHT);
        if(frame == 225) guy2 = spawnSprite(GUY, 100, starts[2], starts[3]-300, 0, 0, LEFT);
        if(frame == 375) mode = TITLE;

        // Process key presses which are game state changes / menu selections
        while(SDL_PollEvent(&e) != 0)
        {
            if(e.type == SDL_QUIT)
            {
                quit = true;
            }
            else if(e.type == SDL_KEYDOWN)
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
                                if(vs_or_ai == AI) teleportSprite(guy2, SCREEN_WIDTH+20, 0);
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
                            resetGame(guy1, guy2, &mode, &selection, &vs_or_ai, &score);
                        }
                        else if(key == SDLK_UP)
                        {
                            selection = hover(mode, UP);
                            if(getLevel() != selection) setLevel(guy1, guy2, FOREST, vs_or_ai);
                        }
                        else if(key == SDLK_DOWN)
                        {
                            selection = hover(mode, DOWN);
                            if(getLevel() != selection) setLevel(guy1, guy2, VOLCANO, vs_or_ai);
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

                    case GAME_OVER:
                        // Hit esc or enter to return to the title screen
                        if(key == SDLK_ESCAPE || key == SDLK_RETURN)
                        {
                            resetGame(guy1, guy2, &mode, &selection, &vs_or_ai, &score);
                        }
                        break;

                    default:
                        // In 2-player mode or during the opening, player can't access anything
                        break;
                }
            }
        }

        // Process key presses as actions in battle
        const Uint8* keys = SDL_GetKeyboardState(NULL);
        bool success = 0;
        switch(mode)
        {
            case VS:
                // Input for guy 1
                success = 0;
                if(!success && keys[SDL_SCANCODE_2])                          success = spellcast(guy1, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_1])                          success = spellcast(guy1, FIREBALL);
                if(!success && keys[SDL_SCANCODE_D])                          success = jump(guy1);
                if(!success && keys[SDL_SCANCODE_X] && !keys[SDL_SCANCODE_V]) success = walk(guy1, LEFT);
                if(!success && keys[SDL_SCANCODE_V] && !keys[SDL_SCANCODE_X]) success = walk(guy1, RIGHT);

                // Input for guy 2
                success = 0;
                if(!success && keys[SDL_SCANCODE_U])                                 success = spellcast(guy2, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_Y])                                 success = spellcast(guy2, FIREBALL);
                if(!success && keys[SDL_SCANCODE_UP])                                success = jump(guy2);
                if(!success && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) success = walk(guy2, LEFT);
                if(!success && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) success = walk(guy2, RIGHT);
                break;

            case AI:
                // Input for guy
                success = 0;
                if(!success && keys[SDL_SCANCODE_2])                                 success = spellcast(guy1, ICESHOCK);
                if(!success && keys[SDL_SCANCODE_1])                                 success = spellcast(guy1, FIREBALL);
                if(!success && keys[SDL_SCANCODE_UP])                                success = jump(guy1);
                if(!success && keys[SDL_SCANCODE_LEFT] && !keys[SDL_SCANCODE_RIGHT]) success = walk(guy1, LEFT);
                if(!success && keys[SDL_SCANCODE_RIGHT] && !keys[SDL_SCANCODE_LEFT]) success = walk(guy1, RIGHT);
                break;
        }

        if(mode != PAUSE)
        {
            // Process ai decisions
            // no ai yet

            // Unload dead sprites and check for game over
            // (game ends if dead sprite is a Guy)
            bool game_over = unloadSprites();
            if(game_over) mode = GAME_OVER;

            // Check for and handle collisions
            detectCollisions(getPlatforms(), getWalls());

            // Change action states for sprites (controls how they're drawn/animated)
            // based on key input and collisions
            updateActions();

            // Move sprites based on current actions, velocities, and terrain
            // Update the frames for those sprites
            // Update the velocities for those sprites
            moveSprites(getPlatforms(), getWalls());

            // Spawn any new sprites TODO: can I do anything about this ugliness?
            launchSpells(guy1, &score);
            launchSpells(guy2, NULL);

            // Move the background
            moveBackground();

            // Update timers (spell cooldowns, casting times, collision times, etc) TODO: collapse these functions
            // and dont pass args
            advanceGuyTimers(guy1);
            advanceGuyTimers(guy2);
            advanceCollisions();
        }

        // Render changes to screen
        SDL_RenderClear(renderer);
        renderBackground();
        renderForeground();
        renderSprites();
        renderInterface(mode, frame, getHealth(guy1), getHealth(guy2), getCooldowns(guy1), getCooldowns(guy2), score);
        SDL_RenderPresent(renderer);

        // Cap framerate at MAX_FPS
        int sleep_time = MS_PER_FRAME - (SDL_GetTicks() - start_time);
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
