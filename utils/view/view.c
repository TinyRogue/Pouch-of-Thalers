#include "view.h"


static void print_background() {
    int width = 0, height = 0;
    getmaxyx(stdscr, height, width);
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            print_field(BLANK, i, j);
        }
    }
}


void init_screen() {
    initscr();
    start_color();
    noecho();
    raw();
    curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors() == FALSE){
        perror("Terminal has no colours");
    }

    start_color();
    init_color(COLOR_BLUE, 150, 430, 680);
    init_color(COLOR_YELLOW, 1000, 900, 100);
    init_color(COLOR_WHITE, 1000, 990, 960);
    init_color(COLOR_RED, 600, 100, 100);
    init_color(COLOR_BLACK, 100, 100, 100);
    
    init_pair(WALL_COLOUR, COLOR_BLACK, COLOR_BLACK);
    init_pair(BUSHES_COLOUR, COLOR_GREEN, COLOR_WHITE);
    init_pair(BLANK_COLOUR, COLOR_WHITE, COLOR_WHITE);
    init_pair(BEAST_COLOUR, COLOR_BLACK, COLOR_RED);
    init_pair(TREASURE_COLOUR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(CAMPSITE_COLOUR, COLOR_BLACK, COLOR_GREEN);
    init_pair(PLAYER_COLOUR, COLOR_WHITE, COLOR_BLUE);
    init_pair(TEXT_COLOUR, COLOR_BLACK, COLOR_WHITE);
    init_pair(BEAST_COLOUR, COLOR_YELLOW, COLOR_RED);

    print_background();
}


void print_field(const field_kind_t type, const int row, const int column) {
    colours_t current_colour;
    char current_character;

    switch (type){
        case BLANK:
            current_character = ' ';
            current_colour = BLANK_COLOUR;
            break;
        case WALL:
            current_character = '|';
            current_colour = WALL_COLOUR;
            break;
        case BUSH:
            current_character = '#';
            current_colour = BUSHES_COLOUR;
            break;
        case COIN:
            current_character = 'c';
            current_colour = TREASURE_COLOUR;
            break;
        case TREASURE:
            current_character = 't';
            current_colour = TREASURE_COLOUR;
            break;
        case LARGE_TREASURE:
            current_character = 'T';
            current_colour = TREASURE_COLOUR;
            break;
        case DROPPED_TREASURE:
            current_character = 'D';
            current_colour = TREASURE_COLOUR;
            break;
        case CAMPSITE:
            current_character = 'A';
            current_colour = CAMPSITE_COLOUR;
            break;
        default:
            perror("Invalid use of print");
            return;
    }
    attron(COLOR_PAIR(current_colour));
    mvprintw(row, column, "%c", current_character);
    attroff(COLOR_PAIR(current_colour));
    refresh();
}


void print_beast(const int row, const int column) {
    attron(COLOR_PAIR(BEAST_COLOUR));
    mvprintw(row, column, "*");
    attroff(COLOR_PAIR(BEAST_COLOUR));
    refresh();
}


void print_player(const short player_number, const int row, const int column) {
    attron(COLOR_PAIR(PLAYER_COLOUR));
    mvprintw(row, column, "%d", player_number);
    attroff(COLOR_PAIR(PLAYER_COLOUR));
    refresh();
}


void print(const char* const message, const int row, const int column) {
    attron(COLOR_PAIR(TEXT_COLOUR));
    mvprintw(row, column, "%s", message);
    attroff(COLOR_PAIR(TEXT_COLOUR));
    refresh();
}

void print_coloured(const char* const message, const colours_t colour, const int row, const int column) {
    attron(COLOR_PAIR(colour));
    mvprintw(row, column, "%s", message);
    attroff(COLOR_PAIR(colour));
    refresh();
}

void print_legend(int row, int column) {
    const char *legend[] = {
        "Legend: ",
        "1234", " - Players",
        " " , " - Wall:",
        "#", " - Bushes",
        "*", " - Enemy:",
        "c", " - One coin",
        "t", " - Treasure (5 coins)",
        "T", " - Large treasure (10 coins)",
        "A", " - Campsite", NULL
    };

    const colours_t next_colours[] = {
        TEXT_COLOUR,
        PLAYER_COLOUR, TEXT_COLOUR,
        WALL_COLOUR, TEXT_COLOUR,
        BUSHES_COLOUR, TEXT_COLOUR,
        BEAST_COLOUR, TEXT_COLOUR,
        TREASURE_COLOUR, TEXT_COLOUR,
        TREASURE_COLOUR, TEXT_COLOUR, 
        TREASURE_COLOUR, TEXT_COLOUR,
        CAMPSITE_COLOUR, TEXT_COLOUR
    };

    const int text_position[] = {
        column, row,
        column, row + 1, column + strlen("1234"), row + 1,
        column, row + 2, column + strlen(" "), row + 2,
        column, row + 3, column + strlen("#"), row + 3,
        column, row + 4, column + strlen("*"), row + 4,
        column, row + 5, column + strlen("c"), row + 5,
        column, row + 6, column + strlen("t"), row + 6,
        column, row + 7, column + strlen("T"), row + 7,
        column, row + 8, column + strlen("A"), row + 8,
    };
    
    for (int i = 0; *(legend + i); i++) {
        print_coloured(*(legend + i), next_colours[i], text_position[i * 2 + 1], text_position[i * 2]);
    }
    refresh();
}

// Logger section
static logger_t logger;
pthread_mutex_t logger_lock = PTHREAD_MUTEX_INITIALIZER;

bool init_logger(int row, int column, int message_amount, int message_size) {
    logger.pos_x = column;
    logger.pos_y = row;
    logger.message_amount = message_amount;
    logger.message_size = message_size;

    logger.messages = (char**)calloc(logger.message_amount, sizeof(char*));
    if (!logger.messages) return false;

    for (int i = 0; i < logger.message_amount; i++) {
        *(logger.messages + i) = (char*)calloc(logger.message_size, sizeof(char));
        
        if (!*(logger.messages + i)) {
            for (int j = 0; j < i; j++) {
                free(*(logger.messages + i));
            }
            free(logger.messages);
            return false;
        }
    }

    return true;
}


void log_message(const char * const message) {
    time_t rawtime;
    char current_time[10] = {"\0"};

    pthread_mutex_lock(&logger_lock);
    time (&rawtime);
    strftime(current_time, 10,"%X> ", localtime(&rawtime));

    for (int i = logger.message_amount - 1; i > 0; i--) {
        strcpy(logger.messages[i], logger.messages[i - 1]);
    }

    strncat(strcpy(logger.messages[0], current_time), message, logger.message_size - 11);
    
    /*LINE BELOW:
        Deceiving ncurses line is longer to avoid clearing screen.
        Clearing screen causes mighty screen flashing.
    */
    strncat(logger.messages[0], "                              ", logger.message_size - 1 - strlen(logger.messages[0]));

    print_coloured("MESSAGES", TEXT_COLOUR, logger.pos_y, logger.pos_x);
    for (int i = 0; i < logger.message_amount; i++) {
        print_coloured(logger.messages[i], TEXT_COLOUR, logger.pos_y + i + 1, logger.pos_x);
    }
    pthread_mutex_unlock(&logger_lock);
}


void destroy_logger() {

    for (int i = 0; i < logger.message_amount; i++) {
        free(*(logger.messages + i));
    }
    free(logger.messages);
}
