#define _XOPEN_SOURCE

#include "parcours.h"
#include "positions.h"
#include <execinfo.h>
#include <signal.h>

#ifndef __STDC_ISO_10646__
    #error "Wide chars are not defined as Unicode codepoints"
#endif

#define pg_up "pg_up"
#define pg_dn "pg_dn"

int maxlen = 40;
int debug = 0;
volatile sig_atomic_t outside_box = 0;
volatile sig_atomic_t resized = 0;
volatile sig_atomic_t reprint = 0;
volatile sig_atomic_t back_pressed = 0;
volatile sig_atomic_t enter_backspace = 1;

char position[PLACE_SZ];
char del_in[IN_SZ];

Window w_main, w1, w2;
Attributes attributes;

void handler(int sig);
void reprint_menu(Window *w, Scroll *s1, Array *a, Attributes *attr, int pos, int option);
void copy_scroll(Scroll *s_in, Scroll *s_out);
void indicators(Window *w, int y, int x, char pos_c[], char in[], char *msg);
void erase_window(Window *w, Scroll *s);
void copy_array(Array *array, char **entry, char **types, int size);
void highlight2(Array *a, int *pos);
int update(Window *w, Scroll *s, int *pos, int size);
void print_debug(Window *w, Scroll *s, int option, int pos, int cursor_pos, Array *a);
void print_entries(Window *w, Scroll *s, char **entries, int option, int *c, int *pos, Array *a);
static void sig_win_ch_handler(int sig);
void mvwprintw(Window *win, int y, int x, char *str);
void draw_box(Window *w);

