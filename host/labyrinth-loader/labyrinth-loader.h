#ifndef GOGGLE_EYED_APPROACH_APPLICATION_H
#define GOGGLE_EYED_APPROACH_APPLICATION_H

#include <stdio.h>      /*  For file handling           */
#include <stdbool.h>    /*  For error handling          */
#include <string.h>     /*  For error handling          */
#include <errno.h>      /*  For error handling          */
#include <stdlib.h>     /*  For exit(), malloc() family */


typedef enum {BLANK, WALL, BUSH, COIN, TREASURE, LARGE_TREASURE, DROPPED_TREASURE, CAMPSITE, SPAWN} field_kind_t;

typedef struct {
    size_t value;
    field_kind_t kind;
} field_t;

typedef struct {
    field_t **lbrth;
    size_t height;
    size_t width;
} labyrinth_t;


labyrinth_t* get_labyrinth_from_file(const char * const filename);
void destroy_labyrinth(labyrinth_t *lbrth);

#endif //GOGGLE_EYED_APPROACH_APPLICATION_H
