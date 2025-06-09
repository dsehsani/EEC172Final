#ifndef MAP_BITMAP_H
#define MAP_BITMAP_H

#include <stdint.h>

// 256x256 px
#define MAP_WIDTH 32   // Number of tiles horizontally
#define MAP_HEIGHT 32  // Number of tiles vertically



extern const uint8_t map_tiles[MAP_WIDTH * MAP_HEIGHT];

#endif
