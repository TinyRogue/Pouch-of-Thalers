#ifndef PLAYER_H
#define PLAYER_H

#include <pthread.h>                    /* For mutexes and threads                  */
#include <semaphore.h>                  /* For semaphores                           */
#include <unistd.h>                     /* For usleep */
#include <fcntl.h>                      /* For shm and sem constants                */
#include <sys/mman.h>                   /* For mmap, shared memory                  */
#include <time.h>                       /* For srand, current time                  */
#include <stdlib.h>                     /* For exit(), malloc() family              */
#include <stdbool.h>                    /* For bool type                            */
#include <errno.h>                      /* For error handling                       */
#include <ctype.h>                      /* For tolower()                            */
#include "../../utils/shared_data.h"    /* For structures used in host and player   */
#include "../../utils/view/view.h"      /* Ncurses wrapper                          */

#define NO_PLAYER -1
#define NO_BEAST 0

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
void play();
void clean_up();

#endif
