#include "labyrinth-loader.h"


static long file_size(FILE * const file) {

    if (!file) {
        errno = EINVAL;
        return -1;
    }

    if (-1 == fseek(file, 0L, SEEK_END)) return -1;
    long f_len = ftell(file);
    rewind(file);
    return f_len;
}


static int line_len(FILE * const file) {
    if (!file) {
        errno = EINVAL;
        return -1;
    }

    int marker_counter = 0, marker = fgetc(file);
    while (marker != '\n') {
        if (marker == EOF) return EOF;
        ++marker_counter;
        marker = fgetc(file);
    }
    rewind(file);
    return marker_counter;
}


static labyrinth_t* allocate_lbrth_mem(FILE * const file) {
    labyrinth_t *lbrth = (labyrinth_t*)calloc(1, sizeof(labyrinth_t));
    if (!lbrth) {
        printf("%s\n", strerror(errno));
        return NULL;
    }

    lbrth->height = file_size(file) / line_len(file);
    lbrth->width = line_len(file);

    lbrth->lbrth = (field_t**)calloc(sizeof(field_t*), lbrth->height + 1);
    if (!lbrth->lbrth) {
        printf("%s\n", strerror(errno));
        return NULL;
    }

    for (size_t i = 0; i < lbrth->height; ++i) {
        *(lbrth->lbrth + i) = (field_t*)calloc(sizeof(field_t), lbrth->width + 1);
        if (!*(lbrth->lbrth + i)) {
            for (size_t j = 0; j < i; ++j) {
                free(*(lbrth->lbrth + j));
                free(lbrth->lbrth);
                return NULL;
            }
        }
    }
    
    return lbrth;
}


static bool parse_labyrinth(FILE * const file, labyrinth_t * const lbrth) {

    if (!file) {
        errno = EINVAL;
        printf("%s\n", strerror(errno));
        return false;
    }

    char *buffer = (char*)calloc(sizeof(char), lbrth->width + 1);
    if (!buffer) {
        printf("%s\n", strerror(errno));
        return false;
    }

    const int new_line = sizeof(char);
    for (size_t i = 0; i < lbrth->height; ++i) {
        if (fread(buffer, sizeof(char), lbrth->width + new_line, file) != lbrth->width + new_line) {
            perror("Wrong file format!\n");
            free(buffer);
            return false;
        }
        
        for (size_t j = 0; j < lbrth->width; ++j) {
            switch (*(buffer + j)) {
            
            case '#':
                (*(lbrth->lbrth + i) + j)->value = 0;
                (*(lbrth->lbrth + i) + j)->kind = WALL;
                break;
            case ' ':
                (*(lbrth->lbrth + i) + j)->value = 0;
                (*(lbrth->lbrth + i) + j)->kind = BLANK;
                break;
            case '_':
                (*(lbrth->lbrth + i) + j)->value = 0;
                (*(lbrth->lbrth + i) + j)->kind = BUSH;
                break;
            case 'c':
                (*(lbrth->lbrth + i) + j)->value = 1;
                (*(lbrth->lbrth + i) + j)->kind = COIN;
                break;
            case 't':
                (*(lbrth->lbrth + i) + j)->value = 10;
                (*(lbrth->lbrth + i) + j)->kind = TREASURE;
                break;
            case 'T':
                (*(lbrth->lbrth + i) + j)->value = 50;
                (*(lbrth->lbrth + i) + j)->kind = LARGE_TREASURE;
                break;
            case 'A':
                (*(lbrth->lbrth + i) + j)->value = 0;
                (*(lbrth->lbrth + i) + j)->kind = CAMPSITE;
                break;
            default:
                if ('\n' == *(buffer + j)) continue;
                printf("DEFAULT: '%c' ", *(buffer + j));
                free(buffer);
                return false;
            }
        }
    }

    free(buffer);
    return true;
}


labyrinth_t *get_labyrinth_from_file(const char * const filename) {

    if (!filename) return NULL;

    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("%s\n", strerror(errno));
        perror("Couldn't open file\n");
        return NULL;
    }

    int file_len = file_size(file);
    if (-1 == file_len) {
        printf("%s\n", strerror(errno));
        fclose(file);
        return NULL;
    }
    
    labyrinth_t *lbrth = allocate_lbrth_mem(file);
    if (!lbrth) {
        printf("%s\n", strerror(errno));
        return NULL;
    }

   if (!parse_labyrinth(file, lbrth)) {
       destroy_labyrinth(lbrth);
       fclose(file);
       return NULL;
   }
    
    fclose(file);
    return lbrth;
}


void destroy_labyrinth(labyrinth_t *lbrth) {
    
    if (!lbrth) {
        errno = EINVAL;
        printf("%s\n", strerror(errno));
        return;
    }

    for (int i = 0; *(lbrth->lbrth + i); ++i) {
        free(*(lbrth->lbrth + i));
    }

    free(lbrth->lbrth);
    free(lbrth);
}
