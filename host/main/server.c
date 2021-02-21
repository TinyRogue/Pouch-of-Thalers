#include "server.h"

static game_t game;
static comms_t comms;

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
static bool tarry_for_player() {

    if (-1 == sem_wait(comms.player_response_sem)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        munmap(comms.comm_shm, sizeof(struct comm_shm));
        return false;
    }
    return true;
}


static bool accept_communication() {
    strncpy(comms.comm_shm->additional_info, "Send acceptance. Waiting for player to fill comms info.", ADDITIONAL_INFO_SIZE - 1);
    sem_post(comms.host_response_sem);
    if (-1 == sem_wait(comms.player_response_sem)) {
        return false;
    }
    return true;
}


static bool fill_server_side_info(int current_slot) {

    game.player_exchange[current_slot].server_side_info.brought_treasure = 0;
    game.player_exchange[current_slot].server_side_info.carried_treasure = 0;
    game.player_exchange[current_slot].server_side_info.deaths = 0;
    game.player_exchange[current_slot].server_side_info.is_slot_taken = true;
    game.player_exchange[current_slot].server_side_info.is_slowed_down = false;
    
    rand_point_on_map(&game.player_exchange[current_slot].server_side_info.spawn_pos_x, &game.player_exchange[current_slot].server_side_info.spawn_pos_y);
    if (game.player_exchange[current_slot].server_side_info.spawn_pos_x == -1 || game.player_exchange[current_slot].server_side_info.spawn_pos_y == -1) {
        strncpy(comms.comm_shm->additional_info, "Couldn't find free space on map.", ADDITIONAL_INFO_SIZE - 1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);
        return false;
    }

    game.player_exchange[current_slot].server_side_info.curr_x = game.player_exchange[current_slot].server_side_info.spawn_pos_x;
    game.player_exchange[current_slot].server_side_info.curr_y = game.player_exchange[current_slot].server_side_info.spawn_pos_y;
    return true;
}


static bool open_player_shm(int current_slot) {

    sprintf(comms.comm_shm->shm_name, "/%d_P_SHM_OF_PID_%lu", current_slot, comms.comm_shm->pid);
    int fd = shm_open(comms.comm_shm->shm_name, O_CREAT | O_RDWR, 0666);
    if (-1 == fd) {
        strncpy(comms.comm_shm->additional_info, "Couldn't open shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);
        return false;
    }
    
    if (-1 == ftruncate(fd, sizeof(shared_info_t))) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms.comm_shm->additional_info, "Couldn't truncate shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);
        close(fd);
        shm_unlink(comms.comm_shm->shm_name);
        return false;
    }

    game.player_exchange[current_slot].shared_info = (shared_info_t*)mmap(NULL, sizeof(shared_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == game.player_exchange[current_slot].shared_info) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms.comm_shm->additional_info, "Couldn't map shm. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);

        close(fd);
        shm_unlink(comms.comm_shm->shm_name);
        return false;
    }

    close(fd);
    sprintf(game.player_exchange[current_slot].server_side_info.shm_name, "%s", comms.comm_shm->shm_name);
    return true;
}


static void fill_shared_info(int current_slot) {

    game.player_exchange[current_slot].shared_info->brought_treasure = 0;
    game.player_exchange[current_slot].shared_info->carried_treasure = 0;
    game.player_exchange[current_slot].shared_info->deaths = 0;
    game.player_exchange[current_slot].shared_info->current_round = game.current_round;
    game.player_exchange[current_slot].shared_info->host_pid = game.host_pid;
    game.player_exchange[current_slot].shared_info->pos_x = game.player_exchange[current_slot].server_side_info.spawn_pos_x;
    game.player_exchange[current_slot].shared_info->pos_y = game.player_exchange[current_slot].server_side_info.spawn_pos_y;
    game.player_exchange[current_slot].shared_info->player_number = current_slot + 1;
    game.player_exchange[current_slot].shared_info->pid = comms.comm_shm->pid;
    
    int start_x = game.player_exchange[current_slot].server_side_info.spawn_pos_x - PLAYER_SIGHT / 2;
    int start_y = game.player_exchange[current_slot].server_side_info.spawn_pos_y - PLAYER_SIGHT / 2;
    for (int row = 0; row < PLAYER_SIGHT; row++) {
        for (int column = 0; column < PLAYER_SIGHT; column++) {
            game.player_exchange[current_slot].shared_info->player_sh_lbrth[row][column] = game.lbrth->lbrth[start_y + row][start_x + column];
        }
    }
}


