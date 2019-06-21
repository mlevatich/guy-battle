# GUY_BATTLE

In this competitive fighting game, take control of a Guy,
a powerful mage seeking to destroy the Other Guy, his mortal foe.

Challenge your friends and claim victory in the Cultist Clearing, or on
Phoenix Mountain, for only one Guy will make it out alive!

Or defeat as many Guys as you can to earn a high score in singleplayer mode!

I wrote GUY_BATTLE in C, using the development library SDL. I wrote the music
in SuperCollider and drew the art/animations in GIMP.

# Dependencies/Install

The game is currently only available for MacOS.

You'll need to install SDL2 and SDL_mixer - this can be done easily with brew:

~~~~
# if you need to install brew
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

# installing sdl2 and sdl2 mixer
brew install sdl2
brew install sdl2_mixer
~~~~

Once that's done, install GUY_BATTLE with the following:

~~~~
git clone https://github.com/mlevatich/GUY_BATTLE.git
cd GUY_BATTLE
make
~~~~

# Run

~~~~
./GUY_BATTLE
~~~~

Controls can be viewed in game.  Have fun!
