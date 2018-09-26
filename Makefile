CC         = gcc
CFLAGS     = -g3 -std=c99 -pedantic -Wall
FRAMEWORKS = -framework ./SDL2 -framework ./SDL2_mixer
DEPS       = sprite.h interface.h level.h global_utils.h
OBJ        = main.o sprite.o interface.o level.o global_utils.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

GUY_BATTLE: $(OBJ)
	$(CC) $(FRAMEWORKS) -o $@ $^ $(CFLAGS)
	rm -f *.o
