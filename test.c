#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#undef _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void traverse(char *fn, int indent)
{
    DIR *dir;
    struct dirent *entry;
    struct stat info;
    char path[MAXPATHLEN];

    for (int i = 0; i < indent; ++i) { printf(" "); }
    printf("%s\n", fn);

    if ((dir = opendir(fn)) == NULL) {
        perror("opendir error\n");
    } else {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                strcpy(path, fn);
                strcat(path, "/");
                strcat(path, entry->d_name);
                if (stat(path, &info) != 0) {
                    fprintf(stderr, "Error: %d, %s",errno, strerror(errno));
                } else if (S_ISDIR(info.st_mode)) {
                    traverse(path, indent + 1);
                }
            }
        }
        (void)closedir(dir);
    }
}

int main(int argc, char **argv)
{
    if (argv[argc - 1] == NULL) { fprintf(stderr, "Missing Arguments\n"); return EXIT_FAILURE; }
    traverse(argv[argc - 1], 0);
    return 0;
}
