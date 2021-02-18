#include "server.h"

static game_t game;

//--- Player listener section -----------------------------------------------------------------
//--- Helper fucntions ---
static void rand_point_on_map(int *column, int *row) {
    int add_attempt = 0;
    int max_attemps = 30;

    *column = rand() % (game.lbrth->width - 2) + 1;
    *row = rand() % (game.lbrth->height - 2) + 1;

    while (game.lbrth->lbrth[*row][*column].kind != BLANK) {
        *column = rand() % (game.lbrth->width - 2) + 1;
        *row = rand() % (game.lbrth->height - 2) + 1;

        for (; *column < (long long)(game.lbrth->width - 1); (*column)++) {
            if (game.lbrth->lbrth[*row][*column].kind == BLANK) {
                return;
            }
        }

        if (add_attempt++ > max_attemps) {
            *column = -1;
            *row = -1;
            return;
        }
    }
}


static int find_free_slot() {

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.player_exchange[i].server_side_info.is_slot_taken) return i;
    }
    return FREE_SLOT_UNAVAILABLE;
}


//--- Main functions ---
static bool tarry_for_player(comms_t *comms) {

    // strncpy(comms->comm_shm->additional_info, "Waiting for player to show up.", ADDITIONAL_INFO_SIZE - 1);
    if (-1 == sem_wait(comms->player_response_sem)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        munmap(comms->comm_shm, sizeof(struct comm_shm));
        return false;
    }
    return true;
}


static bool accept_communication(comms_t *comms) {

    strncpy(comms->comm_shm->additional_info, "Send acceptance. Waiting for player to fill comms info.", ADDITIONAL_INFO_SIZE - 1);
    sem_post(comms->host_response_sem);
    if (-1 == sem_wait(comms->player_response_sem)) {
        return false;
    }
    return true;
}