static bool open_shared_sems(int current_slot) {

    if (sem_init(&game.player_exchange[current_slot].shared_info->host_response, 1, 0)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms.comm_shm->additional_info, "Couldn't init host unnamed sem. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);

        shm_unlink(comms.comm_shm->shm_name);
        munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
        return false;
    }
    
    if (sem_init(&game.player_exchange[current_slot].shared_info->player_response, 1, 0)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        strncpy(comms.comm_shm->additional_info, "Couldn't init player unnamed sem. Internal error.", ADDITIONAL_INFO_SIZE -1);
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.host_response_sem);

        shm_unlink(comms.comm_shm->shm_name);
        munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
        return false;
    }
    return true;
}


pthread_mutex_t player_listener_mut = PTHREAD_MUTEX_INITIALIZER;
static void *player_listener_handler(void *ignored) {

    int current_slot = 0;

    while (true) {
        comms.comm_shm->join_approval = WAITING_FOR_PLAYER;
        if (!tarry_for_player()) {
            return NULL;
        }
        log_message("New willing player has appeared!");
        comms.comm_shm->join_approval = PENDING;

        pthread_mutex_lock(&player_listener_mut);
        if (!accept_communication()) {
            log_message("Communication not accepted.");
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        log_message("Looking for free slot...");
        current_slot = find_free_slot();
        if (FREE_SLOT_UNAVAILABLE ==  current_slot) {
            log_message("No free slot available. Rejecting.");
            strncpy(comms.comm_shm->additional_info, "No slot available.", ADDITIONAL_INFO_SIZE - 1);
            comms.comm_shm->join_approval = REJECTED;
            sem_post(comms.host_response_sem);
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        log_message("Slot found!");
        if (!fill_server_side_info(current_slot)) {
            log_message("Could not fill server side info. Rejecting.");
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        if (!open_player_shm(current_slot)) {
            log_message("Could not open player shm. Rejecting.");
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }

        fill_shared_info(current_slot);

        if (!open_shared_sems(current_slot)) {
            log_message("Could not open shared semaphores. Rejecting.");
            shm_unlink(comms.comm_shm->shm_name);
            munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
            game.player_exchange[current_slot].shared_info = NULL;
            pthread_mutex_unlock(&player_listener_mut);
            continue;
        }
        
        comms.comm_shm->join_approval = ACCEPTED;
        strncpy(comms.comm_shm->additional_info, "Successful connection!", ADDITIONAL_INFO_SIZE - 1);
        sem_post(comms.host_response_sem);

        if (-1 == sem_wait(comms.player_response_sem)) {
            shm_unlink(comms.comm_shm->shm_name);
            sem_destroy(&game.player_exchange[current_slot].shared_info->player_response);
            sem_destroy(&game.player_exchange[current_slot].shared_info->host_response);
            munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
            game.player_exchange[current_slot].shared_info = NULL;

            pthread_mutex_unlock(&player_listener_mut);
            log_message("Final joining step failed. Rejected.");
            continue;
        }

        if (comms.comm_shm->join_approval != ACCEPTED) {
            shm_unlink(comms.comm_shm->shm_name);
            sem_destroy(&game.player_exchange[current_slot].shared_info->player_response);
            sem_destroy(&game.player_exchange[current_slot].shared_info->host_response);
            munmap(game.player_exchange[current_slot].shared_info, sizeof(shared_info_t));
            game.player_exchange[current_slot].shared_info = NULL;
            pthread_mutex_unlock(&player_listener_mut);
            log_message("Player troubled opening shm. Rejected.");
            continue;
        }

        log_message("New player appeared in game!");
        pthread_mutex_unlock(&player_listener_mut);
        sem_post(game.print_map_invoker);
    }

    return NULL;
}


static bool open_player_listener() {

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
        sem_unlink(HOST_LISTENER_SEM);
        sem_unlink(PLAYER_LISTNER_SEM);
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------------------------
//--- Print section ---------------------------------------------------------------------------
static void print_info() {
    char res_server_PID[30] = {"Server's process ID:"};
    char server_PID[10];
    sprintf(server_PID, "%lu", game.host_pid);

    print(strcat(res_server_PID, server_PID), 1, game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10) * 2);
    print("PARAMETER:", 2, game.lbrth->width + 3);
    print("PID:", 3, game.lbrth->width + 3);
    print("Curr X/Y:", 4, game.lbrth->width + 3);
    print("Deaths:", 5, game.lbrth->width + 3);
    print("Player 1", 2, game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10));
    print("Player 2", 2, game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10) * 2);
    print("Player 3", 2, game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10) * 3);
    print("Player 4", 2, game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10) * 4);
    print("Coins", 7, game.lbrth->width + 3);
    print("Carried", 8, game.lbrth->width + 5);
    print("Brought", 9, game.lbrth->width + 5);
    print("Current round: ", 11, game.lbrth->width + 3);
}


