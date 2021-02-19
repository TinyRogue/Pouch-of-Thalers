#ifndef GOGGLE_EYED_APPROACH_VIEW_H
#define GOGGLE_EYED_APPROACH_VIEW_H

#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../shared_data.h"

typedef enum {
    WALL_COLOUR = 1, BUSHES_COLOUR, BEAST_COLOUR, TREASURE_COLOUR, CAMPSITE_COLOUR, PLAYER_COLOUR, BLANK_COLOUR,
    TEXT_COLOUR
} colours_t;

typedef struct {
    int message_size;
    int message_amount;
    char **messages;
    int pos_x;
    int pos_y;
    pthread_mutex_t *lock;
} logger_t;

void init_screen();
void print_field(const field_kind_t type, const int row, const int column);
void print_player(const short player_number, const int row, const int column);
void print_beast(const int row, const int column);
void print(const char* const message, const int row, const int column);
void print_coloured(const char* const message, const colours_t colour, const int row, const int column);
void print_legend(int row, int column);

bool init_logger(int row, int column, int message_amount, int message_size, pthread_mutex_t* lock);
void log_message(const char * const message);
void destroy_logger();


#endif //GOGGLE_EYED_APPROACH_VIEW_H