static bool fill_server_side_info(comms_t *comms, int current_slot) {

    game.player_exchange[current_slot].server_side_info.brought_treasure = 0;
    game.player_exchange[current_slot].server_side_info.carried_treasure = 0;
    game.player_exchange[current_slot].server_side_info.is_slot_taken = true;
    game.player_exchange[current_slot].server_side_info.is_slowed_down = false;
    
    rand_point_on_map(&game.player_exchange[current_slot].server_side_info.spawn_pos_x, &game.player_exchange[current_slot].server_side_info.spawn_pos_y);
    if (game.player_exchange[current_slot].server_side_info.spawn_pos_x == -1 || game.player_exchange[current_slot].server_side_info.spawn_pos_y == -1) {
        strncpy(comms->comm_shm->additional_info, "Couldn't find free space on map.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);
        return false;
    }
    return true;
}


static bool open_player_shm(comms_t *comms, int current_slot) {

    sprintf(comms->comm_shm->shm_name, "/%d_P_SHM_OF_PID_%lu", current_slot, comms->comm_shm->pid);
    int fd = shm_open(comms->comm_shm->shm_name, O_CREAT | O_RDWR, 0666);
    if (-1 == fd) {
        strncpy(comms->comm_shm->additional_info, "Couldn't open shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);
        return false;
    }
    
    if (-1 == ftruncate(fd, sizeof(shared_info_t))) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms->comm_shm->additional_info, "Couldn't truncate shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);
        close(fd);
        shm_unlink(comms->comm_shm->shm_name);
        return false;
    }

    game.player_exchange[current_slot].shared_info = (shared_info_t*)mmap(NULL, sizeof(shared_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == game.player_exchange[current_slot].shared_info) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms->comm_shm->additional_info, "Couldn't map shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);

        close(fd);
        shm_unlink(comms->comm_shm->shm_name);
        return false;
    }

    close(fd);
    sprintf(game.player_exchange[current_slot].server_side_info.shm_name, "%s", comms->comm_shm->shm_name);
    return true;
}


static void fill_shared_info(int current_slot) {

    game.player_exchange[current_slot].shared_info->brought_treasure = 0;
    game.player_exchange[current_slot].shared_info->carried_treasure = 0;
    game.player_exchange[current_slot].shared_info->current_round = game.current_round;
    game.player_exchange[current_slot].shared_info->host_pid = game.host_pid;
    game.player_exchange[current_slot].shared_info->pos_x = game.player_exchange[current_slot].server_side_info.spawn_pos_x;
    game.player_exchange[current_slot].shared_info->pos_y = game.player_exchange[current_slot].server_side_info.spawn_pos_y;
    
    for (int row = 0; row < PLAYER_SIGHT; row++) {
        for (int column = 0; column < PLAYER_SIGHT; column++) {
            game.player_exchange[current_slot].shared_info->player_sh_lbrth[row][column] = game.lbrth->lbrth[row][column];
        }
    }
}


static bool open_shared_sems(comms_t *comms, int current_slot) {

    if (sem_init(&game.player_exchange[current_slot].shared_info->host_response, 1, 0)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms->comm_shm->additional_info, "Couldn't init host unnamed sem. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);

        shm_unlink(comms->comm_shm->shm_name);
        munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
        return false;
    }
    
    if (sem_init(&game.player_exchange[current_slot].shared_info->player_response, 1, 0)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms->comm_shm->additional_info, "Couldn't init player unnamed sem. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms->comm_shm->can_join = false;
        sem_post(comms->host_response_sem);

        shm_unlink(comms->comm_shm->shm_name);
        munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
        return false;
    }
    return true;
}


pthread_mutex_t player_listener_mut = PTHREAD_MUTEX_INITIALIZER;
static void *player_listener_handler(void *comm_sems) {

    comms_t *comms = (comms_t *)comm_sems;
    int current_slot = 0;

    while (true) {
        if (!tarry_for_player(comms)) {
            return NULL;
        }

        pthread_mutex_lock(&player_listener_mut);
        
        if (!accept_communication(comms)) {
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        current_slot = find_free_slot();
        if (FREE_SLOT_UNAVAILABLE ==  current_slot) {
            strncpy(comms->comm_shm->additional_info, "No slot available.", ADDITIONAL_INFO_SIZE - 1);
            comms->comm_shm->can_join = false;
            sem_post(comms->host_response_sem);
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        if (!fill_server_side_info(comms, current_slot)) {
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        if (!open_player_shm(comms, current_slot)) {
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        fill_shared_info(current_slot);

        if (!open_shared_sems(comms, current_slot)) {
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }
        
        comms->comm_shm->can_join = true;
        strncpy(comms->comm_shm->additional_info, "Successful connection!", ADDITIONAL_INFO_SIZE - 1);
        sem_post(comms->host_response_sem);

        pthread_mutex_unlock(&player_listener_mut);
    }

    return NULL;
}


static bool open_player_listener() {
    comms_t comms;

    comms.player_response_sem = sem_open(PLAYER_LISTNER_SEM, O_CREAT | O_EXCL, 0666, 0);
    if (SEM_FAILED == comms.player_response_sem) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        return false;
    }

    comms.host_response_sem = sem_open(HOST_LISTENER_SEM, O_CREAT | O_EXCL, 0600, 0);
    if (SEM_FAILED == comms.host_response_sem) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        sem_unlink(PLAYER_LISTNER_SEM);
        return false;
    }

    int fd = shm_open(HOST_PLAYER_PIPE, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (-1 == fd) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        sem_unlink(HOST_LISTENER_SEM);
        sem_unlink(PLAYER_LISTNER_SEM);
        return false;
    }

    if (-1 == ftruncate(fd, sizeof(struct comm_shm))) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        sem_unlink(HOST_LISTENER_SEM);
        sem_unlink(PLAYER_LISTNER_SEM);
        return false;
    }

    comms.comm_shm = (struct comm_shm*)mmap(NULL, sizeof(struct comm_shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == comms.comm_shm) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        close(fd);
        sem_unlink(HOST_LISTENER_SEM);
        sem_unlink(PLAYER_LISTNER_SEM);
        return false;
    }

    close(fd);

    pthread_t player_listener_thread;
    if (pthread_create(&player_listener_thread, NULL, player_listener_handler, (void*)&comms)) {
        sem_unlink(PLAYER_LISTNER_SEM);
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------------------------
//--- Print section ---------------------------------------------------------------------------
//TODO: implement
static void print_players() {
    
}

//TODO: implement
static void print_beats() {

}

static void print_map() {

    for (size_t i = 0; i < game.lbrth->height; ++i) {
        for (size_t j = 0; j < game.lbrth->width; ++j) {
            print_field(game.lbrth->lbrth[i][j].kind, i, j);
        }    
    }
}


static void *print_map_on_event(void *ignored) {
    pthread_mutex_t printer_mut;

    if (pthread_mutex_init(&printer_mut, NULL)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        return NULL;
    }
    init_screen();
    
    while (true) {
        if (-1 == sem_wait(game.print_map_invoker)) {
            printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
            return NULL;
        }

        pthread_mutex_lock(&printer_mut);
        print_map();
        pthread_mutex_unlock(&printer_mut);
    }

    pthread_mutex_destroy(&printer_mut);
    return NULL;
}
//---------------------------------------------------------------------------------------------


bool initialize(const char* const filename) {
    srand(time(NULL));
    //* game_t vars initialisation section
    game.current_round = 0;
    game.host_pid = getpid();
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        game.player_exchange[i].shared_info = NULL; //* <-- this will be opened as shared memory
        game.player_exchange[i].server_side_info.is_slot_taken = false;
    }

    //* General synchronization section
    if (pthread_mutex_init(&game.general_lock, NULL)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        return false;
    }

    //* Getting labyrinth section
    game.lbrth = get_labyrinth_from_file(filename);
    if (!game.lbrth) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        pthread_mutex_destroy(&game.general_lock);
        return false;
    }

    //* Map displaying section
    game.print_map_invoker = sem_open(MAP_PRNTR_SEM, O_CREAT | O_EXCL, 0666, 1);
    if (SEM_FAILED == game.print_map_invoker) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        pthread_mutex_destroy(&game.general_lock);
        return false;
    }

    pthread_t printer_thrd;
    if (pthread_create(&printer_thrd, NULL, print_map_on_event, NULL)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        pthread_mutex_destroy(&game.general_lock);
        sem_unlink(MAP_PRNTR_SEM);
        return false;
    }

    //* Player listener section
    if (false == open_player_listener()) {
        pthread_mutex_destroy(&game.general_lock);
        sem_unlink(MAP_PRNTR_SEM);
        return false;
    }

    //* Final section
    getchar();
    return true;
}


void clean_up() {
    endwin();
    pthread_mutex_destroy(&game.general_lock);
    sem_unlink(MAP_PRNTR_SEM);
    sem_unlink(PLAYER_LISTNER_SEM);
    sem_unlink(HOST_LISTENER_SEM);
    shm_unlink(HOST_PLAYER_PIPE);
}