static void print_map() {
    for (size_t i = 0; i < game.lbrth->height; ++i) {
        for (size_t j = 0; j < game.lbrth->width; ++j) {
            print_field(game.lbrth->lbrth[i][j].kind, i, j);
        }    
    }
}


static void print_current_round() {
    char round[10];
    sprintf(round, "%lu", game.current_round);
    print(round, 11, game.lbrth->width + 3 + strlen("Current round: "));
}


static void print_players() {

    char pid[10], curr_loc[10], deaths[10], coins[10], player_number[2];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        unsigned short x_pos = game.lbrth->width + 3 + strlen("PARAMETER") + (strlen("Player") + 10) * (i + 1) + strlen("-----") / 2;

        if (game.player_exchange[i].server_side_info.is_slot_taken) {
            sprintf(pid, "%04d", game.player_exchange[i].shared_info->pid);
            print(pid, 3, x_pos);

            sprintf(player_number, "%d", i + 1);
            print_coloured(player_number, PLAYER_COLOUR, game.player_exchange[i].server_side_info.curr_y, game.player_exchange[i].server_side_info.curr_x);
            
            sprintf(curr_loc, "%02d/%02d", game.player_exchange[i].server_side_info.curr_x, game.player_exchange[i].server_side_info.curr_y);
            print(curr_loc, 4, x_pos);

            sprintf(deaths, "%03d", game.player_exchange[i].server_side_info.deaths);
            print(deaths, 5, x_pos);

            sprintf(coins, "%04d", game.player_exchange[i].server_side_info.carried_treasure);
            print(coins, 8, x_pos);

            sprintf(coins, "%04d", game.player_exchange[i].server_side_info.brought_treasure);
            print(coins, 9, x_pos);
        } else {
            print ("-----", 3, x_pos);
            print ("--/--", 4, x_pos);
            print ("-", 5, x_pos + 2);
            print("-", 8, x_pos + 2);
            print("-", 9, x_pos + 2);
        }
    }
}


static void print_beasts() {
    for (size_t i = 0; i < game.beasts.beasts_amount; i++) {
        print_beast(game.beasts.beasts_ptr[i]->shared_info->pos_y, game.beasts.beasts_ptr[i]->shared_info->pos_x);
    }
}


pthread_mutex_t printer_mut = PTHREAD_MUTEX_INITIALIZER;
static void *print_map_on_event(void *ignored) {
    init_screen();
    print_legend(game.lbrth->height - 9, game.lbrth->width + 1);
    print_info();

    while (true) {
        if (-1 == sem_wait(game.print_map_invoker)) {
            printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
            return NULL;
        }

        pthread_mutex_lock(&printer_mut);
        print_map();
        print_current_round();
        print_players();
        print_beasts();
        refresh();
        pthread_mutex_unlock(&printer_mut);
    }

    return NULL;
}
//---------------------------------------------------------------------------------------------
//---Beasts section----------------------------------------------------------------------------
static void kill_beasts() {
    for (size_t i = 0; i < game.beasts.beasts_amount; i++) {
        sem_destroy(&game.beasts.beasts_ptr[i]->shared_info->host_response);
        sem_destroy(&game.beasts.beasts_ptr[i]->shared_info->player_response);
        munmap(game.beasts.beasts_ptr[i]->shared_info, sizeof(shared_info_t));
        shm_unlink(game.beasts.beasts_ptr[i]->shm_name);
        free(*(game.beasts.beasts_ptr + i));
    }
    free(game.beasts.beasts_ptr);
}


