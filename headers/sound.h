/*
 Music and sound effects control
 */

#include <SDL2/SDL_mixer.h>

// Audio-related constants
#define SAMPLE_RATE 44100
#define NUM_CHANNELS 2
#define CHUNK_SIZE 2048

// Number of unique sound effects in the game
#define NUM_SOUND_EFFECTS 2

// List of sound effects
enum sound_effects
{ SFX_HOVER, SFX_SELECT };

// Mute the game's audio
void setMute(void);

// Start the game's main theme
void startMusic(void);

// Play a sound effect
void playSoundEffect(int sfx_id);

// Load audio elements
void loadSound(void);

// Free audio elements
void freeSound(void);
