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
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#define WBG "\033[48;5;252m"
#define BFG "\033[38;5;0m"
#define RES "\033[0m"
unsigned short rows, cols;
void signal_handler(int);
char *get_file(const char *path);
char **get_lines(const char *path);
// print lines from x to x plus y
int x_to_xy(char **line, int x, int y);
static struct termios oldt, newt;
void cleanup(void);

int main(int argc, char **argv)
{
    sysconf(_SC_ATEXIT_MAX);
    if (atexit(cleanup) != 0)
    {
        fprintf(stderr, "more: cannot set exit function\n");
        exit(1);
    }
    /*tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
    tcgetattr(STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;
    /*ICANON normally takes care that one line at a time will be processed
    that means it will return if it sees a "\n" or an EOF or an EOL*/
    newt.c_lflag &= ~(ICANON | ECHO);
    /*Those new settings will be set to STDIN
    TCSANOW tells tcsetattr to change attributes immediately. */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    struct winsize ws;
    ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
    rows = ws.ws_row;
    cols = ws.ws_col;
    struct sigaction s;
    s.sa_handler = signal_handler;
    sigemptyset(&s.sa_mask);
    sigaddset(&s.sa_mask, SIGINT);
    sigaddset(&s.sa_mask, SIGSEGV);
    sigaddset(&s.sa_mask, SIGTERM);
    sigaddset(&s.sa_mask, SIGQUIT);
    sigaddset(&s.sa_mask, SIGTSTP);
    sigaddset(&s.sa_mask, SIGWINCH);
    s.sa_flags = SA_RESTART;
    //
    int user_lines;
    bool num = false, sqz = false, nbl = false;

    char opt;

    while ((opt = getopt(argc, argv, "1234567891:2:3:4:5:6:7:8:9:sd")) != -1)
    {
        if ((opt >= '1') && (opt <= '9'))
        {
            num = true;
            if (optarg != NULL)
            {
                char conv[strlen(optarg) + 2];
                sprintf(conv, "%c%s", opt, optarg);
                user_lines = atoi(conv);
            }
            else
                user_lines = atoi(&opt);
            continue;
        }
        if (opt == 's')
        {
            sqz = true;
            continue;
        }
        if (opt == 'd')
        {
            nbl = true;
        }
    }
    bool exit = false, banner = false, shownbanner = false;
    int space_def;
    if (optind + 1 < argc)
        banner = true;
    for (int i = optind; (i < argc) && !exit; i++)
    {
        shownbanner = false;
        space_def = rows;
        char **line = get_lines(argv[i]);
        if (line == NULL)
            continue;
        bool ts = false;
        if (banner)
        {
            if (i == optind)
            {
                shownbanner = true;
                ts = true;
                printf("::::::::::::::\n%s\n::::::::::::::\n", argv[i]);
            }
            space_def -= 3;
        }
        int start = 0;
        if (i == optind)
        {
            if (x_to_xy(line, start, space_def) == 0)
            {
                if (optind + 1 < argc)
                {
                    printf("\033[7m--More--(Next file: %s)\033[27m", argv[optind + 1]);
                }
                continue;
            }
            start += space_def - 1;
            if (shownbanner) space_def += 3;
        }
    
        while ((opt = getchar()) != EOF)
        {
            ts = false;

            if (tolower(opt) == 'q')
            {
                printf("\r\033[K");
                exit = true;
                break;
            }
            if (opt == ' ')
            {

                printf("\r\033[K");
                if (banner && !shownbanner)
                {
                    printf("::::::::::::::\n%s\n::::::::::::::\n", argv[i]);
                    shownbanner = true;
                }
                if (x_to_xy(line, start, space_def) == 0)
                {
                    if (i + 1 < argc)
                    {
                        printf("\033[7m--More--(Next file: %s)\033[27m", argv[optind + 1]);
                    }
                    break;
                }
                start += space_def - 1;
                if (shownbanner)
                {
                    space_def += 3;
                }
            }
            if (opt == '\n')
            {
                printf("\r\033[K");
                if (banner && !shownbanner)
                {
                    printf("::::::::::::::\n%s\n::::::::::::::\n", argv[i]);
                    shownbanner = true;
                }
                if (x_to_xy(line, start, 2) == 0)
                {
                    if (i + 1 < argc)
                    {
                        printf("\033[7m--More--(Next file: %s)\033[27m", argv[optind + 1]);
                    }
                    break;
                }
                start += 1;
            }

        }
        for (int i = 0; line[i] != NULL; i++)
            free(line[i]);
        free(line);
    }
    return 0;
}
int x_to_xy(char **line, int x, int y)
{   
    int lines = 0;
    while (line[lines++] != NULL)
        ;
        lines--;
    int until = (x + y ) > lines ? lines : x + y - 1;
    int from = x;
    for (int i = from; i < until; i++)
    {
        printf("%s\n", line[i]);
    }
    if (until == lines )
        return 0;

    printf("\033[7m--More--(%.f%%)\033[27m", (((double)(until)) / (lines)) * 100);
    return 1;
}
char *get_file(const char *path)
{
    struct stat st;
    if (stat(path, &st) < 0)
    {
        fprintf(stderr, "more: stat of %s failed: ", path);
        perror(NULL);
        return NULL;
    }
    int fd;
    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(stderr, "more: cannot open %s: ", path);
        perror(NULL);
        return NULL;
    }
    char *file = (char *)malloc(sizeof(char) * (st.st_size + 1));

    if ((read(fd, file, sizeof(char) * st.st_size)) < 0)
    {
        free(file);
        fprintf(stderr, "more: cannot read %s: ", path);
        perror(NULL);
        return NULL;
    }
    if (file[st.st_size - 1] != '\n')
    {
        file = (char *)realloc(file, sizeof(char) * ((st.st_size + 2)));
        file[st.st_size] = '\n';
        file[st.st_size + 1] = '\0';
    }
    return file;
}

char **get_lines(const char *path)
{
    char *file;
    if ((file = get_file(path)) == NULL)
        return NULL;
    int lines = 0;
    for (size_t i = 0; i < strlen(file); i++)
    {
        if (file[i] == '\n')
            lines++;
    }
    char **line = (char **)malloc((lines + 1) * sizeof(char *));
    line[lines] = NULL;

    int it = 0;
    size_t last = 0;
    for (size_t i = 0; i < strlen(file); i++)
    {
        // 12 \n 3 \n 3 \n
        // 01 2  3 4  5 6
        if (file[i] == '\n')
        {
            line[it] = (char *)malloc((sizeof(char)) * (i - last + 1));
            memcpy(line[it], file + last, (i - last) * sizeof(char));
            line[it][i - last] = '\0';
            last = i + 1;
            it++;
        }
    }
    free(file);
    return line;
}

void cleanup(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

void signal_handler(int signal_number)
{
    if (signal_number == SIGWINCH)
    {
        struct winsize ws;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
        rows = ws.ws_row;
        cols = ws.ws_col;
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    exit(1);
}