static bool open_beast_shm(struct single_beast_t *beast_ptr) {
    sprintf(beast_ptr->shm_name, "/BEAST_SHM_%lu", game.beasts.beasts_amount);

    int fd = shm_open(beast_ptr->shm_name, O_CREAT | O_RDWR, 0666);
    if (-1 == fd) {
        log_message("Couldn't open beast shm. Internal error.");
        return false;
    }
    
    if (-1 == ftruncate(fd, sizeof(shared_info_t))) {
        log_message("Couldn't truncate beast shm. Internal error.");
        close(fd);
        shm_unlink(beast_ptr->shm_name);
        return false;
    }

    beast_ptr->shared_info = (shared_info_t*)mmap(NULL, sizeof(shared_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == beast_ptr->shared_info) {
        log_message("Couldn't map beast shm. Internal error.");
        close(fd);
        shm_unlink(beast_ptr->shm_name);
        return false;
    }

    close(fd);
    return true;
}


static bool open_beast_shared_sems(struct single_beast_t *beast_ptr) {

    if (sem_init(&beast_ptr->shared_info->host_response, 1, 0)) {
        log_message("Couldn't init host unnamed sem for beast. Internal error.");
        return false;
    }
    
    if (sem_init(&beast_ptr->shared_info->player_response, 1, 0)) {
        log_message("Couldn't init host unnamed sem for beast. Internal error.");
        return false;
    }
    return true;
}

//* It's obnoxious one, but time is short
bool was_hunting_successful(shared_info_t *shared_info) {
    if (!shared_info) return false;   
    
    int player_x, player_y;
    int x_distance, y_distance;

    for (int i = 0; i < MAX_PLAYERS; i++) {
        player_x = game.player_exchange[i].server_side_info.curr_x;
        player_y = game.player_exchange[i].server_side_info.curr_y;

        x_distance = shared_info->pos_x - player_x;
        y_distance = shared_info->pos_y - player_y;

        //* Player is not even in beast sight, no purpose in checking sight
        if (!(abs(x_distance) <= PLAYER_SIGHT / 2 && abs(y_distance) <= PLAYER_SIGHT / 2)) {
            continue;
        }
        
        //* For one field distance
        if (y_distance == BEAST_BELOW) {

            switch (x_distance) {
            case LEFT:
                if (game.lbrth->lbrth[shared_info->pos_y - 1][shared_info->pos_x].kind == WALL) {
                    shared_info->dir = EAST;
                    return true;
                }
                shared_info->dir = NORTH;
                return true;
            case BEAST_IN_MIDDLE:
                shared_info->dir = NORTH;
                return true;
            case RIGHT:
                if (game.lbrth->lbrth[shared_info->pos_y - 1][shared_info->pos_x].kind == WALL) {
                    shared_info->dir = WEST;
                    return true;
                }
                shared_info->dir = NORTH;
                return true;
            }

        } else if (y_distance == BEAST_ABOVE) {

            switch (x_distance) {
            case LEFT:
                if (game.lbrth->lbrth[shared_info->pos_y + 1][shared_info->pos_x].kind == WALL) {
                    shared_info->dir = EAST;
                    return true;
                }
                shared_info->dir = SOUTH;
                return true;
            case BEAST_IN_MIDDLE:
                shared_info->dir = SOUTH;
                return true;
            case RIGHT:
                if (game.lbrth->lbrth[shared_info->pos_y + 1][shared_info->pos_x].kind == WALL) {
                    shared_info->dir = WEST;
                    return true;
                }
                shared_info->dir = SOUTH;
                return true;
            }

        } else if (y_distance == BEAST_IN_MIDDLE) {

            switch (x_distance) {
            case LEFT:
                shared_info->dir = EAST;
                return true;
            case RIGHT:
                shared_info->dir = WEST;
                return true;
            }
        }

        //* Two fields or more
        if (y_distance == BEAST_IN_MIDDLE) {
            if (x_distance > RIGHT) {
                if (game.lbrth->lbrth[shared_info->pos_y][shared_info->pos_x - 1].kind != WALL) {
                    shared_info->dir = WEST;
                    return true;
                }
                return false;
            } else if (x_distance < LEFT) {
                if (game.lbrth->lbrth[shared_info->pos_y][shared_info->pos_x + 1].kind != WALL) {
                    shared_info->dir = EAST;
                    return true;
                }
                return false;
            }
        } else if (x_distance == BEAST_IN_MIDDLE) {
            if (y_distance < BEAST_ABOVE) {
                if (game.lbrth->lbrth[shared_info->pos_y + 1][shared_info->pos_x].kind != WALL) {
                    shared_info->dir = SOUTH;
                    return true;
                }
                return false;
            } else if (y_distance > BEAST_BELOW) {
                if (game.lbrth->lbrth[shared_info->pos_y - 1][shared_info->pos_x].kind != WALL) {
                    shared_info->dir = NORTH;
                    return true;
                }
                return false;
            }
        }
    }
    return false;
}


static void move_arbitrarily(shared_info_t *shared_info) {
    if (!shared_info) return;
    
    int direction_to_choose = rand() % 4;
    
    switch (direction_to_choose) {
    case 0:
        shared_info->dir = NORTH;
        break;
    case 1:
        shared_info->dir = SOUTH;
        break;
    case 2:
        shared_info->dir = WEST;
        break;
    case 3:
        shared_info->dir = EAST;
        break;
    }
}


static void* run_beast(void *beast_ptr) {
    struct single_beast_t *beast = (struct single_beast_t*)beast_ptr;

    while (true) {
        bool to_go = true;
        if (!was_hunting_successful(beast->shared_info)) {
            to_go = rand() % 100 > 60;
            move_arbitrarily(beast->shared_info);
        }

        if (to_go) {
            sem_post(&beast->shared_info->player_response);
        }

        if (-1 == sem_wait(&beast->shared_info->host_response)) {
            return NULL;
        }
    }
    return NULL;
}


static bool spawn_beast() {
    int row, column;
    rand_point_on_map(&column, &row);
    if (-1 == column || -1 == row) {
        log_message("Couldn't find location for beast, try again.");
        return false;
    }

    struct single_beast_t **check_ptr = NULL;
    check_ptr = realloc(game.beasts.beasts_ptr, (game.beasts.beasts_amount + 1) * (sizeof(struct single_beast_t**)));
    if (!check_ptr) {
        log_message("Could not expand beast array");
        return false;
    }

    game.beasts.beasts_ptr = check_ptr;

    game.beasts.beasts_ptr[game.beasts.beasts_amount] = (struct single_beast_t*)calloc(1, sizeof(struct single_beast_t));
    if (!game.beasts.beasts_ptr[game.beasts.beasts_amount]) {
        log_message("Could not allocate memory for beast");
        return false;
    }

    game.beasts.beasts_ptr[game.beasts.beasts_amount]->beast_id = game.beasts.beasts_amount;

    if (!open_beast_shm(game.beasts.beasts_ptr[game.beasts.beasts_amount])) {
        log_message("Could not add beast to map. Shm error.");
        free(*(game.beasts.beasts_ptr + game.beasts.beasts_amount));
        *(game.beasts.beasts_ptr + game.beasts.beasts_amount) = NULL;
        return false;
    }

    if (!open_beast_shared_sems(game.beasts.beasts_ptr[game.beasts.beasts_amount])) {
        log_message("Could not open beast unnamed sems. Sem error.");
        munmap(game.beasts.beasts_ptr[game.beasts.beasts_amount]->shared_info, sizeof(shared_info_t));
        shm_unlink(game.beasts.beasts_ptr[game.beasts.beasts_amount]->shm_name);
        free(*(game.beasts.beasts_ptr + game.beasts.beasts_amount));
        *(game.beasts.beasts_ptr + game.beasts.beasts_amount) = NULL;
        return false;
    }

    game.beasts.beasts_ptr[game.beasts.beasts_amount]->shared_info->pos_x = column;
    game.beasts.beasts_ptr[game.beasts.beasts_amount]->shared_info->pos_y = row;

    pthread_create(&game.beasts.beasts_ptr[game.beasts.beasts_amount]->beasts_thrd, NULL, run_beast, game.beasts.beasts_ptr[game.beasts.beasts_amount]);
    game.beasts.beasts_amount++;
    return true;
}
//---------------------------------------------------------------------------------------------
//---Command service section-------------------------------------------------------------------
static void turnon_command_service() {
    int user_action = '\0';
    int row, column;

    while (true) {
        user_action = getch();

        switch (user_action) {

        case 'b':
        case 'B':
            pthread_mutex_lock(&game.general_lock);
            if (!spawn_beast()) {
                pthread_mutex_unlock(&game.general_lock);
                sem_post(game.print_map_invoker);
                continue;
            }
            log_message("Added beast to map");

            pthread_mutex_unlock(&game.general_lock);
            sem_post(game.print_map_invoker);
            break;

        case 'c':
        case 'C':
            pthread_mutex_lock(&game.general_lock);
            rand_point_on_map(&column, &row);
            if (-1 == column || -1 == row) {
                log_message("Couldn't find location, try again.");
                pthread_mutex_unlock(&game.general_lock);
                break;
            }
            game.lbrth->lbrth[row][column].kind = COIN;
            game.lbrth->lbrth[row][column].value = 1;
            pthread_mutex_unlock(&game.general_lock);

            log_message("Added coin to map");
            sem_post(game.print_map_invoker);
            break;

        case 't':
            pthread_mutex_lock(&game.general_lock);
            rand_point_on_map(&column, &row);
            if (-1 == column || -1 == row) {
                log_message("Couldn't find location, try again.");
                pthread_mutex_unlock(&game.general_lock);
                break;
            }
            game.lbrth->lbrth[row][column].kind = TREASURE;
            game.lbrth->lbrth[row][column].value = 10;
            pthread_mutex_unlock(&game.general_lock);
            log_message("Added treasure to map");
            sem_post(game.print_map_invoker);
            break;

        case 'T':
            pthread_mutex_lock(&game.general_lock);
            rand_point_on_map(&column, &row);
            if (-1 == column || -1 == row) {
                log_message("Couldn't find location, try again.");
                pthread_mutex_unlock(&game.general_lock);
                break;
            }
            game.lbrth->lbrth[row][column].kind = LARGE_TREASURE;
            game.lbrth->lbrth[row][column].value = 50;
            pthread_mutex_unlock(&game.general_lock);
            log_message("Added large treasure to map");
            sem_post(game.print_map_invoker);
            break;
        case 'q':
        case 'Q':
            return;
        default:
            log_message("Invalid character. No action has been taken.");
        }
        usleep(SECOND / 200);
        flushinp();
    }
}
//---------------------------------------------------------------------------------------------
//--- Game service section --------------------------------------------------------------------
static bool is_player_alive(int player_number) {
    return kill(game.player_exchange[player_number].shared_info->pid, 0) == 0;
}


static void gather_player_remnants(int player_number) {
    char obituary_notice[40];
    sprintf(obituary_notice, "Player %d is gone. Gathering remnants.", player_number);
    log_message(obituary_notice);

    sem_destroy(&game.player_exchange[player_number].shared_info->player_response);
    sem_destroy(&game.player_exchange[player_number].shared_info->host_response);
    shm_unlink(game.player_exchange[player_number].server_side_info.shm_name);
    munmap(game.player_exchange[player_number].shared_info, sizeof(shared_info_t));
    game.player_exchange[player_number].shared_info = NULL;
    
    memset(&game.player_exchange[player_number].server_side_info, 0, sizeof(struct server_side_info_t));
    game.player_exchange[player_number].server_side_info.is_slot_taken = false;
}


static void kill_player(int index) {
    game.player_exchange[index].server_side_info.carried_treasure = 0;
    game.player_exchange[index].server_side_info.curr_x = game.player_exchange[index].server_side_info.spawn_pos_x;
    game.player_exchange[index].server_side_info.curr_y = game.player_exchange[index].server_side_info.spawn_pos_y;
    game.player_exchange[index].server_side_info.deaths += 1;
    game.player_exchange[index].server_side_info.is_slowed_down = false;
}


static void manage_beasts_moves() {
    shared_info_t *beast_info = NULL;
    int move_row_by, move_column_by;
    field_t field_to_go;

    for (size_t curr_beast = 0; curr_beast < game.beasts.beasts_amount; curr_beast++) {
        if (NULL == game.beasts.beasts_ptr[curr_beast]) continue;

        beast_info = game.beasts.beasts_ptr[curr_beast]->shared_info;
        
        if (sem_trywait(&beast_info->player_response) != 0) {
            sem_post(&beast_info->host_response);
            continue;
        }

        move_row_by = move_column_by = 0;

        switch (beast_info->dir) {
        case NORTH:
            move_row_by = -1;
            break;
        case SOUTH:
            move_row_by = 1;
            break;
        case EAST:
            move_column_by = 1;
            break;
        case WEST:
            move_column_by = -1;
            break;
        }

        field_to_go = game.lbrth->lbrth[beast_info->pos_y + move_row_by][beast_info->pos_x + move_column_by];

        //* Fight beasts with players!
        for (int player = 0; player < MAX_PLAYERS; player++) {
            
            bool equal_x = beast_info->pos_x + move_column_by == game.player_exchange[player].server_side_info.curr_x;
            bool equal_y = beast_info->pos_y + move_row_by == game.player_exchange[player].server_side_info.curr_y;
            
            if (equal_x && equal_y && CAMPSITE != field_to_go.kind) {
                game.lbrth->lbrth[beast_info->pos_y + move_row_by][beast_info->pos_x + move_column_by].value = game.player_exchange[player].server_side_info.carried_treasure + game.player_exchange[player].server_side_info.carried_treasure;
                game.lbrth->lbrth[beast_info->pos_y + move_row_by][beast_info->pos_x + move_column_by].kind = DROPPED_TREASURE;
                kill_player(player);
            }
        }

        if (WALL == field_to_go.kind) {
            sem_post(&beast_info->host_response);
            continue;
        } else {
            beast_info->pos_x = beast_info->pos_x + move_column_by;
            beast_info->pos_y = beast_info->pos_y + move_row_by;
        }

        sem_post(&beast_info->host_response);
    }
}


static void manage_players_moves() {
    int curr_pos_x = -1;
    int curr_pos_y = -1;
    int move_row_by, move_column_by;
    field_t field_to_go;
    struct server_side_info_t *host_side_info = NULL;
    shared_info_t *shared_info = NULL;

    for (int curr_player = 0; curr_player < MAX_PLAYERS; curr_player++) {
        
        host_side_info = &game.player_exchange[curr_player].server_side_info;
        shared_info = game.player_exchange[curr_player].shared_info;

        if (!host_side_info->is_slot_taken) continue;
        if (sem_trywait(&shared_info->player_response) != 0) {
            continue;
        }

        if (host_side_info->is_slowed_down) {
            host_side_info->is_slowed_down = false;
            continue;
        }

        move_row_by = move_column_by = 0;
        curr_pos_x = host_side_info->curr_x;
        curr_pos_y = host_side_info->curr_y;
        
        switch (shared_info->dir) {
        case NORTH:
            move_row_by = -1;
            break;
        case SOUTH:
            move_row_by = 1;
            break;
        case EAST:
            move_column_by = 1;
            break;
        case WEST:
            move_column_by = -1;
            break;
        }

        field_to_go = game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by];

        if (field_to_go.kind == WALL) {
            break;
        } else if (field_to_go.kind == BLANK) {
            host_side_info->curr_x = curr_pos_x + move_column_by;
            host_side_info->curr_y = curr_pos_y + move_row_by;

        } else if (field_to_go.kind == COIN || field_to_go.kind == TREASURE || field_to_go.kind == LARGE_TREASURE || field_to_go.kind == DROPPED_TREASURE) {
            host_side_info->curr_x = curr_pos_x + move_column_by;
            host_side_info->curr_y = curr_pos_y + move_row_by;

            host_side_info->carried_treasure += field_to_go.value;

            game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].kind = BLANK;
            game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].value = 0;

        } else if (field_to_go.kind == CAMPSITE) {
            host_side_info->curr_x = curr_pos_x + move_column_by;
            host_side_info->curr_y = curr_pos_y + move_row_by;

            host_side_info->brought_treasure += host_side_info->carried_treasure;
            host_side_info->carried_treasure = 0;

        } else if (field_to_go.kind == BUSH) {
            host_side_info->curr_x = curr_pos_x + move_column_by;
            host_side_info->curr_y = curr_pos_y + move_row_by;
            host_side_info->is_slowed_down = true;
        }

        //* Players fight! =)
        for (int other_player = 0; other_player < MAX_PLAYERS; other_player++) {
            if (curr_player == other_player) continue;
            
            bool equal_x = curr_pos_x + move_column_by == game.player_exchange[other_player].server_side_info.curr_x;
            bool equal_y = curr_pos_y + move_row_by == game.player_exchange[other_player].server_side_info.curr_y;
            
            if (equal_x && equal_y && CAMPSITE != field_to_go.kind) {
                game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].value = game.player_exchange[curr_player].server_side_info.carried_treasure + game.player_exchange[other_player].server_side_info.carried_treasure;
                game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].kind = DROPPED_TREASURE;
                kill_player(curr_player);
                kill_player(other_player);
                break;
            }
        }

        for (size_t beast_num = 0; beast_num < game.beasts.beasts_amount; beast_num++) {
            
            bool equal_x = curr_pos_x + move_column_by == game.beasts.beasts_ptr[beast_num]->shared_info->pos_x;
            bool equal_y = curr_pos_y + move_row_by == game.beasts.beasts_ptr[beast_num]->shared_info->pos_y;
            
            if (equal_x && equal_y && CAMPSITE != field_to_go.kind) {
                game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].value = game.player_exchange[curr_player].server_side_info.carried_treasure;
                game.lbrth->lbrth[curr_pos_y + move_row_by][curr_pos_x + move_column_by].kind = DROPPED_TREASURE;
                kill_player(curr_player);
                break;
            }
        }

    }
}


