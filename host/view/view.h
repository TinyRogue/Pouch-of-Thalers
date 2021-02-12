#ifndef GOGGLE_EYED_APPROACH_VIEW_H
#define GOGGLE_EYED_APPROACH_VIEW_H

#include <ncurses.h>
#include "../main/server.h"


typedef enum {
    WALL_COLOUR = 1, BUSHES_COLOUR, BEAST_COLOUR, TREASURE_COLOUR, CAMPSITE_COLOUR, PLAYER_COLOUR, BLANK_COLOUR,
    TEXT_COLOUR
} colours_t;

void init_screen();
void print_field(const field_kind_t type, const int row, const int column);
void print_player(const short player_number, const int row, const int column);
void print_beast(const int row, const int column);


#endif //GOGGLE_EYED_APPROACH_VIEW_H
