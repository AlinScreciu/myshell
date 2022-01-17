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
char *get_file(const char *path);
char **get_lines(const char *path, int *lines);
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
		"Star (*) indicates argument becomes new default.\n", stdout);
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
		 ".                       Repeat previous command\n", stdout);
    print_sep('-',79);
    printf("\n");
}
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

    for (int i = 1 ; i < argc; i++)
    {
        if (argv[i][0] == '-' && isdigit(argv[i][1]))
        {
            num = true;
            user_lines = atoi(argv[i]+1);
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
    char opt;
    int ps = 1;
    while(argv[ps++][0] == '-');
    ps--;
    bool exit = false, banner = false, shownbanner = false;
    int space_def;
    if (ps + 1 < argc)
        banner = true;
    int initial = rows;
    if (num) initial = user_lines;
    
    for (int i = ps; (i < argc) && !exit; i++)
    {
        
        int file_lines;
        bool shownbanner = false;
        char **line = get_lines(argv[i] , &file_lines);
        if (line == NULL)
            continue;
        // printf("\r\033[K");
        int edef = 1, zdef = rows, ddef = 11;
        bool z_def = true;
        
        if (banner && i == ps)
        {
            printf("::::::::::::::\n%s\n::::::::::::::\n", argv[i]);
            shownbanner = true;
            if (initial >( rows - 3)) initial = rows - 3;
        }
        int from = 0;
        if (i == ps)
        {
            if (x_to_xy(line, from, initial) == 0)
            {
                if (banner)
                    printf("\033[7m--More--(Next file: %s)", argv[i + 1]);

                for (int i = 0; line[i] != NULL; i++)
                    free(line[i]);
                free(line);
                continue;
            }
            from += initial;
            printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
        }
        char *numbuff = (char*)malloc(1*sizeof(char));
        bool getnum = false;
        int numit = 0;
        int last_from = 0, last_until = initial;
        while ((opt = getchar()) != EOF)
        {
            if (opt == '=')
            {
                printf("\r\033[K%d",last_from);
                continue;
            }
            if (opt == 12)
            {
                printf("\r\033[K");

                if (x_to_xy(line, last_from, last_until) == 0)
                {
                    if (banner)
                        printf("\033[7m--More--(Next file: %s)", argv[i + 1]);
                    for (int i = 0; line[i] != NULL; i++)
                        free(line[i]);
                    free(line);
                    continue;
                }
                printf("\033[7m--More--(%.f%%)\033[27m", ((double)last_from / file_lines) * 100);
            }
            if (opt == 'h')
            {
                printf("\r\033[K");

                usage();
                printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
                continue;
            }
            if (z_def) zdef = rows;
            if ((opt >= '0') && (opt <= '9'))
            {
                if (opt == '0' && numit == 0) continue;
                getnum = true;
                numbuff[numit++] = opt;
                numbuff = (char *)realloc(numbuff, sizeof(char) * (numit + 1));
                numbuff[numit] = '\0';
            }
            
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
                    jump = 1;
                if (opt == 4 || opt == '4')
                    jump = 11;
                if (banner && !shownbanner)
                {
                    printf("::::::::::::::\n%s\n::::::::::::::\n", argv[i]);
                    shownbanner = true;
                    if (jump > (rows - 3))
                        jump = rows - 3;
                }
                last_until = jump;
                if (x_to_xy(line, from, jump) == 0)
                {
                    if (banner && (i + 1 < argc))
                        printf("\033[7m--More--(Next file: %s)\033[27m", argv[i + 1]);
                    getnum = false;
                    free(numbuff);
                    numit = 0;
                    break;
                }
                from += jump;
                last_from = from;
                printf("\033[7m--More--(%.f%%)\033[27m", ((double)from / file_lines) * 100);
                getnum = false;
                free(numbuff);
                numbuff = (char *)malloc(1 * sizeof(char));
                numit = 0;
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
  
   while( y-- > 0 && line[x] != NULL)
    {
        printf("%s\n", line[x++]);
    }
    if (line[x] == NULL) return 0;
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
    char *file = (char *)calloc((st.st_size + 1), sizeof(char));

    if ((read(fd, file, sizeof(char) * st.st_size)) < 0)
    {
        free(file);
        fprintf(stderr, "more: cannot read %s: ", path);
        perror(NULL);
        return NULL;
    }
    return file;
}

char **get_lines(const char *path, int* file_lines)
{
    char *file =  get_file(path);
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
    for (size_t i = 0; i < size; i++)
    {
        // 12 \n 3 \n 3 \n
        // 01 2  3 4  5 6
        if (file[i] == '\n' )
        {
            line[it] = (char *)calloc((i - last + 1), (sizeof(char)) );
            memcpy(line[it], file + last, (i - last) * sizeof(char));
            last = i + 1;
            it++;
            continue;
        }
        if (i + 1 == size)
        {            
            line[it] = (char *)calloc((i - last + 2),(sizeof(char))  );
            memcpy(line[it], file + last, (i - last + 1) * sizeof(char));
            line = (char**) realloc(line, (lines + 2) *sizeof(char*));
            line[lines+1] = NULL;
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
