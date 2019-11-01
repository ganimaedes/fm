#include "data.h"
#include "parcours.h"
#include <signal.h>

#ifndef __STDC_ISO_10646__
    #error "Wide chars are not defined as Unicode codepoints"
#endif

#define pg_up "pg_up"
#define pg_dn "pg_dn"

int maxlen = 40;
int debug = 1;
volatile sig_atomic_t outside_box = 0;
volatile sig_atomic_t resized = 0;
volatile sig_atomic_t reprint = 0;

char position[PLACE_SZ];
char del_in[IN_SZ];

typedef struct {
    int y_beg;
    int x_beg;
    int y_size;
    int x_size;
    int y_previous;
    int x_previous;
} Window;

typedef struct {
    int array_size;
    int n_to_print;
    int n_lower_t;
    int pos_upper_t;
    int pos_lower_t;
    int option_previous;
} Scroll;

typedef struct {
    int y;
    int x;
} Box;

Window w_main, w1, w2;
Box box;

void indicators(Window *w, int y, int x, char pos_c[], char in[], char *msg);
void erase_window(Window *w, Scroll *s);
void copy_array(Array *array, char **entry, char **types, int size);
void highlight2(Array *a, int *pos);
int update(Window *w, Scroll *s, int *pos, int size);
void print_debug(Window *w, Scroll *s, int option, int pos, int cursor_pos);
void print_entries(Window *w, Scroll *s, char **entries, int option, int *c, int *pos, Array *a);
static void sig_win_ch_handler(int sig);
void mvwprintw(Window *win, int y, int x, char *str);
void draw_box(Window *w);
void set_box_size(int y, int x);

