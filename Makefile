CC     = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall
LIBS   = -lSDL2 -lSDL2_mixer -L/opt/homebrew/lib
INC    = -D_THREAD_SAFE -I/opt/homebrew/include -I/opt/homebrew/include/SDL2
DEPS   = headers/sprite.h headers/interface.h headers/level.h headers/constants.h headers/sound.h
OBJ    = main.o sprite.o interface.o level.o sound.o
SRC    = src

%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $(INC) $< $(CFLAGS)

GUY_BATTLE: $(OBJ)
	$(CC) $(LIBS) -o $@ $(INC) $^ $(CFLAGS)
	rm -f *.o
