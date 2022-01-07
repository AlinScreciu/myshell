#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define __HELP "Try 'cp --help' for more information"
#define __MISSINGDEST "cp: missing destination file operand"
#define __MISSINGFILE "cp: missing operand"

// check if path is a directory, and if it is print err_message
// error message must contain %s where the file name will be added
int check_dir(const char *, const char *);
// check if path is valid to read from
// in case of recursive directory can be valid
const char *get_type(mode_t mode);
int valid(const char *, bool);
int move(const char *, const char *, bool, bool);
char *read_file(const char *);
char *get_dest(const char *, const char *);
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
                int res;
                if (stat(optarg, &st) < 0)
                {
                    fprintf(stderr, "cp: failed to access '%s': ", optarg);
                    perror(NULL);
                    printf("\n");
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

    if (target_dir || paths > 1)
    {
        if (check_dir(target, "cp: target '%s' is not a directory\n") < 0)
            exit(1);
    }
    for (int i = 0; i < paths; i++)
    {

        char *dest;
        struct stat st;
        bool destdir = false, currentdir = false;
        if (stat(target, &st) < 0)
        {
            fprintf(stderr, "cp: failed to access '%s': ", target);
            perror(NULL);
            return -1;
        }
        if (S_ISDIR(st.st_mode) != 0)
        {
            destdir = true;
            dest = get_dest(path[i], target);
        }
        else
            dest = strdup(target);
        stat(path[i], &st);
        if ((S_ISDIR(st.st_mode) != 0))
        {
            currentdir = true;
            if (!recursive)
            {
                fprintf(stderr, "cp: -r not specified: omitting directory '%s'\n", path[i]);
                continue;
            }
        }
        if (verbose && (!currentdir && !recursive))
            printf("'%s' -> '%s'\n", path[i], dest);
        if (valid(path[i], recursive) < 0)
            continue;

        if (!recursive)
            if (move(path[i], dest, interactive, destdir) < 0)
                continue;
    }
    for (int i = 0; i < paths; i++)
    {
        free(path[i]);
    }
    free(path);
    return 0;
}
int check_dir(const char *path, const char *err_message)
{
    if (access(path, F_OK) < 0)
    {
        fprintf(stderr, "cp: failed to access '%s': ", path);
        perror(NULL);
        return -1;
    }

    struct stat st;
    if (stat(path, &st) < 0)
    {
        perror("stat");
        return -1;
    };
    if (S_ISDIR(st.st_mode) == 0)
    {
        fprintf(stderr, err_message, path);
        return -1;
    }
    return 0;
}
int valid(const char *path, bool recursive)
{

    struct stat st;
    if (stat(path, &st) < 0)
    {
        fprintf(stderr, "cp: cannot stat '%s': ", path);
        perror(NULL);
        return -1;
    };
    return 0;
}
int move(const char *path, const char *dest, bool interactive, bool destdir)
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
                return 0;
    }
    struct stat st;
    stat(path, &st);
    int df;
    struct stat nst;
    if ((stat(dest, &nst) < 0) && !destdir)
    {
        fprintf(stderr, "cp: cannot stat '%s': ", dest);
        perror(NULL);
        return -1;
    }
    if ((df = open(dest, O_WRONLY | O_CREAT | O_TRUNC)) < 0)
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
    if (fchown(df, st.st_uid, st.st_gid) < 0)
    {
        fprintf(stderr, "cp: failed to preserve ownership for %s: ", dest);
        perror(NULL);
        return -1;
    }
    if (fchmod(df, st.st_mode) < 0)
    {
        fprintf(stderr, "cp: failed to preserve permissions for %s: ", dest);
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
char *get_dest(const char *path, const char *target)
{
    char *dest;
    char *dest_base = strdup(target);
    if (dest_base[strlen(dest_base) - 1] != '/')
    {
        dest_base = realloc(dest_base, sizeof(char *) * (strlen(dest_base) + 1));
        strcat(dest_base, "/");
    }
    char *path_base = basename(path);
    dest = (char *)malloc(sizeof(char) * (strlen(dest_base) + strlen(path) + 1));
    strcpy(dest, dest_base);
    strcat(dest, path_base);
    free(dest_base);
    return dest;
}