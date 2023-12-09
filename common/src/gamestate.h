#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "tank.h"

#define MAX_PLAYERS 50

typedef struct {
	uint8_t num_tanks;
	Tank tanks[MAX_PLAYERS];
} GameState;

#endif /* GAMESTATE_H */ // just use //?