int main(int argc, char **argv)
{
    if (argc < 2) { perror("Missing Arguments\n"); return EXIT_FAILURE; } 
    setlocale(LC_ALL, "");
    save_config;
    w1.y_beg = 5;
    w1.x_beg = 5;

    char position[strlen(place)];
    if (get_window_size(STDIN_FILENO, STDOUT_FILENO, &w_main.y_size, &w_main.x_size) < 0) {
        restore_config;
        char *error = "Error getting window size";
        sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
        move(1, position);
        write(1, error, strlen(error));
        return EXIT_FAILURE;
    }

    int pos = 0;
    w1.y_size = w_main.y_size - w1.y_beg;
    w1.x_size = (w_main.x_size / 2) - w1.x_beg;
    w1.y_previous = 0;
    w1.x_previous = 0;

    w2.y_beg = 5;
    w2.y_size = w_main.y_size - w1.y_beg;
    w2.x_beg = w1.x_size + 1;
    w2.x_size = (w_main.x_size - w1.x_size);

    Scroll s;
    s.option_previous = 0;

    struct winsize w_s;
    struct sigaction s_a;
    sigemptyset(&s_a.sa_mask);
    s_a.sa_flags = 0;
    s_a.sa_handler = sig_win_ch_handler;
    char err1[] = "sigaction";
    if (sigaction(SIGWINCH, &s_a, NULL)) {
        restore_config;
        write(STDERR_FILENO, err1, sizeof(err1));
    }

    int c = 0,
        option = 0,
        i,
        initial_loop = 1,
        secondary_loop = 0;

    Array left_box;
    Array right_box;
    init(&left_box, 1);
    init(&right_box, 1);
    //copy_array(&left_box, entries, entries_types, sz);
    //copy_array(&right_box, s_entries, s_entries_types, s_sz);

    parcours(argv[1], 0, &left_box, 0);

    Array ps_left_box;
    Array ps_right_box;
    init(&ps_left_box, 1);
    init(&ps_right_box, 1);
    //copy_array(&ps_left_box, entries, entries_types, sz);
    //copy_array(&ps_right_box, s_entries, s_entries_types, s_sz);
    
    parcours(argv[1], 0, &ps_left_box, 0);

    sprintf(del_in, del, maxlen);

    int left_allocation = 1;
    int previous_pos = pos;

    for (;;) {
        
        reprint = 0;
        if (secondary_loop && left_box.n_elements != 0 ) { // && left_allocation
            sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
            move(1, position);
            highlight2(&left_box, &pos);
            for (i = 1; i < s.n_to_print; ++i) {
                mvwprintw(&w1, i + w1.y_beg + 1, w1.x_beg + 1, left_box.menu[i].name);
            }
           secondary_loop = 0;
        }
        
        if (!ioctl(STDIN_FILENO, TIOCGWINSZ, &w_s)) {
            w_main.y_size = w_s.ws_row;
            w_main.x_size = w_s.ws_col;

            w1.y_size = w_main.y_size - w1.y_beg;
            w1.x_size = (w_main.x_size / 2) - 1;
            maxlen = (w1.x_size) - w1.x_beg - 3;
            w2.y_size = w_main.y_size - w1.y_beg;
            w2.x_beg = w1.x_size;
            w2.x_size = w_main.x_size - 10;
            w2.x_size = w_s.ws_col + w1.x_beg - 11;

            if (w1.y_size > w1.y_previous || w1.x_size > w1.x_previous ||
                w1.y_size < w1.y_previous || w1.x_size < w1.x_previous) {
                erase_scr(STDOUT_FILENO, "\033[2J");

                draw_box(&w1);
                draw_box(&w2);
                option = update(&w1, &s, &pos, left_box.n_elements);
                
                if (initial_loop) {
                    sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
                    move(1, position);
                    highlight2(&left_box, &pos);
                    for (i = 1; i < s.n_to_print; ++i) {
                        mvwprintw(&w1, i + w1.y_beg + 1, w1.x_beg + 1, left_box.menu[i].name);
                    }
                   initial_loop = 0;
                }
            }
            w1.y_previous = w1.y_size;
            w1.x_previous = w1.x_size;
        }
        
        if (left_box.n_elements != 0) {
             if (pos > left_box.n_elements - 1) { 
                 if (previous_pos < left_box.n_elements - 1 && left_allocation != 0) {
                    pos = previous_pos;
                 } else {
                    pos = left_box.n_elements - 1;
                 }
                 char pos_of_pos[10];
                 sprintf(pos_of_pos, "pos: %d", pos);
                 sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
                 move(1, position);
                 write(1, pos_of_pos, strlen(pos_of_pos));
                 //sleep(5);
             }

                if (left_allocation != 0) {
                    //reprint = 1;
                    previous_pos = pos;
                } else {
                    reprint = 1;
                    // rentrer le chemin d access dans update 
                    // find parent
                    //
                }
                print_entries(&w1, &s, entries, option, &c, &pos, &left_box);
                if (!strcmp(left_box.menu[pos].type, "directory") && (c != 'l' && c != KEY_ENTER)) {
                    parcours(left_box.menu[pos].complete_path, 0, &right_box, 0);
                    for (i = 0; i < right_box.n_elements; ++i) { // s.n_to_print
                        mvwprintw(&w2, i + w2.y_beg + 1, w2.x_beg + 1, right_box.menu[i].name);
                    }
                    sprintf(position, place, pos - s.pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
                    move(1, position);
                }
        }
        if (debug) {
            int p = pos - s.pos_upper_t + w1.y_beg + 1;
            print_debug(&w_main, &s, option, pos, p);
        }
        
        if ((c = kbget()) == KEY_ESCAPE) { 
            break; 
        } else if ((c == 'l' || c == KEY_ENTER) && !strcmp(left_box.menu[pos].type, "directory")) {
            erase_window(&w1, &s);
            if (left_box.n_elements != 0) {
                free_array(&left_box);
                left_box.n_elements = 0; 
            }

            erase_window(&w2, &s);
            if (right_box.n_elements != 0) {
                free_array(&right_box);
                right_box.n_elements = 0;
            }

            init(&left_box, 1);
            init(&right_box, 1);
            copy_array(&left_box, s_entries, s_entries_types, s_sz);
            copy_array(&right_box, t_entries, t_entries_types, t_sz);
            pos = 0;
            option = update(&w1, &s, &pos, left_box.n_elements);
            secondary_loop = 1;
        } else if (c == 'h') { // && parent.type == "directory"
            erase_window(&w1, &s);
            if (left_box.n_elements != 0) {
                free_array(&left_box);
            }

            erase_window(&w2, &s);
            if (right_box.n_elements != 0) {
                free_array(&right_box);
            }
            init(&left_box, 1);
            init(&right_box, 1);
            if (ps_left_box.n_elements != 0) {
                dup_array(&ps_left_box, &left_box);
            } else {
                restore_config;
                free_array(&left_box);
                free_array(&ps_left_box);
                free_array(&right_box);
                fprintf(stderr, "Error: ps_left_box size: %d\n", ps_left_box.n_elements);
                exit(1);
            }
            if (ps_right_box.n_elements != 0) {
                dup_array(&ps_right_box, &right_box);
            } else {
                restore_config;
                free_array(&right_box);
                free_array(&ps_right_box);
                free_array(&left_box);
                free_array(&ps_left_box);
                fprintf(stderr, "Error: ps_right_box size: %d\n", ps_right_box.n_elements);
                exit(1);
            }

            option = update(&w1, &s, &pos, left_box.n_elements);
            reprint = 0;
            left_allocation = 0; 
        }
        erase_window(&w2, &s);

    }
    if (left_box.n_elements != 0) {
        free_array(&left_box);
    }
    if (right_box.n_elements != 0 || right_box.capacity != 0) {
        free_array(&right_box);
    }
    if (ps_left_box.n_elements != 0) {
        free_array(&ps_left_box);
    }
    if (ps_right_box.n_elements != 0 || ps_right_box.capacity != 0) {
        free_array(&ps_right_box);
    }
    restore_config;
    return 0;
}

void indicators(Window *w, int y, int x, char pos_c[], char in[], char *msg)
{
    sprintf(pos_c, place, y, x);
    move(1, pos_c);
    del_from_cursor(in);
    mvwprintw(w, y, x, msg);
}

void erase_window(Window *w, Scroll *s)
{
    int i;
    char pos[strlen(place)];
    char in[strlen(del)];
    //int erase_width = maxlen - 5;
    //sprintf(in, del, maxlen + 1);
    sprintf(in, del, maxlen);

    //sprintf(in, del, erase_width);
    for (i = 0; i < s->n_to_print; ++i) {
        sprintf(pos, place, i + w->y_beg + 1, w->x_beg + 1);
        move(1, pos);
        del_from_cursor(in);
    }
}

void copy_array(Array *array, char **entry, char **types, int size)
{
    Menu menu;
    int i;
    int len = 0;
    for (i = 0; i < size; ++i) {
        len = strlen(entry[i]);
        menu.name = (char *)malloc(sizeof(char) * (len + 1));
        if (menu.name) {
            strcpy(menu.name, entry[i]);
            menu.name[len] = '\0';
        }
        len = strlen(types[i]);
        menu.type = (char *)malloc(sizeof *(menu.type) * (len + 1));
        if (menu.type) {
            strcpy(menu.type, types[i]);
            menu.type[len] = '\0';
        }
        add_menu(array, menu);
        free(menu.name);
        free(menu.type);
    }
}

void highlight2(Array *a, int *pos)
{
    int len = strlen(a->menu[*pos].name);
    char space[1] = " ";
    write(1, bg_cyan, sizeof(bg_cyan));
    write(1, a->menu[*pos].name, len);
    int i;
    for (i = 0; i < maxlen - len; ++i) {
        write(1, space, 1);
    }
    write(1, bg_reset, sizeof(bg_reset));
}

static void sig_win_ch_handler(int sig) { resized = 1;}

int update(Window *w, Scroll *s, int *pos, int size)
{
    int option = 0;
    s->array_size = size;
    int y = w->y_size - w->y_beg - 2;
    if (*pos == 0 ) {
        s->pos_upper_t = 0;
    }
    if (s->array_size <= y) {
        s->n_to_print = s->array_size;
        s->pos_lower_t = s->n_to_print - 1;
        s->n_lower_t = 0;
        if (s->option_previous == 3) { 
            s->pos_upper_t = 0;
        }
        if (s->array_size == y) {
            option = 1;
        } else if (s->array_size < y) {
            option = 2;
        }
    } else if (s->array_size > y) {
        s->n_to_print = y;
        if (s->n_to_print < s->pos_upper_t || s->pos_upper_t != 0) {
            s->pos_lower_t = s->n_to_print + s->pos_upper_t - 1;
        } else {
            s->pos_lower_t = s->n_to_print - s->pos_upper_t - 1;
        }
        if (s->pos_lower_t < 0) { s->pos_lower_t = s->array_size - 1; }
        s->n_lower_t = s->array_size - s->pos_lower_t - 1;

        if (s->n_to_print + s->pos_upper_t >= s->array_size) {
            s->n_to_print = s->array_size - s->pos_upper_t;
            s->pos_lower_t = s->array_size - 1;
            s->n_lower_t = s->array_size - s->pos_lower_t - 1;
        }
        if (s->n_to_print != w->y_size - w->y_beg - 2) {
            s->n_to_print = w->y_size - w->y_beg - 2;
            if (s->n_to_print > s->array_size - 1) {
                s->n_to_print = s->array_size - 1;
            }
            s->pos_upper_t = s->pos_lower_t - s->n_to_print;
            s->pos_lower_t = s->pos_upper_t + s->n_to_print - 1;
            s->n_lower_t = s->array_size - s->pos_lower_t - 1;
        }
        if (*pos == s->array_size - 1 && resized) {
            *pos = s->pos_lower_t;
            if (s->pos_lower_t == s->array_size - 1) {
                *pos = s->array_size - 1;
                //s->pos_upper_t = s->pos_lower_t - s->n_to_print ;
                if (debug) {
                    indicators(&w_main, w_main.y_size - 2, w_main.x_beg + 2, position, del_in, "lower == size");
                }
            }
            if (s->pos_lower_t != s->array_size - 1) {
                ++s->pos_upper_t;
                ++s->pos_lower_t;
                ++*pos;
            }
        }
        if (reprint) {
            //++s->pos_upper_t;
            //++s->pos_lower_t;
            //++*pos;
        }
        option = 3;
    }
    s->option_previous = option;
    return option;
}

void print_entries(Window *w, Scroll *s, __attribute__((__unused__)) char **entries, int option, int *c, int *pos, Array *a)
{
    int i;
    int y = 0;
    char pos_c[strlen(place)];
    char in[strlen(del)];
    sprintf(in, del, maxlen);

    if (reprint) {
        if (*pos + 1 < s->array_size - 1) {
            //++*pos;
            //++s->pos_upper_t;
            //++s->pos_lower_t;
            //++*pos;
        }
            //++s->pos_upper_t;
            //++s->pos_lower_t;
    }
    if (s->option_previous != option || resized || reprint) {
        //int u_value = s->pos_upper_t;
        //int l_value = s->pos_lower_t;
        int diff = s->pos_lower_t - s->pos_upper_t + 1;
        int j = s->pos_upper_t;
        if (reprint) {
            //u_value = s->pos_upper_t + 1;
            //l_value = s->pos_lower_t + 1;
            //diff = l_value - u_value + 1;
            //j = u_value;
            //++s->pos_upper_t;
            //++s->pos_lower_t;
        }
        for (i = 0; i < diff; ++i, ++j) {
            sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
            move(1, pos_c);
            if (j == *pos) {
                sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                //sprintf(pos_c, place, *pos - u_value + w->y_beg + 1, w->x_beg + 1);
                move(1, pos_c);
                del_from_cursor(in);
                highlight2(a, pos);
            } else {
                write(1, a->menu[j].name, strlen(a->menu[j].name));
            }
            if (reprint) {
                //--s->pos_upper_t;
                //--s->pos_lower_t;
            }
        }
    }

    switch (*c) {
        case KEY_UP:
        case UP:
            if (*pos > 0) {
                --*pos;
                resized = 0;
                if (*pos >= s->pos_upper_t && *pos < s->pos_lower_t) {
                    y = *pos - s->pos_upper_t + w->y_beg + 1;
                    sprintf(pos_c, place, y + 1, w->x_beg + 1);
                    move(1, pos_c);
                    del_from_cursor(in);
                    if (*pos + 1 < s->array_size) {
                        write(1, a->menu[*pos + 1].name, strlen(a->menu[*pos + 1].name));
                    }
                    sprintf(pos_c, place, y, w->x_beg + 1);
                    move(1, pos_c);
                } else if (*pos <= s->pos_upper_t && s->pos_upper_t > 0) {
                    --s->pos_upper_t;
                    ++s->n_lower_t;
                    --s->pos_lower_t;
                    for (i = 0; i < s->n_to_print; ++i) {
                        sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);
                        if (i + w->y_beg < box.y - 3) {
                            sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);
                        }
                        if (s->pos_upper_t + i < s->array_size + w->y_beg) {
                            write(1, a->menu[s->pos_upper_t + i].name, strlen(a->menu[s->pos_upper_t + i].name));
                        }
                    }
                    if (*pos - s->pos_upper_t + w->y_beg + 1 < w->y_size - 1) {
                        outside_box = 0;
                        //del_from_cursor(in);
                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                    } else {
                        outside_box = 1;
                        sprintf(pos_c, place, i + w->y_beg, w->x_beg + 1);
                        move(1, pos_c);
                    }
                }
                if (*pos >= s->pos_lower_t) {
                    *pos = s->pos_lower_t;
                    if (*pos < s->array_size - 1) {
                        sprintf(pos_c, place, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);
                        sprintf(pos_c, place, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
                        move(1, pos_c);
                    }
                }

                if (*pos < s->pos_lower_t + 1) {
                    //sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    //del_from_cursor(in);
                    highlight2(a, pos);
                }
            }
            break;
        case KEY_DOWN:
        case DN:
            if (*pos < s->array_size - 1) {
                ++*pos;
                resized = 0;
                if (*pos > s->pos_upper_t && *pos <= s->pos_lower_t) {
                    y = *pos - s->pos_upper_t + w->y_beg;
                    sprintf(pos_c, place, y, w->x_beg + 1);
                    move(1, pos_c);
                    del_from_cursor(in);
                    if (*pos - 1 < s->array_size) {
                        write(1, a->menu[*pos - 1].name, strlen(a->menu[*pos - 1].name));
                    }
                    sprintf(pos_c, place, y + 1, w->x_beg + 1);
                    move(1, pos_c);
                    outside_box = 0;
                } else if (*pos - 1 >= s->pos_lower_t) {
                    ++s->pos_upper_t;
                    --s->n_lower_t;
                    ++s->pos_lower_t;

                    for (i = 0; i < s->n_to_print; ++i) {

                        if (i + w->y_beg + 1 < w->y_size) {
                            sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);
                        }
                        del_from_cursor(in);

                        if (s->pos_upper_t + i < s->array_size) {
                            write(1, a->menu[s->pos_upper_t + i].name, strlen(a->menu[s->pos_upper_t + i].name));
                        }
                    }

                    if (*pos - s->pos_upper_t + w->y_beg + 1 < w->y_size - 1) {
                        outside_box = 0;
                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);
                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                    } else {
                        outside_box = 1;
                        sprintf(pos_c, place, i + w->y_beg, w->x_beg + 1);
                        move(1, pos_c);
                    }
                }
                if (outside_box) {
                    *pos = s->pos_lower_t;
                }
                highlight2(a, pos);
            }
            break;
        case KEY_PAGE_UP:
                //char pg_up[] = "pg_up";

                if (*pos - s->n_to_print + 1 >= 0) {

                    if (*pos - s->n_to_print + 1 >= s->pos_upper_t) {

                        if (debug) {
                            sprintf(pos_c, place, w->y_size + 2, w->x_beg + 2);
                            move(1, pos_c);
                            write(1, pg_up, strlen(pg_up));
                            indicators(w, w->y_size + 5, w->x_beg + 1, pos_c, in, "1aup");
                        }

                        if (*pos - s->pos_upper_t + w->y_beg + 1 < w->y_size - 1) {
                            if (s->n_to_print > 1) {
                                sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                                move(1, pos_c);
                                del_from_cursor(in);
                                sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                                move(1, pos_c);
                                write(1, a->menu[*pos].name, strlen(a->menu[*pos].name));
                            }
                            *pos -= (s->n_to_print - 1); // donne *pos = *pos - s->n_to_print + 1
                            sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);

                        } else {
                            *pos = s->pos_lower_t;
                            sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);
                        }
                        highlight2(a, pos);

                    } else {

                        if (debug) {
                            indicators(w, w->y_size + 5, w->x_beg + 1, pos_c, in, "1bup");
                        }

                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);

                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        write(1, a->menu[*pos].name, strlen(a->menu[*pos].name));
                        if (*pos - s->n_to_print + 1 < s->pos_upper_t) {
                            *pos -= (s->n_to_print - 1);
                            s->pos_upper_t = *pos;
                            s->pos_lower_t = s->pos_upper_t + s->n_to_print - 1;
                            s->n_lower_t = s->array_size - s->pos_lower_t - 1;

                        }
                        if (*pos > s->pos_lower_t) {
                            *pos = s->pos_lower_t;
                        }
                        for (i = 0; i < s->n_to_print; ++i) {

                            sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);
                            del_from_cursor(in);

                            if (*pos + i < s->array_size - 1) {
                                write(1, a->menu[*pos + i].name, strlen(a->menu[*pos + i].name));
                            }
                        }

                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);

                        highlight2(a, pos);
                    }
                } else if (*pos - s->n_to_print < 0) {
                    if (debug) {
                        indicators(w, w->y_size + 3, w->x_beg + 1, pos_c, in, "2up");
                    }

                    *pos = 0;

                    //sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    //move(1, pos_c);
                    for (i = 0; i < s->n_to_print; ++i) {

                        sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);

                        if (*pos + i < s->array_size) {
                            write(1, a->menu[*pos + i].name, strlen(a->menu[*pos + i].name));
                        }
                    }

                    s->pos_lower_t -= s->pos_upper_t;
                    s->pos_upper_t = *pos;
                    s->n_lower_t = s->array_size - s->n_to_print;
                    sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, pos_c);

                    highlight2(a, pos);
                }
            break;
        case KEY_PAGE_DN:
                //char pg_dn[] = "pg_dn";

                if (*pos + s->n_to_print - 1 <= s->array_size - 1) {
                    if (debug) {
                        sprintf(pos_c, place, w->y_size + 2, w->x_beg + 2);
                        move(1, pos_c);
                        write(1, pg_dn, strlen(pg_dn));
                        indicators(w, w->y_size + 3, w->x_beg + 2, pos_c, in, "1");
                    }

                    if (*pos + s->n_to_print - 1 <= s->pos_lower_t) { // no scroll

                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);

                        sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        write(1, a->menu[*pos].name, strlen(a->menu[*pos].name));

                        *pos += (s->n_to_print - 1);
                    } else {
                        if (debug) {
                            indicators(w, w->y_size + 3, w->x_beg + 1, pos_c, in, "1a");
                        }
                        s->pos_upper_t = *pos;
                        s->pos_lower_t = *pos + s->n_to_print - 1;
                        s->n_lower_t = s->array_size - s->pos_lower_t - 1;
                        if (s->n_lower_t < 0) {
                            s->n_lower_t = 0;
                        }
                        // va ds 2 ne s'applique pas ici
                        //if (s->pos_lower_t > s->array_size - 1) {
                        //    s->pos_lower_t = s->array_size - 1;
                        //}
                        for (i = 0; i < s->n_to_print; ++i) {
                            sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                            move(1, pos_c);
                            del_from_cursor(in);

                            if (*pos + i < s->array_size) {
                                write(1, a->menu[*pos + i].name, strlen(a->menu[*pos + i].name));
                                //write(1, "a", strlen("a"));
                            }
                        }
                        *pos += (s->n_to_print - 1);
                    }
                    sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, pos_c);
                    highlight2(a, pos);

                } else if (*pos + s->n_to_print - 1 > s->array_size - 1) {
                    if (debug) {
                        indicators(w, w->y_size + 3, w->x_beg + 1, pos_c, in, "2  ");
                    }

                    for (i = 0; i < s->n_to_print; ++i) {
                        sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, pos_c);
                        del_from_cursor(in);

                        write(1, a->menu[s->pos_upper_t + s->n_lower_t + i].name,
                                strlen(a->menu[s->pos_upper_t + s->n_lower_t + i].name));
                    }

                    int prev_i = i;
                    s->pos_upper_t += s->n_lower_t;
                    s->pos_lower_t = s->array_size - 1;
                    s->n_lower_t = 0;

                    *pos = s->array_size - 1;

                    sprintf(pos_c, place, prev_i + w->y_beg, w->x_beg + 1);
                    move(1, pos_c);
                    highlight2(a, pos);
                } else if (*pos + 1 < s->array_size - 1) {
                    del_from_cursor(in);
                    mvwprintw(w, w->y_size + 3, w->x_beg + 2, "3");
                }
            break;
        case KEY_HOME:
            *pos = 0;
            s->pos_upper_t = 0;
            s->pos_lower_t = s->n_to_print - 1;
            s->n_lower_t = s->array_size - s->n_to_print;
            for (i = 0; i < s->n_to_print; ++i) {
                sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                move(1, pos_c);
                del_from_cursor(in);
                write(1, a->menu[i].name, strlen(a->menu[i].name));
            }
            sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, pos_c);
            highlight2(a, pos);
            break;
        case KEY_END:
            s->pos_upper_t = s->array_size - s->n_to_print;
            *pos = s->array_size - 1;
            s->pos_lower_t = *pos;
            s->n_lower_t = 0;
            for (i = 0; i < s->n_to_print; ++i) {
                sprintf(pos_c, place, i + w->y_beg + 1, w->x_beg + 1);
                move(1, pos_c);
                del_from_cursor(in);
                write(1, a->menu[i + s->pos_upper_t].name, strlen(a->menu[i + s->pos_upper_t].name));
            }
            sprintf(pos_c, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, pos_c);
            highlight2(a, pos);
            break;
        default:
            break;
    }

    if (*pos == 0) {
        sprintf(pos_c, place, w->y_beg + 1, w->x_beg + 1);
        move(1, pos_c);
    }

    snprintf(pos_c, strlen(place), place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
    move(1, pos_c);
}