static void update_shared_info() {
    struct server_side_info_t *host_side_info = NULL;
    shared_info_t *shared_info = NULL;

    for (int curr_player = 0; curr_player < MAX_PLAYERS; curr_player++) {
        
        host_side_info = &game.player_exchange[curr_player].server_side_info;
        shared_info = game.player_exchange[curr_player].shared_info;

        if (!host_side_info->is_slot_taken) continue;

        shared_info->current_round = game.current_round;
        shared_info->deaths = host_side_info->deaths;
        shared_info->pos_x = host_side_info->curr_x;
        shared_info->pos_y = host_side_info->curr_y;
        shared_info->brought_treasure = host_side_info->brought_treasure;
        shared_info->carried_treasure = host_side_info->carried_treasure;

        int corner_x = host_side_info->curr_x - PLAYER_SIGHT / 2;
        int corner_y = host_side_info->curr_y - PLAYER_SIGHT / 2;

        for (int row = 0; row < PLAYER_SIGHT; row++) {
            for (int column = 0; column < PLAYER_SIGHT; column++) {
                game.player_exchange[curr_player].shared_info->player_sh_lbrth[row][column] = game.lbrth->lbrth[corner_y + row][corner_x + column];
            }
        }
        
        // * Fill map with players if in sight
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (curr_player == i) continue;

            int otherp_x = game.player_exchange[i].server_side_info.curr_x;
            int otherp_y = game.player_exchange[i].server_side_info.curr_y;

            if (corner_x <= otherp_x && corner_x + PLAYER_SIGHT > otherp_x && corner_y <= otherp_y && corner_y + PLAYER_SIGHT > otherp_y) {
                game.player_exchange[curr_player].shared_info->player_sh_lbrth[otherp_y - corner_y][otherp_x - corner_x] = (field_t){.kind = PLAYER, .value = 0};
            }
        }

        // * Fill map with beasts if in sight
        for (size_t i = 0; i < game.beasts.beasts_amount; i++) {

            int beast_x = game.beasts.beasts_ptr[i]->shared_info->pos_x;
            int beast_y = game.beasts.beasts_ptr[i]->shared_info->pos_y;

            if (corner_x <= beast_x && corner_x + PLAYER_SIGHT > beast_x && corner_y <= beast_y && corner_y + PLAYER_SIGHT > beast_y) {
                game.player_exchange[curr_player].shared_info->player_sh_lbrth[beast_y - corner_y][beast_x - corner_x] = (field_t){.kind = BEAST, .value = 0};
            }
        }

        sem_post(&shared_info->host_response);
    }
}


