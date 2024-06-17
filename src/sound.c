#include "../headers/constants.h"
#include "../headers/sound.h"

// Audio is not muted by default
bool mute = false;

// Declaring audio elements
Mix_Music* main_theme;
Mix_Chunk** sfx_list;

// Mute all audio
void setMute(void)
{
    mute = true;
}

// Start the game's main theme
void startMusic(void)
{
    if(!mute) Mix_PlayMusic(main_theme, -1);
}

// Play a sound effect
void playSoundEffect(int sfx_id)
{
    if(!mute)
    {
        int ch = Mix_PlayChannel(-1, sfx_list[sfx_id], 0);
	    Mix_ExpireChannel(ch, 1000);
    }
}

// Load audio elements into memory
void loadSound(void)
{
    // Initialize audio
    Mix_OpenAudio(SAMPLE_RATE, MIX_DEFAULT_FORMAT, NUM_CHANNELS, CHUNK_SIZE);

    // Load music and set volume
    main_theme = Mix_LoadMUS("sound/music/twilight_of_the_guys.wav");
    Mix_VolumeMusic(100);

    // Make space for sound effect list
    sfx_list = (Mix_Chunk**) malloc(sizeof(Mix_Chunk*) * NUM_SOUND_EFFECTS);

    // Menu navigation noises
    sfx_list[SFX_HOVER] = Mix_LoadWAV("sound/effects/hover.wav");
    sfx_list[SFX_SELECT] = Mix_LoadWAV("sound/effects/select.wav");
    sfx_list[SFX_BACK] = Mix_LoadWAV("sound/effects/back.wav");
}

// Free audio elements from memory
void freeSound(void)
{
    // Free music
    Mix_FreeMusic(main_theme);

    // Free all sound effects
    for(int i = 0; i < NUM_SOUND_EFFECTS; i++)
    {
        Mix_FreeChunk(sfx_list[i]);
    }
    free(sfx_list);

    // Quit SDL Mixer
    Mix_Quit();
}