void draw_box(Window *w)
{
    int horiz = w->y_size - w->y_beg;
    int vert = w->x_size - w->x_beg;
    set_box_size(horiz, vert);
    int i, j;
    BOX_CONTOUR(line, v_line, 
            lu_corner, ll_corner, ru_corner, rl_corner, 
            heavy_line, heavy_v_line,
            lu_heavy_corner, ll_heavy_corner, ru_heavy_corner, rl_heavy_corner);
    if (box_color && box_thickness) {
        write(1, fg_cyan, strlen(fg_cyan));
    } 
    sprintf(position, place, w->y_beg, w->x_beg - 1);
    move(1, position);
    write(1, ARRAY[cont_2], strlen(ARRAY[cont_2]));
    for (j = 0; j < vert - 2; ++j) {                            // maxlen + 2
        write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
    }
    write(1, ARRAY[cont_4], strlen(ARRAY[cont_4]));
    for (i = 0; i < horiz - 1; ++i) {                           // to_print + 1
        sprintf(position, place, i + w->y_beg + 1, w->x_beg - 1);
        move(1, position);
        write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
        sprintf(position, place, i + w->y_beg + 1, w->x_beg + vert - 2);
        move(1, position);
        write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
    }
    sprintf(position, place, i + w->y_beg, w->x_beg - 1);
    move(1, position);
    write(1, ARRAY[cont_3], strlen(ARRAY[cont_3]));
    for (j = 0; j < vert - 1; ++j) {
        sprintf(position, place, i + w->y_beg, w->x_beg + j);
        move(1, position);
        write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
    }
    sprintf(position, place, i + w->y_beg, w->x_beg + j - 1);
    move(1, position);
    write(1, ARRAY[cont_5], strlen(ARRAY[cont_5]));
    write(1, fg_reset, strlen(fg_reset));
}

