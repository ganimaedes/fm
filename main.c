#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef _POSIX_SOURCE

#include "array.h"

void traverse(char *fn, int indent, Array *array, int recursive)
{
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    char path[MAXPATHLEN];
    int len_path = 0, len_type = 0;
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
                
                
                fprintf(stdout, "fn           : %s\n", fn);
                fprintf(stdout, "entry->d_name: %s\n", entry->d_name);
                fprintf(stdout, "path         : %s\n", path);
                
                len_path = strlen(path);
                menu.name = (char *)malloc(sizeof(char) * (len_path + 1));
                menu.name[len_path] = '\0';
                strcpy(menu.name, path);
                
                if (stat(path, &info) != 0) {
                    fprintf(stderr, "Error: %d %s", errno, strerror(errno));
                } else if (S_ISLNK(info.st_mode)) {
                    
                    s_l = "symbolic_link";
                    len_type = strlen(s_l);
                    menu.type = (char *)malloc(sizeof(char) * (len_type + 1));
                    menu.type[len_type] = '\0';
                    strcpy(menu.type, s_l);
                    
                } else if (S_ISREG(info.st_mode)) {
                    
                    s_l = "file";
                    len_type = strlen(s_l);
                    menu.type = (char *)malloc(sizeof(char) * (len_type + 1));
                    menu.type[len_type] = '\0';
                    strcpy(menu.type, s_l);
                    
                } else if (S_ISDIR(info.st_mode)) {
                    
                    s_l = "directory";
                    len_type = strlen(s_l);
                    menu.type = (char *)malloc(sizeof(char) * (len_type + 1));
                    menu.type[len_type] = '\0';
                    strcpy(menu.type, s_l);
                    
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
