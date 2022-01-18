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
#include <ctype.h>

#define WBG "\033[48;5;252m"
#define BFG "\033[38;5;0m"
#define RES "\033[0m"

unsigned short rows;
void signal_handler(int);
char *get_file(const char *path, bool *binerr);
char **get_lines(const char *path, int *lines, bool squeeze, bool *binary);
// print lines from x to x plus y
int x_to_xy(char **line, int x, int y);
static struct termios oldt, newt;
void cleanup(void);
void print_sep(char sep, int times)
{
    while (times--)
    {
        putchar(sep);
    }
}
void usage(void)
{
    fputs("Most commands optionally preceded by integer argument k.  "
          "Defaults in brackets.\n"
          "Star (*) indicates argument becomes new default.\n",
          stdout);
    print_sep('-', 79);
    fputs(

        "\n<space>                 Display next k lines of text [current screen size]\n"
        "z                       Display next k lines of text [current screen size]*\n"
        "<return>                Display next k lines of text [1]*\n"
        "d or ctrl-D             Scroll k lines [current scroll size, initially 11]*\n"
        "q or Q or <interrupt>   Exit from more\n"
        "s                       Skip forward k lines of text [1]\n"
        "f                       Skip forward k screenfuls of text [1]\n"
        "b or ctrl-B             Skip backwards k screenfuls of text [1]\n"
        "=                       Display current line number\n"
        "ctrl-L                  Redraw screen\n"
        ":n                      Go to kth next file [1]\n"
        ":p                      Go to kth previous file [1]\n"
        ":f                      Display current file name and line number\n"
        ".                       Repeat previous command\n",
        stdout);
    print_sep('-', 79);
    printf("\n");
}
int inFd;
bool piped;
char *in;
int main(int argc, char **argv)
{
    piped = !isatty(STDIN_FILENO);
    bool outtty = isatty(STDOUT_FILENO);
    sysconf(_SC_ATEXIT_MAX);
    if (atexit(cleanup) != 0)
    {
        fprintf(stderr, "more: cannot set exit function\n");
        exit(1);
    }
    /*tcgetattr gets the parameters of the current terminal
    STDIN_FILENO will tell tcgetattr that it should write the settings
    of stdin to oldt*/
    if (piped)
    {
        bool err = false;
        in = get_file("-",&err);
        close(STDIN_FILENO);
        inFd = open("/dev/tty", O_RDONLY);
        dup2(STDIN_FILENO, inFd);
    }

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

    rows = ws.ws_row - 1;
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

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-' && isdigit(argv[i][1]))
        {
            num = true;
            user_lines = atoi(argv[i] + 1);
        }
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'd')
            {
                nbl = true;
            }
            if (argv[i][1] == 's')
            {
                sqz = true;
            }
        }
    }
    bool exit = false, banner = false, shownbanner = false;
    char opt;
    int paths = 0;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            paths++;
    }
    if (piped)
        paths++;
    char *path[paths];
    if (paths > 1)
        banner = true;
    paths = 0;
    if (piped)
        path[paths++] = strdup("-");

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            path[paths++] = strdup(argv[i]);
    }
    int space_def;

    int initial = rows;
    if (num)
        initial = user_lines;

    if (!outtty){
        for (int i = 0; (i < paths); i++)
        {
            bool err;
            char *file = get_file(path[i], &err);
            if (file == NULL)
            {
                if (err)
                    printf("\n******** %s: Not a text file ********\n\n", path[i]);

                continue;
            }
            if (banner && !piped)
            {
                printf("::::::::::::::\n%s\n::::::::::::::\n", path[i]);
            }

            for (size_t j = 0; j < strlen(file); j++)
            {
                printf("%c", file[j]);
            }
            free(file);
        }
    }
    else
        for (int i = 0; (i < paths) && !exit; i++)
        {
            int file_lines;
            bool shownbanner = false;
            bool binerr = false;
            char **line = get_lines(path[i], &file_lines, sqz, &binerr);
            if (line == NULL)
            {
                if (binerr)
                    fprintf(stderr, "\n******** %s: Not a text file ********\n\n", path[i]);
                if (banner)
                {
                    if (nbl)
                        printf("\033[7m--More--(Next file: %s)[Press space to continue, 'q' to quit.]\033[27m", path[i + 1]);
                    else
                        printf("\033[7m--More--(Next file: %s)\033[27m", path[i + 1]);
                }
                continue;
            }
            // printf("\r\033[K");
            int edef = 1, zdef = rows, ddef = 11;
            bool z_def = true;

            if (banner && i == 0 && !piped)
            {
                printf("::::::::::::::\n%s\n::::::::::::::\n", path[i]);
                shownbanner = true;
                if (initial > (rows - 3))
                    initial = rows - 3;
            }
            int from = 0;
            if (i == 0)
            {
                if (x_to_xy(line, from, initial) == 0)
                {
                    if (banner)
                    {
                        if (nbl)
                            printf("\033[7m--More--(Next file: %s)[Press space to continue, 'q' to quit.]\033[27m", path[i + 1]);
                        else
                            printf("\033[7m--More--(Next file: %s)\033[27m", path[i + 1]);
                    }

                    for (int i = 0; line[i] != NULL; i++)
                        free(line[i]);
                    free(line);
                    continue;
                }
                from += initial;
                if (nbl)
                    printf("\033[7m--More--(%.f%%)[Press space to continue, 'q' to quit.]\033[27m", ((double)from / file_lines) * 100);
                else
                    printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
            }
            char *numbuff = (char *)malloc(1 * sizeof(char));
            bool getnum = false;
            int numit = 0;
            int last_from = 0, last_until = initial;
            // two dot opt, eg: :n :etc
            bool tdopt = false;
            bool cont = false;
            while ((opt = getchar()) != EOF)
            {
                if ((opt >= '0') && (opt <= '9'))
                {
                    if (opt == '0' && numit == 0)
                        continue;
                    getnum = true;
                    numbuff[numit++] = opt;
                    numbuff = (char *)realloc(numbuff, sizeof(char) * (numit + 1));
                    numbuff[numit] = '\0';
                    continue;
                }
                if (opt == ':')
                {
                    tdopt = true;
                    continue;
                }
                if (tdopt)
                {
                    if (opt == 'f')
                    {
                        printf("\r\033[k\"%s\" line %d", path[i], from);
                    }
                    tdopt = false;
                    continue;
                }
                if (opt == '=')
                {
                    printf("\r\033[K%d", last_from);
                    continue;
                }
                if (opt == 12)
                {
                    printf("\r\033[K");

                    if (x_to_xy(line, last_from, last_until) == 0)
                    {
                        if (banner)
                        {
                            if (nbl)
                                printf("\033[7m--More--(Next file: %s)[Press space to continue, 'q' to quit.]\033[27m]", path[i + 1]);
                            else
                                printf("\033[7m--More--(Next file: %s)\033[27m]", path[i + 1]);
                        }
                        free(numbuff);
                        break;
                    }
                    if (nbl)
                        printf("\033[7m--More--(%.f%%)[Press space to continue, 'q' to quit.]\033[27m", ((double)last_from / file_lines) * 100);
                    else
                        printf("\033[7m--More--(%.f%%)\033[27m", ((double)last_from / file_lines) * 100);
                    continue;
                }
                if (opt == 'h')
                {
                    printf("\r\033[K");

                    usage();
                    if (nbl)
                        printf("\033[7m--More--(%.f%%)[Press space to continue, 'q' to quit.]\033[27m", ((double)from / file_lines) * 100);
                    else
                        printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
                    continue;
                }
                if (z_def)
                    zdef = rows;

                if (tolower(opt) == 'q')
                {
                    printf("\r\033[K");
                    free(numbuff);
                    exit = true;
                    break;
                }
                if (opt == ' ' || opt == '\n' || opt == 'z' || opt == 4 || opt == 'd')
                {
                    printf("\r\033[K");
                    int jump;
                    if (opt == ' ' || opt == 'z')
                    {
                        jump = rows;
                        if (num)
                            jump = user_lines;
                        if (getnum)
                            jump = atoi(numbuff);
                    }
                    if (opt == 'z')
                    {
                        if (getnum)
                        {
                            zdef = atoi(numbuff);
                            z_def = false;
                        }
                        jump = zdef;
                    }
                    if (opt == '\n')
                    {
                        jump = 1;
                        if (getnum)
                            jump = atoi(numbuff);
                    }
                    if (opt == 4 || opt == '4')
                        jump = 11;
                    if (banner && !shownbanner)
                    {
                        printf("::::::::::::::\n%s\n::::::::::::::\n", path[i]);
                        shownbanner = true;
                        if (jump > (rows - 3))
                            jump = rows - 3;
                    }
                    last_until = jump;
                    if (x_to_xy(line, from, jump) == 0)
                    {
                        if (banner && (i + 1 < paths))
                        {
                            if (nbl)
                                printf("\033[7m--More--(Next file: %s)[Press space to continue, 'q' to quit.]\033[27m", path[i + 1]);
                            else
                                printf("\033[7m--More--(Next file: %s)\033[27m", path[i + 1]);
                        }
                        getnum = false;
                        free(numbuff);
                        numit = 0;
                        break;
                    }
                    from += jump;
                    last_from = from;
                    if (nbl)
                        printf("\033[7m--More--(%.f%%)[Press space to continue, 'q' to quit.]\033[27m", ((double)from / file_lines) * 100);
                    else
                        printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
                    getnum = false;
                    free(numbuff);
                    numbuff = (char *)malloc(1 * sizeof(char));
                    numit = 0;
                    continue;
                }
                if (!nbl)
                    printf("\a");
            }

            for (int i = 0; line[i] != NULL; i++)
                free(line[i]);
            free(line);
        }
    for (int i = 0; (i < paths); i++)
        free(path[i]);

    return 0;
}
int x_to_xy(char **line, int x, int y)
{

    while (y-- > 0 && line[x] != NULL)
    {
        printf("%s\n", line[x++]);
    }
    if (line[x] == NULL)
        return 0;
    return 1;
}
char *get_file(const char *path, bool *binerr)
{
    if (strcmp(path, "-") == 0)
    {
        int c;
        size_t p4kB = 4096, i = 0;
        void *newPtr = NULL;
        char *ptrString = malloc(p4kB * sizeof(char));

        while (ptrString != NULL && (c = getchar()) != EOF)
        {
            if (c == '\0') 
            {
                *binerr = true;
                free(ptrString);
                return NULL;
            }

            if (i == p4kB * sizeof(char))
            {
                p4kB += 4096;
                if ((newPtr = realloc(ptrString, p4kB * sizeof(char))) != NULL)
                    ptrString = (char *)newPtr;
                else
                {
                    free(ptrString);
                    return NULL;
                }
            }
            ptrString[i++] = c;
        }

        ptrString[i] = '\0';
        return ptrString;
    }
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
    char *file = (char *)calloc((st.st_size + 1), sizeof(char));

    if ((read(fd, file, sizeof(char) * st.st_size)) < 0)
    {
        free(file);
        fprintf(stderr, "more: cannot read %s: ", path);
        perror(NULL);
        return NULL;
    }
    for (long int i = 0; i < st.st_size; i++)
    {
        if (file[i] == '\0') {
            *binerr = true;
            free(file);
            return NULL;
        }
    }
    return file;
}

