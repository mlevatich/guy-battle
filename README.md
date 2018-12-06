# GUY_BATTLE

A 2D fighting game I made, written in C using SDL!

I also wrote the music in SuperCollider and drew the art/animations in GIMP.

It's a work in progress right now. Planned features (in order) include:

- Sound effects
- Collisions / hp loss / dying (i.e. the full game loop)
- Three new spells! For a total of five
- Better art for Phoenix Mountain
- AI and 1 player mode
- Improved animations

# Dependencies/Install

I've only tested on MacOS as of now - no guarantees for Windows or Linux, but should 
be compatible with older MacOS versions.

You'll need to install SDL2 and SDL_mixer - on Mac this can be done easily with brew:

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

./GUY_BATTLE

Controls can be viewed in game.  Have fun!
