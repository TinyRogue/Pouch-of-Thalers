#include "player.h"

shared_info_t *my_info;
sem_t map_invoker_sem;

//--- Player joining section ------------------------------------------------------------------
static bool join_the_game() {
    int width = 0, height = 0;
    getmaxyx(stdscr, height, width);

    //* Openening general connection memory section
    int fd = shm_open(HOST_PLAYER_PIPE, O_RDWR, 0666);
    if (-1 == fd) {    
        char message[] = "Couldn't open connection shared memory. Probably server is down.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    comms_t comms;
    comms.comm_shm = (struct comm_shm*)mmap(NULL, sizeof(struct comm_shm), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == comms.comm_shm) {
        char message[] = "Couldn't map shared memory. Internal error.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }
    close(fd);

    comms.player_response_sem = sem_open(PLAYER_LISTNER_SEM, 0);
    if (comms.player_response_sem == SEM_FAILED) {
        char message[] = "Couldn't open player response sem. Probably server is down.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    comms.host_response_sem = sem_open(HOST_LISTENER_SEM, 0);
    if (comms.host_response_sem == SEM_FAILED) {
        char message[] = "Server rejected joining. More info below.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    sem_post(comms.player_response_sem);
    sem_wait(comms.host_response_sem);

    if (comms.comm_shm->join_approval != PENDING) {
        char message[] = "Server rejected joining. More info below.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print(comms.comm_shm->additional_info, height / 2 + 1, width / 2 + strlen(comms.comm_shm->additional_info) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }
    
    comms.comm_shm->pid = getpid();

    sem_post(comms.player_response_sem);
    sem_wait(comms.host_response_sem);
    if (comms.comm_shm->join_approval != ACCEPTED) {
        char message[] = "Server rejected joining. More info below.";

        print(message, height / 2, width / 2 - strlen(message) / 2);
        print(comms.comm_shm->additional_info, height / 2 + 1, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    //* Openening specified memory section
    fd = shm_open(comms.comm_shm->shm_name, O_RDWR, 0666);
    if (-1 == fd) {
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.player_response_sem);

        char message[] = "Could not open memory given by server. Aborting...";
        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    my_info = (shared_info_t*)mmap(NULL, sizeof(shared_info_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (MAP_FAILED == my_info) {
        comms.comm_shm->join_approval = REJECTED;
        sem_post(comms.player_response_sem);

        char message[] = "Could not map memory given by server. Aborting...";
        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        getchar();
        return false;
    }

    sem_post(comms.player_response_sem);
    return true;
}
//---------------------------------------------------------------------------------------------
//--- Print section ---------------------------------------------------------------------------
static int old_x[4] = {NO_PLAYER, NO_PLAYER, NO_PLAYER, NO_PLAYER};
static int old_y[4] = {NO_PLAYER, NO_PLAYER, NO_PLAYER, NO_PLAYER};

static void forget_players() { 
    for (int i = 0; i < 4; i++) {
        if (-1 == old_x[i]) continue;
        if (abs(my_info->pos_x - old_x[i]) > PLAYER_SIGHT / 2 || abs(my_info->pos_y - old_y[i]) > PLAYER_SIGHT / 2) {
            print_field(BLANK, old_y[i], old_x[i]);
            old_x[i] = old_y[i] = NO_PLAYER;
        }
    }
    
    old_x[3] = my_info->pos_x;
    old_y[3] = my_info->pos_y;
}


void print_map() {
    int corner_x = my_info->pos_x - 1;
    int corner_y = my_info->pos_y - 1;

    for (int i = 0; i < PLAYER_SIGHT; i++) {
        for (int j = 0; j < PLAYER_SIGHT; j++) {

            if (PLAYER == my_info->player_sh_lbrth[i][j].kind) {
                for (int x = 0; x < 3; x++) {
                    if (old_x[x] == NO_PLAYER) {
                        old_x[x] = j + corner_x;
                        old_y[x] = i + corner_y;
                    }
                }
            }

            print_field(my_info->player_sh_lbrth[i][j].kind, corner_y + i, corner_x + j);
        }
    }
}


void print_info() {
    char host_pid[30], round_number[30], player_number[30], current_loc[30], deaths[30], coins_found[30], coins_brought[30];

    sprintf(host_pid, "Server's pid: %d", my_info->host_pid);
    sprintf(round_number, "Round number: %lu", my_info->current_round);
    sprintf(player_number, "Number: %d", my_info->player_number);
    sprintf(current_loc, "Curr X/Y: %02d/%02d", my_info->pos_x, my_info->pos_y);
    sprintf(deaths, "Deaths: %02d", my_info->deaths);
    sprintf(coins_found, "Coins found: %05d", my_info->carried_treasure);
    sprintf(coins_brought, "Coins brought: %05d", my_info->brought_treasure);

    print(host_pid, 2, MAX_MAP_WIDTH + 1);
    print(round_number, 3, MAX_MAP_WIDTH + 1);
    print("---Player---", 4, MAX_MAP_WIDTH + 1);
    print(player_number, 5, MAX_MAP_WIDTH + 1);
    print(current_loc, 6, MAX_MAP_WIDTH + 1);
    print(deaths, 7, MAX_MAP_WIDTH + 1);
    print(coins_found, 9, MAX_MAP_WIDTH + 1);
    print(coins_brought, 10, MAX_MAP_WIDTH + 1);
}

pthread_mutex_t printer_mut = PTHREAD_MUTEX_INITIALIZER;
void *print_map_on_event(void *ignored) {

    init_screen();
    log_message("Successfully joined to game!");
    print_legend(MAX_MAP_HEIGHT - 9, MAX_MAP_WIDTH + 1);

    while (true) {
        
        if (-1 == sem_wait(&my_info->host_response)) {
            printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
            return NULL;
        }
        
        pthread_mutex_lock(&printer_mut);
        print_map();
        print_info();
        print_player(my_info->player_number, my_info->pos_y, my_info->pos_x);
        forget_players();
        refresh();
        pthread_mutex_unlock(&printer_mut);
    }

    return NULL;
}


//---------------------------------------------------------------------------------------------
bool initialise() {
    // //* Display init section
    if (!init_logger(MAX_MAP_HEIGHT - 9, MAX_MAP_WIDTH + 1 + 36, 9, 120, &printer_mut)) {
        printf("Couldn't initialise // logger. Aborting...\n");
        return false;
    }
    init_screen();

    //* Connecting to server section
    if (!join_the_game()) {
        return false;
    }

    //* Display main section
    pthread_t printer_thrd;
    if (pthread_create(&printer_thrd, NULL, print_map_on_event, NULL)) {
        int width = 0, height = 0;
        getmaxyx(stdscr, height, width);

        char message[] = "Map invoker sem init failed. Aborting...";
        print(message, height / 2, width / 2 - strlen(message) / 2);
        print("Press any key to continue.", height / 2 + 2, width / 2 - strlen(message) / 2);
        sem_destroy(&map_invoker_sem);
        getchar();
        return false;
    }

    sem_post(&map_invoker_sem);
    return true;
}


void play() {
    int user_action = '\0';
    
    while (true) {
        user_action = tolower(getch());
        switch (user_action) {
        case KEY_UP:
        case 'w':
            log_message("Trying to move up.");
            my_info->dir = NORTH;
            break;
        case KEY_DOWN:
        case 's':
            log_message("Trying to move down.");
            my_info->dir = SOUTH;
            break;
        case KEY_LEFT:
        case 'a':
            log_message("Trying to move left.");
            my_info->dir = WEST;
            break;
        case KEY_RIGHT:
        case 'd':
            log_message("Trying to move right.");
            my_info->dir = EAST;
            break;
        case 'q':
            log_message("Exiting. Goodbye!");
            return;
        default:
            log_message("Invalid character. No action has been taken.");
        }
        int check_val = 0;
        sem_getvalue(&my_info->player_response, &check_val);
        
        if (check_val != 0) {
            flushinp();
            continue;
        }

        sem_post(&my_info->player_response);
        sem_post(&map_invoker_sem);

        usleep(50000);
        flushinp();
    }
}


void clean_up() {
    munmap(my_info, sizeof(shared_info_t));
    sem_destroy(&map_invoker_sem);
    destroy_logger();
    endwin();
}
