#include "player.h"

int main(void) {
    if (!initialise()) {
        printf("Initialisation fault.\n");
        clean_up();
        exit(EXIT_FAILURE);
    }

    play();

    clean_up();
    exit(EXIT_SUCCESS);
}
