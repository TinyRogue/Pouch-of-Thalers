#ifndef GOGGLE_EYED_APPROACH_APPLICATION_H
#define GOGGLE_EYED_APPROACH_APPLICATION_H

#include <stdio.h>                      /*  For file handling           */
#include <stdbool.h>                    /*  For error handling          */
#include <string.h>                     /*  For error handling          */
#include <errno.h>                      /*  For error handling          */
#include <stdlib.h>                     /*  For exit(), malloc() family */
#include "../../utils/shared_data.h"    /* For field_kind_t and field_t */

typedef struct {
    field_t **lbrth;
    size_t height;
    size_t width;
} labyrinth_t;


labyrinth_t* get_labyrinth_from_file(const char * const filename);
void destroy_labyrinth(labyrinth_t *lbrth);

#endif //GOGGLE_EYED_APPROACH_APPLICATION_H
