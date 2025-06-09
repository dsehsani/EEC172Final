#ifndef TILES_H
#define TILES_H

#include <stdint.h>


#define TILE_SIZE 16
#define NUM_TILE_TYPES 6


#define TILE_GRASS 0
#define TILE_BUSH  1
#define TILE_WATER 2
#define TILE_TREE  3
#define TILE_ROCK  4
#define TILE_PLAYER 5


extern const uint16_t* tile_set[NUM_TILE_TYPES];



#endif