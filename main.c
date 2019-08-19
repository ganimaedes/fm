#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef _POSIX_SOURCE

#include "print.h"

char *insert_to_menu(char *str)
{
    int len_str = strlen(str);
    char *add = (char *)malloc(sizeof(char) * (len_str + 1));
    add[len_str] = '\0';
    strcpy(add, str);
    return add;
}

int get_last_slash_pos(Array *a, int i)
{
    int j, len = strlen(a->menu[i].name);
    for (j = len - 1; j >= 0; --j) {
        if (a->menu[i].name[j] == '/') {
            return j;
        }
    }
    return -1;
}

char **get_name_only(Array *a)
{
    char **names = (char **)calloc(a->n_elements, sizeof(char *));
    int i, len = 0;
    int slash_pos = 0;
    int total = 0;
    for (i = 0; i < a->n_elements; ++i) {
        slash_pos = get_last_slash_pos(a, i);
        if (slash_pos > 0) {
            len = strlen(a->menu[i].name);
            total = len - slash_pos;
            names[i] = (char *)malloc(sizeof(char) * (total + 1));
            strncpy(names[i], &a->menu[i].name[slash_pos + 1], total);
            names[i][total] = '\0';
        }
    }
    return names;
}

void traverse(char *fn, int indent, Array *array, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    char path[MAXPATHLEN];
    Menu menu;
    char *s_l;

    if ((dir = opendir(fn)) == NULL) {
        perror("opendir error\n");
    } else {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                strcpy(path, fn);
                strcat(path, "/");
                strcat(path, entry->d_name);

                menu.name = insert_to_menu(path);

                if (stat(path, &info) != 0) {
                    fprintf(stderr, "Error: %d %s", errno, strerror(errno));
                } else if (S_ISLNK(info.st_mode)) {

                    s_l = "symbolic_link";
                    menu.type = insert_to_menu(s_l);

                } else if (S_ISREG(info.st_mode)) {

                    s_l = "file";
                    menu.type = insert_to_menu(s_l);

                } else if (S_ISDIR(info.st_mode)) {

                    s_l = "directory";
                    menu.type = insert_to_menu(s_l);

                    if (recursive) {
                        traverse(path, indent + 1, array, 1);
                    }
                }
                add_menu(array, menu);
                free(menu.name);
                free(menu.type);
            }
        }
        (void)closedir(dir);
    }
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    printf("\x1b[?1049h\x1b[2J\x1b[H");
    if (argv[argc - 1] == NULL) { fprintf(stderr, "Error: Missing Arguments\n"); return EXIT_FAILURE; }
    int len_argv = strlen(argv[1]);
    if (argv[1][len_argv - 1] == '/') {
        argv[1][len_argv - 1] = '\0';
    }
    Array a;
    init(&a, 1);
    fprintf(stdout, "%s\n", argv[1]);
    traverse(argv[1], 0, &a, 0);

    //print_array(&a);

    int c = 0; int pos = 0; int len = 0; int maxlen = 60;

    Window w_1;

    if (get_window_size(STDIN_FILENO, STDOUT_FILENO, &w_1.y_size, &w_1.x_size)) {
        printf("\x1b[2J\x1b[H\x1b[?1049l");
        perror("Error getting window size\n");
        return EXIT_FAILURE;
    }

    w_1.y_beg = 5;
    w_1.x_beg = 5; // w_1 goes to x == 65 (maxlen + w_1.x_beg)

    Scroll s = set_scroll(pos, a.n_elements);

    int SIZE = 0;
    if (s.n_to_print < a.n_elements) {
        SIZE = s.n_to_print;
    } else {
        SIZE = a.n_elements - 1;
    }

    int i = 0; int y = 0;

    char **names = get_name_only(&a);

    for (i = 0; i < SIZE; ++i) {
        gotoyx(i + w_1.y_beg, w_1.x_beg);
        if (i == 0) {
            len = strlen(names[i]);
            highlight(&names[i], &maxlen, &i, &len);
        } else {
            printf("%s", names[i]);
        }
    }

    while (1) {

        if (debug_mode) {
            print_debug_info(&w_1, &s, &pos, &a.n_elements, &y);
        }

        c = kbget();
        if (c == KEY_ESCAPE) {
            break;
        } else if (c == KEY_UP || c == UP) {
            move_up(&w_1, &s, names, &pos, &y, &maxlen, SIZE, len);
        } else if (c == KEY_DOWN || c == DN) {
            move_dn(&w_1, &s, names, &pos, &y, &maxlen, SIZE, len, &a.n_elements);
        }
    }
    for (i = 0; i < a.n_elements; ++i) {
        free(names[i]);
    }
    free(names);
    names = NULL;
    free_array(&a);
    printf("\x1b[2J\x1b[H\x1b[?1049l");
    return 0;
}
