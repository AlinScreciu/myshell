#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#define __HELP "Try 'cp --help' for more information"
#define __MISSINGDEST "cp: missing destination file operand"
#define __MISSINGFILE "cp: missing operand"

// check if path is valid to read from
// in case of recursive directory can be valid
const char *get_type(mode_t);
int valid(const char *, bool);
// move path 1 to path 2
int move(const char *, const char *, bool);
char *read_file(const char *);
char *get_dest(const char *, const char *, bool);
int core_loop(char** path, int paths, const char* target, bool recursive, bool interactive, bool verbose, bool dir);
int move_from_dir(const char *, const char *, bool , bool , bool , bool );
int main(int argc, char *argv[])
{

    if (argc < 2) 
    {
        fputs(__MISSINGDEST "\n"__HELP
                            "\n",
              stderr);
        exit(1);
    }
    if (argc < 3)
    {
        fprintf(stderr, __MISSINGDEST " after '%s'\n"__HELP
                                      "\n",
                argv[1]);
        exit(1);
    }
    const char opts[] = "virRt:";
    char *target = NULL;
    int target_index;
    bool verbose = false, interactive = false, recursive = false, target_dir = false;
    char opt;
    /* -i, -r (-R), -t, -v */

    while ((opt = getopt(argc, argv, opts)) != -1)
    {
        switch (opt)
        {
        case 'v':
            verbose = true;
            break;
        case 'i':
            interactive = true;
            break;
        case 'r':
            recursive = true;
            break;
        case 'R':
            recursive = true;
            break;
        case 't':
            if (target_dir)
            {
                fprintf(stderr, "cp: multiple target directories specified\n");
                exit(1);
            }
            else
            {
                struct stat st;
                if (stat(optarg, &st) < 0)
                {
                    fprintf(stderr, "cp: failed to access '%s': ", optarg);
                    perror(NULL);
                    exit(1);
                }
                if (!S_ISDIR(st.st_mode))
                {
                    fprintf(stderr, "cp: target '%s' is not a directory\n", optarg);
                    exit(1);
                }
            }
            target_dir = true;
            target = optarg;

            break;
        }
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-t") == 0)
        {
            target_index = i + 1;
            break;
        }
    }
    int paths = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!(argv[i][0] == '-'))
        {
            if (target_dir)
                if (argv[i] == argv[target_index])
                    continue;
            paths++;
        }
    }
    if ((paths == 1 && !target) || (paths == 0 && target))
    {
        fprintf(stderr, __MISSINGFILE "?\n"__HELP
                                      "\n");
        exit(1);
    }
    char **path = (char **)calloc(paths, sizeof(char *));
    int pi = 0;
    for (int i = 1; i < argc; i++)
    {
        if (!(argv[i][0] == '-'))
        {
            if (target_dir)
                if (argv[i] == argv[target_index])
                    continue;
            path[pi++] = strdup(argv[i]);
        }
    }

    if (!target_dir)
        target = path[paths-- - 1];
    bool dir = false;
    if (access(target, F_OK) == 0)
    {
        struct stat st;
        stat(target, &st);
        dir = S_ISDIR(st.st_mode) != 0;
    }
    if (recursive)
    {
        for (int i = 0; i < paths; i++)
        {
            struct stat st;
            stat(path[i], &st);
            if ((S_ISDIR(st.st_mode) == 0) && !dir)
            {
                fprintf(stderr, "cp: target '%s' is not a directory\n", target);
                exit(1);
            }
        }
    }
    if (core_loop(path, paths, target, recursive,interactive, verbose, dir) < 0)
    {
        exit(-1);
    }

    for (int i = 0; i < paths; i++)
    {
        free(path[i]);
    }
    free(path);
    return 0;
}
int move(const char *path, const char *dest, bool interactive)
{
    char *file = read_file(path);
    if (file == NULL)
        return -1;
    if ((access(dest, F_OK) == 0) && interactive)
    {
        printf("copymv: overwrite '%s'? ", dest);
        char c;
        while ((c = getchar()) != EOF && c != '\n')
            if ((tolower(c) == 'y'))
                break;
            else
                return -1;
    }
    struct stat st;
    int df;
    stat(path, &st);

    if ((df = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0664)) < 0)
    {
        fprintf(stderr, "cp: cannot create %s '%s': ", get_type(st.st_mode), dest);
        perror(NULL);
        return -1;
    }

    if (write(df, file, sizeof(char) * (st.st_size)) < 0)
    {
        fprintf(stderr, "cp: failed to write to %s: ", dest);
        perror(NULL);
        return -1;
    }
    if (close(df) < 0)
    {
        fprintf(stderr, "cp: failed to close '%s': ", dest);
        perror(NULL);
        return -1;
    }
    free(file);
    return 0;
}
const char *get_type(mode_t mode)
{
    if (S_ISREG(mode))
        return "regular file";
    if (S_ISDIR(mode))
        return "directory";
    if (S_ISCHR(mode))
        return "character device";
    if (S_ISBLK(mode))
        return "block device";
    if (S_ISFIFO(mode))
        return "FIFO (named pipe)";
    if (S_ISLNK(mode))
        return "symbolik link";
    if (S_ISSOCK(mode))
        return "socket";
    fprintf(stderr, "cp: file mode corrupted\n");
    exit(1);
}
char *read_file(const char *path)
{
    char *file = NULL;
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        fprintf(stderr, "cp: cannot open '%s' for reading: ", path);
        perror(NULL);
        return file;
    }
    struct stat stbf;
    stat(path, &stbf);
    file = (char *)malloc((stbf.st_size + 1) * sizeof(char));
    file[stbf.st_size] = '\0';
    if (read(fd, file, stbf.st_size) < 0)
    {
        fprintf(stderr, "cp: cannot read '%s': ", path);
        perror(NULL);
        return file;
    } //    /* -i, -r (-R), -t, -v */

    if (close(fd) < 0)
    {
        fprintf(stderr, "cp: cannot close '%s': ", path);
        perror(NULL);
        return file;
    }
    return file;
}
char *get_dest(const char *path, const char *target, bool dir)
{
    // a/ a -> d d/
    // d/a
    char *dest;
    if (dir)
    {
        dest = (char *)malloc(sizeof(char) * (strlen(target) + 1));
        if (target[strlen(target) - 1] != '/')
        {
            dest = (char *)malloc(sizeof(char) * (strlen(target) + 2));
            strcpy(dest, target);
            strcat(dest, "/");
        }
        else
            strcpy(dest, target);

        if (path[strlen(path) - 1] == '/')
        {
            dest = (char *)realloc(dest, sizeof(char) * (strlen(dest) + strlen(path)));
            strncat(dest, path, strlen(path) - 1);
        }
        else
        {
            dest = (char *)realloc(dest, sizeof(char) * (strlen(dest) + strlen(path) + 1));
            strcat(dest, path);
        }
    }
    else
    {
        char *base = basename(strdup(path));
        dest = (char *)malloc(sizeof(char) * (strlen(target) + strlen(base) + 1));
        if (target[strlen(target) - 1] != '/')
        {
            dest = (char *)malloc(sizeof(char) * (strlen(target) + strlen(base) + 2));
            strcpy(dest, target);
            strcat(dest, "/");
        }
        else
            strcpy(dest, target);
        strcat(dest, base);
    }
    return dest;
}
int move_from_dir(const char *dir, const char *target, bool recursive, bool interactive, bool verbose, bool isdir)
{
    bool exists = true;

    struct stat st;
    if (stat(target, &st) < 0)
    {
        if (errno == ENOENT)
        {
            exists = false;
        }
        else
        {
            fprintf(stderr, "cp: cannot stat '%s': ", target);
            perror(NULL);
            return -1;
        }
    }
    if ((S_ISDIR(st.st_mode) != 0) && !exists)
    {
        fprintf(stderr, "cp: cannot overwrite non-directory '%s' with directory '%s'\n", dir, target);
    }
    if (!exists)
    {
        if (verbose)
            printf("'%s' -> '%s'\n", dir, target);

        if (mkdir(target, 0751) < 0)
        {
            fprintf(stderr, "cp: cannot make directory '%s': ", dir);
            perror(NULL);
            return -1;
        }
    }
    DIR* dirstr;
    if ((dirstr = opendir(dir)) == NULL) {
        fprintf(stderr, "cp: cannot open directory '%s': ",dir);
        perror(NULL);
        return -1;
    }

    char* pathpre = strdup(dir);
    if (dir[strlen(dir)-1] !='/') {
        pathpre = (char*) malloc(strlen(dir) + 2);
        strcpy(pathpre, dir);
        strcat(pathpre,"/");
    }
    struct dirent *ent;
    char** path = NULL;
    int paths = 0;
    while ((ent = readdir(dirstr)) != NULL)
    {
        if (ent->d_name[0] != '.')
        {
            path = (char**) realloc(path, sizeof(char*)*(++paths));
            path[paths - 1] = (char*)malloc(sizeof(char)*(strlen(pathpre) + strlen(ent->d_name) + 1));
            strcpy(path[paths-1], pathpre);
            strcat(path[paths-1], ent->d_name);
        }
    }
    
    if (core_loop(path, paths, target, recursive, interactive, verbose, true) < 0)
    {
        return -1;
    }
    for (int i = 0; i < paths; i++)
    {
        free(path[i]);
    }
    free(pathpre);
    free(path);
    closedir(dirstr);
    return 0;
}
int core_loop(char** path, int paths, const char* target, bool recursive, bool interactive, bool verbose, bool dir)
{
    for (int i = 0; i < paths; i++)
    {
        if (access(path[i], F_OK) < 0)
        {
            fprintf(stderr, "cp: cannot access '%s': ", path[i]);
            perror(NULL);
            continue;
        }

        char *dest = strdup(target);
        struct stat stbf;
        bool dirpath = false;
        if (stat(path[i], &stbf) < 0)
        {
            fprintf(stderr, "cp: cannot stat '%s': ", dest);
            perror(NULL);
            continue;
        }
        if (S_ISDIR(stbf.st_mode) != 0)
            dirpath = true;

        if (dirpath && !recursive)
        {
            fprintf(stderr, "cp: -r not specified; omiting directory '%s'\n", path[i]);
            continue;
        }
        if (dir)
            dest = get_dest(path[i], target, dirpath);
        if (verbose && !dirpath)
            printf("'%s' -> '%s'\n", path[i], dest);
        
        if (!dirpath)
            if (move(path[i], dest, interactive) < 0)
            {
                continue;
            }
        if (dirpath)
        {
            if (move_from_dir(path[i], dest, recursive, interactive, verbose,dir ) < 0)
            {
                continue;
            }
        }
    }
    return 0;
}