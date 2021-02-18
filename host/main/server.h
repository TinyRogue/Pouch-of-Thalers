#ifndef GOGGLE_EYED_APPROACH_SERVER_H
#define GOGGLE_EYED_APPROACH_SERVER_H

#include <pthread.h> /* For mutex and semaphore service */
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include "../labyrinth-loader/labyrinth-loader.h"
#include "../../utils/view/view.h"
#include "../../utils/shared_data.h"

#define PLAYER_SIGHT 3  //* <-- Override for sake of safety
#define MAX_PLAYERS 4
#define MAP_PRNTR_SEM "/MAP_PRNTR_SEM"
#define FREE_SLOT_UNAVAILABLE -1
#define ADDITIONAL_INFO_SIZE 100

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
    
    labyrinth_t *lbrth;
    sem_t *print_map_invoker;
    player_t player_exchange[MAX_PLAYERS];
} game_t;


bool initialise(const char* const filename);
void clean_up();

#endif //GOGGLE_EYED_APPROACH_SERVER_H