#define J_NUM "J_SLASH: %d"
#define L_NUM "L_SLASH: %d"
#define CUR_DIR "cur_dir: "
int main(int argc, char **argv)
{
    char **entries = NULL;
    signal(SIGSEGV, handler);
    if (argc < 2) { fprintf(stderr, "Missing Arguments\n"); return EXIT_FAILURE; }
    setlocale(LC_ALL, "");
    save_config;

    char position[strlen(place)];
    if (get_window_size(0, 1, &w_main.y_size, &w_main.x_size) < 0) {
        restore_config;
        char *error = "Error getting window size";
        sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
        move(1, position);
        write(1, error, strlen(error));
        return EXIT_FAILURE;
    }

    int pos = 0;

    w_main.y_beg = 1;
    w_main.x_beg = 1;
    w_main.y_previous = 0;
    w_main.x_previous = 0;

    w1.y_beg = 3;
    w1.x_beg = 1;
    w1.y_previous = 0;
    w1.x_previous = 0;

    w2.y_beg = w1.y_beg;
    w2.x_beg = (w_main.x_size / 2);

    Scroll s;
    s.option_previous = 0;

    struct winsize w_s;
    struct sigaction sact;
    sigemptyset(&sact.sa_mask);
    sact.sa_flags = 0;
    sact.sa_handler = sig_win_ch_handler;
    char err1[] = "sigaction";
    if (sigaction(SIGWINCH, &sact, NULL)) {
        restore_config;
        write(2, err1, sizeof(err1));
    }

    int c = 0,
        option = 0,
        i,
        initial_loop = 1,
        secondary_loop = 0;
    int previous_pos = pos;
    int left_allocation = 1;
    int backspace = 0;

    Array left_box;
    Array right_box;
    init(&left_box, 5);
    init(&right_box, 5);

    Positions posit;
    posit.m_lower_pos = 0;
    posit.m_position = 0;
    posit.m_upper_pos = 0;

    attributes.paths = NULL;
    attributes.pos = NULL;
    attributes.counter = 0;
    attributes.n_elements = 0;
    attributes.capacity = 0;
    init_attr(&attributes, 2);

    parcours(argv[1], 0, &left_box, 0, &w_main);
    char *parent_up = NULL;

    for (;;) {
        if (!ioctl(0, TIOCGWINSZ, &w_s)) {
            w_main.y_size = w_s.ws_row;
            w_main.x_size = w_s.ws_col;

            w1.y_size = w_main.y_size - w1.y_beg - 5;
            w1.x_size = (w_main.x_size / 2) ;

            w2.y_beg = w1.y_beg;
            w2.x_beg = w1.x_size;
            w2.y_size = w1.y_size;
            w2.x_size = w1.x_size ;

            sprintf(del_in, del, w1.x_size - 2);

            if (w1.y_size > w1.y_previous || w1.x_size > w1.x_previous ||
                w1.y_size < w1.y_previous || w1.x_size < w1.x_previous) {
                erase_scr(1, "\033[2J");

                draw_box(&w1);
                draw_box(&w2);
                option = update(&w1, &s, &pos, left_box.n_elements);

                if (initial_loop) {
                    sprintf(position, place, w1.y_beg + 1, w1.x_beg + 1);
                    move(1, position);
                    highlight2(&left_box, &pos);
                    for (i = 1; i < s.n_to_print; ++i)
                        mvwprintw(&w1, i + w1.y_beg + 1, w1.x_beg + 1, left_box.menu[i].name);
                }
                resized = 1;
            }
            w1.y_previous = w1.y_size;
            w1.x_previous = w1.x_size;
        }

        if (left_box.n_elements != 0) {
             if (previous_pos < left_box.n_elements) {
                 if (backspace) {
                    pos = previous_pos;
                    backspace = 0;
                 }
             }

            if (!left_allocation || secondary_loop) {
                reprint = 1;
                secondary_loop = 0;
            }

            if (c == KEY_ENTER || c == KEY_BACKSPACE || resized) {
                reprint_menu(&w1, &s, &left_box, &attributes, pos, option);
            }
            print_entries(&w1, &s, entries, option, &c, &pos, &left_box);

            if (pos < left_box.n_elements && !strcmp(left_box.menu[pos].type, "directory")) {
                if (right_box.n_elements != 0) {
                    free_array(&right_box);
                    init(&right_box, 1);
                }
                parcours(left_box.menu[pos].complete_path, 0, &right_box, 0, &w_main);
                int to_print = 0;
                if (right_box.n_elements <= s.n_to_print) {
                    to_print = right_box.n_elements;
                } else {
                    if (right_box.n_elements >= w2.y_size - 1)
                        to_print = w2.y_size - 1;
                     else
                        to_print = right_box.n_elements;
                }
                for (i = 0; i < to_print; ++i) {
                    mvwprintw(&w2, i + w2.y_beg + 1, w2.x_beg + 1, right_box.menu[i].name);
                }
                sprintf(position, place, pos - s.pos_upper_t + w1.y_beg + 1, w1.x_beg + 1);
                move(1, position);
            }
        }

        if (enter_backspace == 1 && attributes.n_elements != 0 && back_pressed == 0 && initial_loop != 1) {
            free_attr(&attributes);
            init_attr(&attributes, 1);
            attributes.n_elements = 0;
        }

        if (debug) {
            int p = pos - s.pos_upper_t + w1.y_beg + 1;
            print_debug(&w_main, &s, option, pos, p, &left_box);
        }

        if ((c = kbget()) == KEY_ESCAPE) {
            break;
        } else if ((c == 'l' || c == KEY_ENTER) &&
                !strcmp(left_box.menu[pos].type, "directory") &&
                right_box.n_elements != 0) {

            previous_pos = pos;

            erase_window(&w1, &s);

            ++enter_backspace;

            posit.m_position = pos;
            posit.m_upper_pos = s.pos_upper_t;
            posit.m_lower_pos = s.pos_lower_t;
            add_attr(&attributes, &posit, left_box.menu[pos].complete_path);

            if (left_box.n_elements != 0) {
                free_array(&left_box);
                left_box.n_elements = 0;
                init(&left_box, right_box.n_elements);
            }
            dup_array(&right_box, &left_box);
            erase_window(&w2, &s);
            if (right_box.n_elements != 0) {
                free_array(&right_box);
                right_box.n_elements = 0;
                init(&right_box, 1);
            }
            pos = 0;
            option = update(&w1, &s, &pos, left_box.n_elements);
            secondary_loop = 1;
            back_pressed = 0;
        } else if ((c == 'h' || c == KEY_BACKSPACE)  ) {
            // && 
            //    (strlen(left_box.menu[pos].complete_path) != 1) 
            // && !strcmp(left_box.menu[pos].complete_path, "/")

            // char *parent_up = get_parent(left_box.menu[pos].complete_path);
            //if (strlen(get_parent(left_box.menu[pos].complete_path)) <= 1) {

            int n_pos = 0;
            //if (num_of_slashes(left_box.menu[pos].complete_path) > 0) 
            //    n_pos = get_last_slash_pos(left_box.menu[pos].complete_path);

            int len_cur = strlen(left_box.menu[pos].complete_path);
            int j = 0;
            if (len_cur > 0) {
                j = len_cur - 1;
                if (n_pos > 0) {
                    for (; j >= 0; --j) {
                        if (left_box.menu[pos].complete_path[j] == '/') break;
                    }
                }
            }
            n_pos = get_last_slash_pos(left_box.menu[pos].complete_path);
            if (n_pos > 0) {
                parent_up = (char *)malloc(sizeof(char) * (len_cur - n_pos ));
                if (parent_up) {
                    strncpy(parent_up, left_box.menu[pos].complete_path, len_cur - n_pos - 1);
                    parent_up[len_cur - n_pos - 1] = '\0';
                }
            }

            char DIRECTORY[sizeof(CUR_DIR)];
            sprintf(position, place, w_main.y_beg , w_main.x_beg);
            move(1, position);
            write(1, "\033[2K", sizeof("\033[2K"));
            write(1, DIRECTORY, strlen(DIRECTORY));
            //write(1, left_box.menu[pos].complete_path, len_cur);
            write(1, parent_up, strlen(parent_up));


            /*

            char j_number[sizeof(J_NUM)];
            char l_number[sizeof(L_NUM)];
            sprintf(j_number, J_NUM, j);
            sprintf(l_number, L_NUM, n_pos);
            sprintf(del_in, del, 50);

            sprintf(position, place, w_main.y_beg , w_main.x_beg);
            move(1, position);
            del_from_cursor(del_in);
            write(1, j_number, strlen(j_number));

            
            sprintf(position, place, w_main.y_beg + 1, w_main.x_beg);
            move(1, position);
            del_from_cursor(del_in);
            write(1, l_number, strlen(l_number));
            */
            free(parent_up);

            if (strlen(left_box.menu[pos].name) != 0) { // n_pos == j strlen(left_box.menu[pos].complete_path)

            erase_window(&w1, &s);
            --enter_backspace;

            if (left_box.n_elements != 0) {
                // chercher le parent avec l'inode place en ordre decroissant

                char *parent = get_parent(left_box.menu[pos].complete_path);
                char *r_parent = NULL;
                int b = strlen(left_box.menu[pos].complete_path) - 1;
                for (; b >= 0; --b) {
                    if (left_box.menu[pos].complete_path[b] == '/') {
                        break;
                    }
                }
                if ((r_parent = (char *)malloc(sizeof(char) * (b + 1))) != NULL) {
                    strncpy(r_parent, left_box.menu[pos].complete_path, b);
                    r_parent[b] = '\0';
                }

                free_array(&left_box);
                left_box.n_elements = 0;
                init(&left_box, 1);
                if (b > 0) {
                    parcours(parent, 0, &left_box, 0, &w_main);
                }

                if (left_box.n_elements != 0 && r_parent != NULL) {
                    for (previous_pos = 0; previous_pos < left_box.n_elements; ++previous_pos) {
                        if (!strcmp(r_parent, left_box.menu[previous_pos].complete_path)) {
                            break;
                        }
                    }
                }
                free(r_parent);
                r_parent = NULL;
                free(parent);
                parent = NULL;
            }

            erase_window(&w2, &s);

            if (right_box.n_elements != 0) {
                free_array(&right_box);
                right_box.n_elements = 0;
                if (left_box.n_elements != 0) {
                    init(&right_box, left_box.n_elements);
                }
            }

            option = update(&w1, &s, &pos, left_box.n_elements);
            left_allocation = 0;
            secondary_loop = 1;
            backspace = 1;
            back_pressed = 1;

            }
        }

        erase_window(&w2, &s);

        initial_loop = 0;
        resized = 0;
    }
    if (left_box.n_elements != 0 || left_box.capacity != 0) {
        free_array(&left_box);
    }
    if (right_box.n_elements != 0 || right_box.capacity != 0) {
        free_array(&right_box);
    }
    if (attributes.n_elements != 0 || attributes.capacity != 0) {
        int j;
        for (j = 0; j < attributes.n_elements; ++j) {
            free(attributes.paths[j]);
            free(attributes.pos[j]);
        }
        free(attributes.paths);
        attributes.paths = NULL;
        free(attributes.pos);
        attributes.pos = NULL;
    }
    restore_config;
    return 0;
}

