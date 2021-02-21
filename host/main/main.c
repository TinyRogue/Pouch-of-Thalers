#include "../labyrinth-loader/labyrinth-loader.h"
#include "server.h"

int main(void) {
    printf("Entering...");
    if (!initialise("maps/classic-map.txt")) {
        printf("Initializing failed!\n");
        exit(errno);
    }

    clean_up();
    printf("DONE\n");
    exit(EXIT_SUCCESS);
}
