#ifndef GOGGLE_EYED_APPROACH_SERVER_H
#define GOGGLE_EYED_APPROACH_SERVER_H

#include <pthread.h> /* For mutex and semaphore service */
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "../labyrinth-loader/labyrinth-loader.h"
#include "../view/view.h"

#define PLAYER_SIGHT 3
#define MAX_PLAYERS 4
#define MAP_PRNTR_SEM "/MAP_PRNTR_SEM"

typedef enum {CPU, HUMAN} player_kind_t;
typedef enum {EAST, NORTH, WEST, SOUTH} dir_t;

typedef struct {
    int spawn_pos_x;
    int spawn_pos_y;
    int carried_treasure;
    int brought_treasure;
    bool is_slowed_down;

    struct shared_info_t {
        int pid;
        int server_pid;
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
        sem_t host_resp;
        sem_t player_resp;
    } *shared_info;
} player_t;

typedef struct {
    size_t current_round;
    int server_pid;
    pthread_mutex_t general_lock;
    
    labyrinth_t *lbrth;
    sem_t *print_map_invoker;
    player_t player_exchange[MAX_PLAYERS];
} game_t;


bool initialize(const char * const filename);
void clean_up();

#endif //GOGGLE_EYED_APPROACH_SERVER_H