void set_box_size(int y, int x)
{
    box.y = y;
    box.x = x;
}

void mvwprintw(Window *win, int y, int x, char *str)
{
    if (win->y_size > w_main.y_size) {
        char *err = "Error: out of window size: ";
        write(2, err, strlen(err));
        write(2, __func__, strlen(__func__));
    }
    char pos[strlen(place)];
    sprintf(pos, place, y, x);
    move(1, pos);

    sprintf(del_in, del, win->x_size - win->x_beg - 5);
    del_from_cursor(del_in);
    move(1, pos);
    write(1, str, strlen(str));
}

#define snprint   "s->n_to_print: %d"
#define sposupper "s->pos_upper_t: %d"

#define snlower   "s->n_lower_t: %d"

void print_debug(Window *w, Scroll *s, int option, int pos, int cursor_pos)
{
    char x_size[sizeof("%d")];
    //char y_size[3];
    char y_size[sizeof("%d")];
    //char pos_place[strlen(place)];
    char pos_place[sizeof(place) + 1];

    // size of w1 and not of w !
    sprintf(pos_place, place, 1, w->x_size - 3);
    move(1, pos_place);
    sprintf(y_size, "%d", w->y_size);
    write(1, y_size, strlen(y_size));
    //mvwprintw(w, 1, w->x_size - 3, y_size);

    sprintf(pos_place, place, 2, w->x_size - 3);
    move(1, pos_place);
    sprintf(x_size, "%d", w->x_size);
    write(1, x_size, strlen(x_size));
    //mvwprintw(w, 2, w->x_size - 3, x_size);

    //char num[21];
    //char num[sizeof("s->n_to_print: %d") + 1];
    //char snprint[] = "s->n_to_print";
    char num[strlen(snprint)];

    
    int y_top = w->y_beg + 3;
    int x_top = w->x_beg + 3;
    sprintf(pos_place, place, y_top, x_top);
    move(1, pos_place);
    //sprintf(num, "s->n_to_print: %d", s->n_to_print);
    sprintf(num, snprint, s->n_to_print);
    write(1, num, strlen(num));
    //write(1, num, sizeof(num));
   

    char num_pos[strlen(sposupper)];

    sprintf(pos_place, place, w->y_beg + 1, (w->x_size / 2) + 5);
    move(1, pos_place);
    //sprintf(num, "s->pos_upper_t: %d", s->pos_upper_t);
    sprintf(num_pos, sposupper, s->pos_upper_t);
    del_from_cursor(del_in);
    sprintf(pos_place, place, w->y_beg + 1, (w->x_size / 2) + 5);
    move(1, pos_place);
    write(1, num_pos, strlen(num_pos));

    //char in[strlen(del)];
    //sprintf(in, del, maxlen);
    //del_from_cursor(in);
    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 5);
    move(1, pos_place);
    //sprintf(num, "s->pos_lower_t: %d", s->pos_lower_t);

    del_from_cursor(del_in);
    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 5);
    move(1, pos_place);

    sprintf(num_pos, "s->pos_lower_t: %d", s->pos_lower_t);
    write(1, num_pos, strlen(num_pos));

    char num_lower[strlen(snlower)];
    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 25);
    move(1, pos_place);
    //sprintf(num, "s->n_lower_t: %d", s->n_lower_t);
    //write(1, num, strlen(num));
    sprintf(num_lower, snlower, s->n_lower_t);
    write(1, num_lower, strlen(num_lower));

    char opt[10];
    sprintf(opt, "option: %d", option);
    sprintf(pos_place, place, w->y_size - 3, (w->x_size / 2) + 10);
    move(1, pos_place);
    write(1, opt, strlen(opt));


    sprintf(pos_place, place, w->y_size - 4, (w->x_size / 2) + 10);
    move(1, pos_place);
    del_from_cursor(del_in);
    sprintf(pos_place, place, w->y_size - 4, (w->x_size / 2) + 10);
    move(1, pos_place);
    sprintf(opt, "pos:    %d", pos);
    sprintf(pos_place, place, w->y_size - 4, (w->x_size / 2) + 10);
    move(1, pos_place);
    write(1, opt, strlen(opt));

    char pos_c[strlen(place)];
    sprintf(pos_c, place, w->y_size + 1, (w->x_size / 2) + 8);
    move(1, pos_c);
    sprintf(num, "box_size_y: %d", box.y);
    write(1, num, strlen(num));

    char y_value[12];
    char x_value[12];
    sprintf(pos_c, place, w->y_size + 4, w->x_size - 20);
    move(1, pos_c);
    char de[sizeof(del)];
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 4, w->x_size - 20);
    move(1, pos_c);
    sprintf(y_value, "y_value: %d", pos - s->pos_upper_t + w->y_beg + 1);
    write(1, y_value, strlen(y_value));

    sprintf(pos_c, place, w->y_size + 3, w->x_size - 20);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 3, w->x_size - 20);
    move(1, pos_c);
    sprintf(x_value, "x_value: %d", w->x_beg + 1);
    write(1, x_value, strlen(x_value));

    char b_value[12];
    sprintf(pos_c, place, w->y_size + 2, w->x_size - 20);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 2, w->x_size - 20);
    move(1, pos_c);
    sprintf(b_value, "b_value: %d", box.y + w->y_beg - 2);
    write(1, b_value, strlen(b_value));

    char w_cur[11];
    sprintf(pos_c, place, w->y_size + 3, w->x_size - 40);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 3, w->x_size - 40);
    move(1, pos_c);
    sprintf(w_cur, "w_cur: %d", w->y_size);
    write(1, w_cur, strlen(w_cur));

    sprintf(pos_c, place, w->y_size + 4, w->x_size - 40);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 4, w->x_size - 40);
    move(1, pos_c);
    sprintf(w_cur, "resized: %d", resized);
    write(1, w_cur, strlen(w_cur));

    sprintf(pos_c, place, w->y_size + 4, w->x_size - 60);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size + 4, w->x_size - 60);
    move(1, pos_c);
    sprintf(w_cur, "outside: %d", outside_box);
    write(1, w_cur, strlen(w_cur));
    // replace le curseur a la 1ere lettre de la dern entree
    sprintf(pos_c, place, cursor_pos, w1.x_beg + 1);
    move(1, pos_c);
}
