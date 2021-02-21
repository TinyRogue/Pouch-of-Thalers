#ifndef GOGGLE_EYED_APPROACH_SERVER_H
#define GOGGLE_EYED_APPROACH_SERVER_H

#include <pthread.h>                                /* For mutexes and threads                  */
#include <semaphore.h>                              /* For semaphores                           */
#include <unistd.h>                                 /* For usleep()                             */
#include <fcntl.h>                                  /* For shm and sem constants                */
#include <sys/mman.h>                               /* For mmap, shared memory                  */
#include <time.h>                                   /* For srand, current time                  */
#include <stdlib.h>                                 /* For exit(), malloc() family              */
#include <ctype.h>                                  /* For tolower()                            */
#include <signal.h>                                 /* For kill()                               */
#include "../labyrinth-loader/labyrinth-loader.h"   /* For labyrinth struct from file           */
#include "../../utils/view/view.h"                  /* Ncurses wrapper                          */
#include "../../utils/shared_data.h"                /* For structures used in host and player   */

#ifdef PLAYER_SIGHT
#undef PLAYER_SIGHT
#endif
#define PLAYER_SIGHT 5  //* <-- Override for sake of safety

#define MAX_PLAYERS 4
#define MAP_PRNTR_SEM "/MAP_PRNTR_SEM"
#define FREE_SLOT_UNAVAILABLE -1
#define ADDITIONAL_INFO_SIZE 100
#define SECOND 1000000

#define RIGHT 1
#define BEAST_IN_MIDDLE 0
#define LEFT -1
#define BEAST_ABOVE -1
#define BEAST_BELOW 1

typedef struct {
    struct server_side_info_t {
        int spawn_pos_x;
        int spawn_pos_y;
        int curr_x;
        int curr_y;
        int carried_treasure;
        int brought_treasure;
        int deaths;
        bool is_slowed_down;
        char shm_name[40];
        
        bool is_slot_taken;
    } server_side_info;

    shared_info_t *shared_info;
} player_t;

typedef struct {
    size_t current_round;
    size_t host_pid;
    pthread_mutex_t general_lock;
    struct beasts_t {
        size_t beasts_amount;
        struct single_beast_t {
            char shm_name[40];
            pthread_t beasts_thrd;
            shared_info_t *shared_info;
            int beast_id;
        } **beasts_ptr;
    } beasts;
    
    labyrinth_t *lbrth;
    sem_t *print_map_invoker;
    player_t player_exchange[MAX_PLAYERS];
} game_t;


bool initialise(const char* const filename);
void clean_up();

#endif //GOGGLE_EYED_APPROACH_SERVER_H
