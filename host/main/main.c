#include "../labyrinth-loader/labyrinth-loader.h"
#include "server.h"

int main(void) {
    printf("Entering...");
    if (!initialise("maps/blank.txt")) {
        printf("Initializing failed!\n");
        exit(errno);
    }

    clean_up();
    printf("DONE\n");
    exit(EXIT_SUCCESS);
}