void handler(int sig)
{
    void *array[10];
    size_t size = backtrace(array, 10);
    restore_config;
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

void reprint_menu(Window *w, Scroll *s1, Array *a, Attributes *attr, int pos, int option)
{
    if (s1->option_previous != option || resized || reprint) {

        int n = 0;
        int found = 0;
        for (; n < attr->n_elements; ++n) {
            if (!strcmp(a->menu[pos].complete_path, attr->paths[n])) {
                found = 1;
                break;
            }
        }
        if (found  &&
                (pos - attr->pos[n]->m_upper_pos + w->y_beg + 1 > w->y_size - w->y_beg + 1)) {

            int k;
            for (k = 0; k < attr->n_elements; ++k) {
                if (!strcmp(a->menu[pos].complete_path, attr->paths[k])) {
                    s1->pos_upper_t = attr->pos[k]->m_upper_pos;
                    s1->pos_lower_t = attr->pos[k]->m_lower_pos;
                    s1->n_lower_t = s1->array_size - s1->pos_lower_t - 1;
                    pos = attr->pos[k]->m_position;
                    break;
                }
            }
            option = update(w, s1, &pos, a->n_elements);

            back_pressed = 0;
        }

        int i;
        int len = 0;

        for (i = s1->pos_upper_t; i <= s1->pos_lower_t; ++i) {
            sprintf(position, place, i - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
            if (i == pos) {
                del_from_cursor(del_in);
                sprintf(position, place, pos - s1->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                move(1, position);
                highlight2(a, &pos);
            } else if (i < a->n_elements) {
                len = strlen(a->menu[i].name);
                if (len > w->x_size - 2) {
                    len = w->x_size - 2;
                }
                write(1, a->menu[i].name, len);
            }
        }

    }
}

void copy_scroll(Scroll *s_in, Scroll *s_out)
{
    s_out->n_to_print = s_in->n_to_print;
    s_out->n_lower_t = s_in->n_lower_t;
    s_out->array_size = s_in->array_size;
    s_out->option_previous = s_in->option_previous;
    s_out->pos_lower_t = s_in->pos_lower_t;
    s_out->pos_upper_t = s_in->pos_upper_t;
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
    sprintf(del_in, del, w->x_size - 2);
    for (i = 0; i <  w->y_size - 1; ++i) {
        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
        move(1, position);
        del_from_cursor(del_in);
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
    if (len > w1.x_size - 2)
        len = w1.x_size - 2;

    int horiz = w1.x_size - len - 2;
    write(1, bg_cyan, sizeof(bg_cyan));
    write(1, a->menu[*pos].name, len);
    int i;
    if (horiz > 0) {
        for (i = 0; i < horiz; ++i)
            write(1, space, 1);
    }
    write(1, bg_reset, sizeof(bg_reset));
}

static void sig_win_ch_handler(int sig) { resized = 1;}

int update(Window *w, Scroll *s, int *pos, int size)
{
    int option = 0;
    s->array_size = size;

    int y = w->y_size - 1;
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
        s->n_to_print = y ;
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
        if (s->n_to_print != w->y_size - 1) {
            s->n_to_print = w->y_size  - 1;
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
        option = 3;
    }
    s->option_previous = option;

    return option;
}

void print_entries(Window *w, Scroll *s, __attribute__((__unused__)) char **entries,
                    int option, int *c, int *pos, Array *a)
{
    int i;
    int y = 0;
    char in[strlen(del)];
    sprintf(del_in, del, w->x_size - 2);
    int len = 0;

    switch (*c) {
        case KEY_UP:
        case UP:
            if (*pos > 0) {
                --*pos;
                resized = 0;
                if (*pos >= s->pos_upper_t && *pos < s->pos_lower_t) {
                    y = *pos - s->pos_upper_t + w->y_beg + 1;
                    sprintf(position, place, y + 1, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(del_in);

                    len = strlen(a->menu[*pos + 1].name);
                    if (len > w->x_size - 2) {
                        len = w->x_size - 2;
                    }

                    write(1, a->menu[*pos + 1].name, len);
                    sprintf(position, place, y, w->x_beg + 1);
                    move(1, position);
                } else if (*pos <= s->pos_upper_t && s->pos_upper_t > 0) {
                    --s->pos_upper_t;
                    ++s->n_lower_t;
                    --s->pos_lower_t;
                    for (i = 0; i < s->n_to_print; ++i) {
                        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(del_in);
                        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        if (s->pos_upper_t + i < s->array_size + w->y_beg) {

                            len = strlen(a->menu[s->pos_upper_t + i].name);
                            if (len > w->x_size - 2) {
                                len = w->x_size - 2;
                            }
                            write(1, a->menu[s->pos_upper_t + i].name, len);
                        }
                    }
                //del_from_cursor(del_in);
                sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                move(1, position);
                }
                if (*pos >= s->pos_lower_t) {
                    *pos = s->pos_lower_t;
                    if (*pos < s->array_size - 1) {
                        sprintf(position, place, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(del_in);
                        sprintf(position, place, *pos + w->y_beg - s->pos_upper_t + 1, w->x_beg + 1);
                        move(1, position);
                    }
                }

                if (*pos < s->pos_lower_t + 1) {
                    //sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    //del_from_cursor(del_in);
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
                    sprintf(position, place, y, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(del_in);
                    if (*pos - 1 < s->array_size) {
                        len = strlen(a->menu[*pos - 1].name);
                        if (len > w->x_size - 2) {
                            len = w->x_size - 2;
                        }
                        write(1, a->menu[*pos - 1].name, len);
                    }
                    sprintf(position, place, y + 1, w->x_beg + 1);
                    move(1, position);
                    outside_box = 0;
                } else if (*pos - 1 >= s->pos_lower_t) {
                    ++s->pos_upper_t;
                    --s->n_lower_t;
                    ++s->pos_lower_t;

                    for (i = 0; i < s->n_to_print; ++i) {

                        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(del_in);

                        if (s->pos_upper_t + i < s->array_size) {
                            len = strlen(a->menu[s->pos_upper_t + i].name);
                            if (len > w->x_size - 2) {
                                len = w->x_size - 2;
                            }
                            write(1, a->menu[s->pos_upper_t + i].name, len);
                        }
                    }

                    if (*pos - s->pos_upper_t + w->y_beg + 1 < w->y_size - w->y_beg + 1) {
                        outside_box = 0;
                        sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(del_in);
                        sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                    } else {
                        outside_box = 1;
                        sprintf(position, place, i + w->y_beg, w->x_beg + 1);
                        move(1, position);
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

            sprintf(in, del, w->x_size - 2);
            if (*pos - s->n_to_print + 1 >= 0) {

                if (*pos - s->n_to_print + 1 >= s->pos_upper_t) {

                    if (debug) {
                        sprintf(position, place, w_main.y_size - 1, w->x_beg + 2);
                        move(1, position);
                        write(1, pg_up, strlen(pg_up));
                        indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "1aup");
                    }

                    if (*pos - s->pos_upper_t - w->y_beg + 1 < w->y_size - 1) { //
                        if (s->n_to_print > 1) {
                            sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                            move(1, position);
                            del_from_cursor(in);
                            sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                            move(1, position);

                            len = strlen(a->menu[*pos].name);
                            if (len > w->x_size - 2) {
                                len = w->x_size - 2;
                            }
                            write(1, a->menu[*pos].name, len);
                        }
                        *pos -= (s->n_to_print - 1); // donne *pos = *pos - s->n_to_print + 1
                        sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);

                    } else {
                        sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(in);
                        *pos = s->pos_lower_t;
                        sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                    }
                    highlight2(a, pos);

                } else {

                    if (debug) {
                        indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "1bup");
                    }

                    sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(in);

                    sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);


                    len = strlen(a->menu[*pos].name);
                    if (len > w->x_size - 2) {
                        len = w->x_size - 2;
                    }
                    write(1, a->menu[*pos].name, len);
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

                        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(in);

                        if (*pos + i < s->array_size - 1) {
                            len = strlen(a->menu[*pos + i].name);
                            if (len > w->x_size - 2) {
                                len = w->x_size - 2;
                            }
                            write(1, a->menu[*pos + i].name, len);
                        }
                    }

                    sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);
                    highlight2(a, pos);
                }
            } else if (*pos - s->n_to_print < 0) {
                if (debug) {
                    indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "2up");
                }

                *pos = 0;

                //sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                //move(1, position);
                for (i = 0; i < s->n_to_print; ++i) {

                    sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(in);

                    if (*pos + i < s->array_size) {
                        len = strlen(a->menu[*pos + i].name);
                        if (len > w->x_size - 2) {
                            len = w->x_size - 2;
                        }
                        write(1, a->menu[*pos + i].name, len);
                    }
                }

                s->pos_lower_t -= s->pos_upper_t;
                s->pos_upper_t = *pos;
                s->n_lower_t = s->array_size - s->n_to_print;
                sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                move(1, position);

                highlight2(a, pos);
            }
            break;
        case KEY_PAGE_DN:
                //char pg_dn[] = "pg_dn";

            sprintf(in, del, w->x_size - 2);
            if (*pos + s->n_to_print - 1 <= s->array_size - 1) {
                if (debug) {
                    sprintf(position, place, w_main.y_size - 2, w->x_beg + 2);
                    move(1, position);
                    write(1, pg_dn, strlen(pg_dn));
                    indicators(w, w_main.y_size - 1, w->x_beg + 2, position, del_in, "1");
                }

                if (*pos + s->n_to_print - 1 <= s->pos_lower_t) { // no scroll

                    sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(in);

                    sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);


                    len = strlen(a->menu[*pos].name);
                    if (len > w->x_size - 2) {
                        len = w->x_size - 2;
                    }
                    write(1, a->menu[*pos].name, len);

                    *pos += (s->n_to_print - 1);
                } else {
                    if (debug) {
                        indicators(w, w->y_size + 3, w->x_beg + 1, position, del_in, "1a");
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
                        sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                        move(1, position);
                        del_from_cursor(in);

                        if (*pos + i < s->array_size) {

                            len = strlen(a->menu[*pos + i].name);
                            if (len > w->x_size - 2) {
                                len = w->x_size - 2;
                            }
                            write(1, a->menu[*pos + i].name, len);
                            //write(1, "a", strlen("a"));
                        }
                    }
                    *pos += (s->n_to_print - 1);
                }
                sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
                move(1, position);
                highlight2(a, pos);

            } else if (*pos + s->n_to_print - 1 > s->array_size - 1) {
                if (debug) {
                    indicators(w, w_main.y_size - 2, w->x_beg + 1, position, del_in, "2  ");
                }

                for (i = 0; i < s->n_to_print; ++i) {
                    sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                    move(1, position);
                    del_from_cursor(in);

                    len = strlen(a->menu[s->pos_upper_t + s->n_lower_t + i].name);
                    if (len > w->x_size - 2) {
                        len = w->x_size - 2;
                    }
                    write(1, a->menu[s->pos_upper_t + s->n_lower_t + i].name, len);
                }

                s->pos_upper_t += s->n_lower_t;
                s->pos_lower_t = s->array_size - 1;
                s->n_lower_t = 0;

                *pos = s->array_size - 1;

                sprintf(position, place, s->n_to_print + w->y_beg, w->x_beg + 1);
                move(1, position);
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
                sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                move(1, position);
                del_from_cursor(del_in);

                len = strlen(a->menu[i].name);
                if (len > w->x_size - 2) {
                    len = w->x_size - 2;
                }
                write(1, a->menu[i].name, len);
            }
            sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
            highlight2(a, pos);
            break;
        case KEY_END:
            s->pos_upper_t = s->array_size - s->n_to_print;
            *pos = s->array_size - 1;
            s->pos_lower_t = *pos;
            s->n_lower_t = 0;
            for (i = 0; i < s->n_to_print; ++i) {
                sprintf(position, place, i + w->y_beg + 1, w->x_beg + 1);
                move(1, position);
                del_from_cursor(del_in);

                len = strlen(a->menu[i + s->pos_upper_t].name);
                if (len > w->x_size - 2) {
                    len = w->x_size - 2;
                }
                write(1, a->menu[i + s->pos_upper_t].name, len);
            }
            sprintf(position, place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
            move(1, position);
            highlight2(a, pos);
            break;
        default:
            break;
    }

    if (*pos == 0) {
        sprintf(position, place, w->y_beg + 1, w->x_beg + 1);
        move(1, position);
    }

    snprintf(position, strlen(place), place, *pos - s->pos_upper_t + w->y_beg + 1, w->x_beg + 1);
    move(1, position);
}

void draw_box(Window *w)
{
    int i = 1, j = 0;
    int horiz = w->x_size;
    int vert = w->y_size;
    BOX_CONTOUR(line, v_line,
            lu_corner, ll_corner, ru_corner, rl_corner,
            heavy_line, heavy_v_line,
            lu_heavy_corner, ll_heavy_corner, ru_heavy_corner, rl_heavy_corner,
            heavy_uppert_corner, heavy_v_up, heavy_lowert_corner);
    if (box_color && box_thickness) {
        write(1, fg_cyan, sizeof(fg_cyan));
    }
    // upper left corner
    sprintf(position, place, w->y_beg, w->x_beg);
    move(1, position);
    if (w != &w2) {
        write(1, ARRAY[cont_2], strlen(ARRAY[cont_2]));
    } else if (w == &w2) {
        write(1, ARRAY[cont_6], strlen(ARRAY[cont_6]));
    }
    // upper horizontal line
    for (; j < horiz - 1; ++j) {
        write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
    }
    // upper right corner
    if (w == &w2) {
        sprintf(position, place, w->y_beg, w->x_beg + horiz );
        move(1, position);
        write(1, ARRAY[cont_4], strlen(ARRAY[cont_4]));
    }
    // both vertical lines
    for (; i < vert ; ++i) {
        sprintf(position, place, w->y_beg + i  , w->x_beg);
        move(1, position);
        if (w != &w2) {
            write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
        } else {
            write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
            sprintf(position, place, w->y_beg + i , w->x_beg + horiz);
            move(1, position);
            write(1, ARRAY[cont_1], strlen(ARRAY[cont_1]));
        }
    }
    // lower left corner
    sprintf(position, place, vert + w->y_beg, w->x_beg );
    move(1, position);
    if (w == &w2) {
        write(1, ARRAY[cont_8], strlen(ARRAY[cont_8]));
    } else {
        write(1, ARRAY[cont_3], strlen(ARRAY[cont_3]));
    }
    // lower horizontal line
    sprintf(position, place, vert + w->y_beg, w->x_beg + 1);
    move(1, position);
    for (j = 0; j < horiz - 1; ++j) {
        write(1, ARRAY[cont_0], strlen(ARRAY[cont_0]));
    }
    // lower right corner
    if (w == &w2) {
        sprintf(position, place, vert + w->y_beg, w->x_beg + horiz);
        move(1, position);
        write(1, ARRAY[cont_5], strlen(ARRAY[cont_5]));
    }
    write(1, fg_reset, sizeof(fg_reset));
    sprintf(position, place, vert / 2, w->x_beg + 1);
    move(1, position);
}

void mvwprintw(Window *win, int y, int x, char *str)
{
    if (win->y_size > w_main.y_size) {
        char *err = "Error: out of window size: ";
        write(2, err, strlen(err));
        write(2, __func__, strlen(__func__));
    }

    sprintf(position, place, 1, 1);
    move(1, position);
    int len = strlen(str);

    if (len > win->x_size - 2) {
        len = win->x_size - 2;
    }

    sprintf(position, place, y, x);
    move(1, position);
    //sprintf(del_in, del, win->x_size - win->x_beg - 5);
    sprintf(del_in, del, len );
    del_from_cursor(del_in);
    move(1, position);
    write(1, str, len);
}

#define snprint   "s->n_to_print: %d"
#define sposupper "s->pos_upper_t: %d"

#define snlower   "s->n_lower_t: %d"

#define len_num   "horiz: %d"

#define Y_SIZE    "y_size: %d"
#define Y_BEG     "y_beg : %d"
#define X_SIZE    "x_size: %d"
#define X_BEG     "x_beg : %d"
#define BACK_PRESSED "back_pressed: %d"

#define ATTR_N "attr_number: %d"

#define ATTRIBUTES_PATH "attr[size-1]: %s"

void print_debug(Window *w, Scroll *s, int option, int pos, int cursor_pos, Array *a)
{
    char back[sizeof(BACK_PRESSED)];
    sprintf(position, place, 1, w->x_beg + 25);
    move(1, position);
    del_from_cursor(del_in);
    sprintf(back, BACK_PRESSED, back_pressed);
    write(1, back, strlen(back));

    char x_size[sizeof("%d")];
    char y_size[sizeof("%d")];
    char pos_place[sizeof(place) + 1];

    sprintf(pos_place, place, 1, w->x_size - 3);
    move(1, pos_place);
    sprintf(y_size, "%d", w->y_size);
    write(1, y_size, strlen(y_size));

    sprintf(pos_place, place, 2, w->x_size - 3);
    move(1, pos_place);
    sprintf(x_size, "%d", w->x_size);
    write(1, x_size, strlen(x_size));

    sprintf(position, place, w->y_size - 3, (w->x_size / 2) - 30);
    move(1, position);
    char ny_size[sizeof(Y_SIZE)];
    sprintf(ny_size, Y_SIZE, w1.y_size);
    write(1, ny_size, strlen(ny_size));

    sprintf(position, place, w->y_size - 2, (w->x_size / 2) - 30);
    move(1, position);
    char nx_size[sizeof(X_SIZE)];
    sprintf(nx_size, X_SIZE, w1.x_size);
    write(1, "\033[2K", sizeof("\033[2K"));
    write(1, nx_size, sizeof(nx_size));

    char num[strlen(snprint)];

    char attributes_num[sizeof(ATTR_N)];
    sprintf(attributes_num, ATTR_N, attributes.n_elements);
    sprintf(position, place, w->y_size - 4, w->x_beg + 2);
    move(1, position);
    del_from_cursor(del_in);
    write(1, attributes_num, sizeof(attributes_num));

    sprintf(pos_place, place, w->y_beg + 1, w->x_beg + 1);
    move(1, pos_place);
    del_from_cursor(del_in);
    sprintf(num, snprint, s->n_to_print);
    write(1, num, strlen(num));

    sprintf(position, place, w->y_size - 3, (w->x_size / 2) - 10);
    move(1, position);
    int len = strlen(a->menu[pos].name);
    int horiz = 0;
    //char num_len[sizeof(len_num) + 1]; // a cause du signe negatif ajouter + 1
    if (len > w1.x_size - w1.x_beg - 4) {
        len = w1.x_size - w1.x_beg - 4;
        horiz = w1.x_size - len;
    }
    if (horiz < 0) {
        horiz *= -1;
    }

    char num_pos[strlen(sposupper)];

    sprintf(pos_place, place, w->y_beg + 1, (w->x_size / 2) + 5);
    move(1, pos_place);
    sprintf(num_pos, sposupper, s->pos_upper_t);
    del_from_cursor(del_in);
    sprintf(pos_place, place, w->y_beg + 1, (w->x_size / 2) + 5);
    move(1, pos_place);
    write(1, num_pos, strlen(num_pos));

    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 5);
    move(1, pos_place);

    del_from_cursor(del_in);
    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 5);
    move(1, pos_place);
    sprintf(num_pos, "s->pos_lower_t: %d", s->pos_lower_t);
    write(1, num_pos, strlen(num_pos));

    char num_lower[strlen(snlower)];
    sprintf(pos_place, place, w->y_size - 2, (w->x_size / 2) + 25);
    move(1, pos_place);
    del_from_cursor(del_in);
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
    sprintf(pos_c, place, w->y_size + 4, w->x_size - 20);
    move(1, pos_c);
    char de[sizeof(del)];
    sprintf(de, del, 20);

    //char attr_first[sizeof("attr[size-1]: %s")];
    //sprintf(pos_c, place, w->y_size + 2, w->x_size - 60);
    //move(1, pos_c);
    //sprintf(de, del, 30);
    //del_from_cursor(de);
    //sprintf(pos_c, place, w->y_size + 2, w->x_size - 60);
    //move(1, pos_c);
    /*
    if (attributes.n_elements != 0) {
        sprintf(attr_first, ATTRIBUTES_PATH, attributes.paths[attributes.n_elements - 1]);
        write(1, attr_first, strlen(attr_first));
    }
    */
    char attr_arr[sizeof("attr_arr: %s")];
    sprintf(pos_c, place, w->y_size - 1, w->x_beg + 1);
    move(1, pos_c);
    sprintf(de, del, 40);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size - 1, w->x_beg + 1);
    move(1, pos_c);
    sprintf(attr_arr, "attr_arr: %s", attributes.paths[0]);
    write(1, attr_arr, strlen(attr_arr));

    char entr_bckspc[sizeof("enter_backspace: %d")];
    sprintf(pos_c, place, w->y_size, w->x_beg);
    move(1, pos_c);
    sprintf(de, del, 20);
    del_from_cursor(de);
    sprintf(pos_c, place, w->y_size, w->x_beg);
    move(1, pos_c);
    sprintf(entr_bckspc, "enter_backspace: %d", enter_backspace);
    write(1, entr_bckspc, strlen(entr_bckspc));

    // replace le curseur a la 1ere lettre de la dern entree
    sprintf(pos_c, place, cursor_pos, w1.x_beg + 1);
    move(1, pos_c);
}