char **get_lines(const char *path, int *file_lines, bool squeeze, bool *binerr)
{
    char *file;

    if (piped && (strcmp(path, "-") == 0))
        file = strdup(in);
    else{
        file = get_file(path, binerr);}
    if (file == NULL)
        return NULL;
    int lines = 0;
    size_t size = strlen(file);
    for (size_t i = 0; i < size; i++)
    {
        if (file[i] == '\n')
            lines++;
    }
    *file_lines = lines;
    char **line = (char **)malloc((lines + 1) * sizeof(char *));
    line[lines] = NULL;

    int it = 0;
    size_t last = 0;
    bool jump = false, prev = false;
    for (size_t i = 0; i < size; i++)
    {
        // 12 \n 3 \n 3 \n
        // 01 2  3 4  5 6
        if (file[i] == '\n')
        {
            if (squeeze)
            {
                prev = jump;
                if ((file[last] == '\n') && (i - last == 0))
                    jump = true;
                else
                    jump = false;
                if (prev && jump)
                {
                    last = i + 1;
                    continue;
                }
            }
            line[it] = (char *)calloc((i - last + 1), (sizeof(char)));
            memcpy(line[it], file + last, (i - last) * sizeof(char));
            last = i + 1;
            it++;
            continue;
        }
        if (i + 1 == size)
        {
            if (squeeze)
            {
                prev = jump;
                if ((file[last] == '\n') && (i - last == 0))
                    jump = true;
                else
                    jump = false;
                if (prev && jump)
                {
                    last = i + 1;
                    continue;
                }
            }
            line[it] = (char *)calloc((i - last + 2), (sizeof(char)));
            memcpy(line[it], file + last, (i - last + 1) * sizeof(char));
            line = (char **)realloc(line, (lines + 2) * sizeof(char *));
            line[lines + 1] = NULL;
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
        return;
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    exit(1);
}
