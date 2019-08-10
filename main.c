#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef _POSIX_SOURCE

#include "array.h"

char *insert_to_menu_name(char *name)
{
    int len_str  = strlen(name);
    char *add_name = (char *)malloc(sizeof(char) * (len_str + 1));
    add_name[len_str] = '\0';
    strcpy(add_name, name);
    return add_name;
}

char *insert_to_menu_type(char *type)
{
    int len_str  = strlen(type);
    char *add_type = (char *)malloc(sizeof(char) * (len_str + 1));
    add_type[len_str] = '\0';
    strcpy(add_type, type);
    return add_type;
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
                
                menu.name = insert_to_menu_name(path);

                if (stat(path, &info) != 0) {
                    fprintf(stderr, "Error: %d %s", errno, strerror(errno));
                } else if (S_ISLNK(info.st_mode)) {
                    
                    s_l = "symbolic_link";
                    menu.type = insert_to_menu_type(s_l);

                } else if (S_ISREG(info.st_mode)) {
                    
                    s_l = "file";
                    menu.type = insert_to_menu_type(s_l);

                } else if (S_ISDIR(info.st_mode)) {
                    
                    s_l = "directory";
                    menu.type = insert_to_menu_type(s_l);

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
    if (argv[argc - 1] == NULL) { fprintf(stderr, "Error: Missing Arguments\n"); return EXIT_FAILURE; }
    int len_argv = strlen(argv[1]);
    if (argv[1][len_argv - 1] == '/') {
        argv[1][len_argv - 1] = '\0';
    } 
    Array a;
    init(&a, 1);
    fprintf(stdout, "%s\n", argv[1]);
    traverse(argv[1], 0, &a, 0);

    print_array(&a);

    free_array(&a);
    return 0;
}


/*
 * fprintf(stdout, "fn           : %s\n", fn);
                fprintf(stdout, "entry->d_name: %s\n", entry->d_name);
                fprintf(stdout, "path         : %s\n", path);
 * 
 * 
 */
