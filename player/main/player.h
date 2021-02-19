#ifndef PLAYER_H
#define PLAYER_H

#include <pthread.h> /* For mutex and semaphore service */
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include "../../utils/shared_data.h"
#include "../../utils/view/view.h"

typedef struct {
    int pid;
    int host_pid;
    int carried_treasure;
    int brought_treasure;
    player_kind_t player_kind;
    field_t player_sh_lbrth[PLAYER_SIGHT][PLAYER_SIGHT];
    size_t current_round;
    struct {
        int pos_x;
        int pos_y;
    };
    dir_t dir;
    sem_t host_response;
    sem_t player_response;
} *shared_info;


bool initialise();
void clean_up();

#endif
