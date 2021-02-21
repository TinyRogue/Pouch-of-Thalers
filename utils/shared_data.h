#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <semaphore.h>


#define PLAYER_LISTNER_SEM "/PLAYER_LSTNR_SEM"
#define HOST_LISTENER_SEM "/HOST_LSTNR_SEM"
#define HOST_PLAYER_PIPE "/HP_PIPE"
#define PLAYER_SIGHT 5
#define MAX_MAP_WIDTH 128
#define MAX_MAP_HEIGHT 28

typedef enum {BLANK, WALL, BUSH, COIN, TREASURE, LARGE_TREASURE, DROPPED_TREASURE, CAMPSITE, SPAWN, PLAYER} field_kind_t;
typedef enum {CPU, HUMAN} player_kind_t;
typedef enum {EAST, NORTH, WEST, SOUTH} dir_t;
typedef enum {WAITING_FOR_PLAYER, REJECTED, PENDING, ACCEPTED} join_approval_t;

typedef struct {
    size_t value;
    field_kind_t kind;
} field_t;

typedef struct {
    int pid;
    int host_pid;
    int carried_treasure;
    int brought_treasure;
    int deaths;
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
    int player_number;
} shared_info_t;

typedef struct {
    sem_t *player_response_sem;
    sem_t *host_response_sem;
    struct comm_shm {
        size_t pid;
        char shm_name[40];
        short join_approval;
        char additional_info[100];
    } *comm_shm;
} comms_t;

#endif
