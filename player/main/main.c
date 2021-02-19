#include "player.h"

int main(void) {
    if (!initialise()) {
        printf("Initialisation fault.\n");
        clean_up();
        exit(EXIT_FAILURE);
    }

    clean_up();
    exit(EXIT_SUCCESS);
}
