CC     = gcc
CFLAGS = -g3 -std=c99 -pedantic -Wall
LIBS   = -lSDL2 -lSDL2_mixer
DEPS   = headers/sprite.h headers/interface.h headers/level.h headers/constants.h headers/sound.h
OBJ    = main.o sprite.o interface.o level.o sound.o
SRC    = src

%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

GUY_BATTLE: $(OBJ)
	$(CC) $(LIBS) -o $@ $^ $(CFLAGS)
	rm -f *.o