static void* manage_movements(void *ignored) {
    
    while (true) {
        
        pthread_mutex_lock(&game.general_lock);
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!game.player_exchange[i].server_side_info.is_slot_taken) continue;
            if (!is_player_alive(i)) {
                gather_player_remnants(i);
            }
        }

        manage_players_moves();
        manage_beasts_moves();

        game.current_round++;
        update_shared_info();
        pthread_mutex_unlock(&game.general_lock);
        
        sem_post(game.print_map_invoker);
        usleep(SECOND / 4);
    }
    return NULL;
}

//---------------------------------------------------------------------------------------------

bool initialise(const char* const filename) {
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
        destroy_labyrinth(game.lbrth);
        pthread_mutex_destroy(&game.general_lock);
        return false;
    }

    pthread_t printer_thrd;
    if (pthread_create(&printer_thrd, NULL, print_map_on_event, NULL)) {
        printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        pthread_mutex_destroy(&game.general_lock);
        destroy_labyrinth(game.lbrth);
        sem_unlink(MAP_PRNTR_SEM);
        return false;
    }

    if (!init_logger(game.lbrth->height - 9, game.lbrth->width + 1 + 36, 9, 120, &printer_mut)) {
        pthread_mutex_destroy(&game.general_lock);
        destroy_labyrinth(game.lbrth);
        sem_unlink(MAP_PRNTR_SEM);
    }
    
    //* Player listener section
    if (false == open_player_listener()) {
        pthread_mutex_destroy(&game.general_lock);
        destroy_labyrinth(game.lbrth);
        sem_unlink(MAP_PRNTR_SEM);
        return false;
    }

    //* Game logic section
    pthread_t manage_movements_thrd;
        if (pthread_create(&manage_movements_thrd, NULL, manage_movements, NULL)) {
        pthread_mutex_destroy(&game.general_lock);
        destroy_labyrinth(game.lbrth);
        sem_unlink(MAP_PRNTR_SEM);
        sem_unlink(HOST_LISTENER_SEM);
        sem_unlink(PLAYER_LISTNER_SEM);
        shm_unlink(HOST_PLAYER_PIPE);
       return false;
    }

    //* Final section
    //* Keyboard command service section
    turnon_command_service();
    return true;
}


static void remove_players() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!game.player_exchange[i].server_side_info.is_slot_taken) continue;
        gather_player_remnants(i);
    }
}


void clean_up() {
    kill_beasts();
    remove_players();
    destroy_labyrinth(game.lbrth);
    sem_close(comms.host_response_sem);
    sem_close(comms.player_response_sem);
    sem_close(game.print_map_invoker);
    sem_unlink(MAP_PRNTR_SEM);
    sem_unlink(PLAYER_LISTNER_SEM);
    sem_unlink(HOST_LISTENER_SEM);
    shm_unlink(HOST_PLAYER_PIPE);
    destroy_logger();
    pthread_mutex_destroy(&game.general_lock);
    endwin();
}
