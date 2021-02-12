#include "server.h"

static game_t game;


//static bool open_player_listener() {
//
//}
//
//
//void *player_listener(void *ignored) {
//
//}

static void print_map() {

    for (size_t i = 0; i < game.lbrth->height; ++i) {
        for (size_t j = 0; j < game.lbrth->width; ++j) {
            print_field(game.lbrth->lbrth[i][j].kind, i, j);
        }    
    }
}


static void *print_map_on_event(void *ignored) {
    init_screen();

    while (true) {
        if (-1 == sem_wait(game.print_map_invoker)) {
            printf("Error: %s %s %d\n", strerror(errno), __FILE__, __LINE__);
        }
        print_map();
    }

    return NULL;
}


bool initialize(const char * const filename) {
    //* game_t vars initialisation section
    game.current_round = 0;
    game.server_pid = getpid();

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
    pthread_create(&printer_thrd, NULL, print_map_on_event, NULL);

    //* Final section
    getchar();
    return true;
}

void clean_up() {
    endwin();
    pthread_mutex_destroy(&game.general_lock);
    sem_unlink(MAP_PRNTR_SEM);
}