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
    init_pair(PLAYER_COLOUR, COLOR_BLUE, COLOR_MAGENTA);
    init_pair(TEXT_COLOUR, COLOR_WHITE, COLOR_BLACK);
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
