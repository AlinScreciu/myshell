#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include <unistd.h>
#define red "\033[31m"
#define green "\033[38;5;10m"
#define yellow "\033[33m"
#define blue "\033[34m"
#define magenta "\033[38;5;13m"
#define cyan "\033[36m"
#define reset "\033[0m"
int chsym(const char *, const char *);
int choct(const char *, char *);
void handle_op(const char *, mode_t, mode_t, char, bool, mode_t);
char *mode_to_string(mode_t mode);
int main(int argc, char **argv)
{ // name [mode] [file]+
    if (argc < 3)
    {
        fprintf(stderr, "Usage: chmod [mode] [file1 file2 ...]\n");
        exit(1);
    }
    char *mode = strdup(argv[1]);
    const int files = argc - 2;
    char *file[files];
    for (int i = 0; i < files; i++)
    {
        file[i] = strdup(argv[i + 2]);
        if (access(file[i], F_OK) != 0)
        {
            fprintf(stderr, "chmod: cannot access ‘%s’: No such file or directory\n", file[i]);
            exit(1);
        }
    }
    regex_t octal, symbolic;
    bool oct, sym;
    regcomp(&octal, "^([-+=]*[0-7]+)$", REG_EXTENDED);
    regcomp(&symbolic, "^([ugoa]*([-+=]([rwxXst]*|[ugo]))+)", REG_EXTENDED);
    oct = !regexec(&octal, mode, 0, NULL, 0);
    sym = !regexec(&symbolic, mode, 0, NULL, 0);
    if (!oct && !sym)
    {
        fprintf(stderr, "Mode must be of form [ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=]?[0-7]+\n");
        return 1;
    }
    for (int i = 0; i < files; i++)
    {
        if (oct)
            choct(file[i], mode);
        else if (sym)
            chsym(file[i], mode);
    }
    free(mode);
    for (int i = 0; i < files; i++)
    {
        free(file[i]);
    }
    return 0;
}
int chsym(const char *file, const char *modes)
{
    regex_t symbolic;
    regcomp(&symbolic, "^([ugoa]*([-+=]([rwxXst]*|[ugo])))$", REG_EXTENDED);
    char *mode = strtok(strdup(modes), ",");
    while (mode != NULL)
    {
        if (regexec(&symbolic, mode, 0, NULL, 0) != 0)
        {
            fprintf(stderr, "chmod: invalid mode: ‘%s’\n", modes);
            exit(1);
        };
        mode = strtok(NULL, ",");
    }
    mode = strtok(strdup(modes), ",");
    while (mode != NULL)
    {
        struct stat stbf;
        stat(file, &stbf);
        mode_t file_mode = stbf.st_mode;
        mode_t spe, ussr, grp, oth;
        spe = 07000 & file_mode;
        ussr = 0700 & file_mode;
        grp = 070 & file_mode;
        oth = 07 & file_mode;
        char op = '=';
        int targets = 0;
        char *target;
        char *perm_target;
        bool mask_f;
        for (size_t i = 0; i < strlen(mode); i++)
        {
            if (mode[i] == '+' || mode[i] == '-' || mode[i] == '=')
            {
                op = mode[i];
                if (i > 0)
                {
                    targets += i;
                }
            }
        }
        mode_t mask = umask(0000);
        umask(mask);
        if (targets > 0)
        {
            target = strndup(mode, targets);
        }
        else
        {
            target = strdup("a");
            mask_f = true;
        }

        perm_target = strdup(mode + targets + 1);

        //printf("target: %s\nperm: %s\nop: %c\n", target, perm_target, op);
        bool u = false, g = false, o = false;
        //printf("%o %o %o %o\n", spe, ussr, grp, oth);
        char* before = mode_to_string(spe | ussr | grp | oth);
        //printf("%s\n", before);
        for (size_t i = 0; i < strlen(target); i++)
        {
            for (size_t j = 0; j < strlen(perm_target); j++)
            {
                u = false;
                g = false;
                o = false;
                if (target[i] == 'a')
                {
                    u = true;
                    g = true;
                    o = true;
                }
                if (target[i] == 'u')
                    u = true;
                if (target[i] == 'g')
                    g = true;
                if (target[i] == 'o')
                    o = true;

                if (perm_target[j] == 'r')
                {
                    if (u)
                    {
                        if (op == '+')
                            ussr |= 0400;
                        else if (op == '-')
                            ussr &= ~0400;
                        else
                            ussr = 0400;
                    }
                    if (g)
                    {
                        if (op == '+')
                            grp |= 040;
                        else if (op == '-')
                            grp &= ~040;
                        else
                            grp = 040;
                    }
                    if (o)
                    {
                        if (op == '+')
                            oth |= 04;
                        else if (op == '-')
                            oth &= ~04;
                        else
                            oth = 04;
                    }
                }
                if (perm_target[j] == 'w')
                {
                    if (u)
                    {
                        if (op == '+')
                            ussr |= 0200;
                        else if (op == '-')
                            ussr &= ~0200;
                        else
                            ussr = 0200;
                    }
                    if (g)
                    {
                        if (op == '+')
                            grp |= 020;
                        else if (op == '-')
                            grp &= ~020;
                        else
                            grp = 020;
                    }
                    if (o)
                    {
                        if (op == '+')
                            oth |= 02;
                        else if (op == '-')
                            oth &= ~02;
                        else
                            oth = 02;
                    }
                }
                if (perm_target[j] == 'x')
                {
                    if (u)
                    {
                        if (op == '+')
                            ussr |= 0100;
                        else if (op == '-')
                            ussr &= ~0100;
                        else
                            ussr = 0100;
                    }
                    if (g)
                    {
                        if (op == '+')
                            grp |= 010;
                        else if (op == '-')
                            grp &= ~010;
                        else
                            grp = 010;
                    }
                    if (o)
                    {
                        if (op == '+')
                            oth |= 01;
                        else if (op == '-')
                            oth &= ~01;
                        else
                            oth = 01;
                    }
                }
                if (perm_target[j] == 'X')
                {
                    if ((ussr & 0100) || (grp & 010) || (oth & 01))
                    {
                        if (u)
                        {
                            if (op == '+')
                                ussr |= 0100;
                            else if (op == '-')
                                ussr &= ~0100;
                            else
                                ussr = 0100;
                        }
                        if (g)
                        {
                            if (op == '+')
                                grp |= 010;
                            else if (op == '-')
                                grp &= ~010;
                            else
                                grp = 010;
                        }
                        if (o)
                        {
                            if (op == '+')
                                oth |= 01;
                            else if (op == '-')
                                oth &= ~01;
                            else
                                oth = 01;
                        }
                    }
                }

                if (perm_target[j] == 's')
                {
                    if (u)
                    {
                        if (op == '+')
                            spe |= S_ISUID;
                        else if (op == '-')
                            spe &= ~S_ISUID;
                        else
                            spe = S_ISUID;
                    }
                    if (g)
                    {
                        if (op == '+')
                            spe |= S_ISGID;
                        else if (op == '-')
                            spe &= ~S_ISGID;
                        else
                            spe = S_ISGID;
                    }
                }
                if (perm_target[j] == 't')
                {
                    if (o)
                    {
                        if (op == '+')
                            spe |= S_ISVTX;
                        else if (op == '-')
                            spe &= ~S_ISVTX;
                        else
                            spe = S_ISVTX;
                    }
                }
                if (perm_target[j] == 'u')
                {
                    if (g)
                    {
                        if (op == '+')
                            grp |= ussr;
                        else if (op == '-')
                            grp &= ~ussr;
                        else
                            grp = ussr;
                    }

                    if (o)
                    {
                        if (op == '+')
                            oth |= ussr;
                        else if (op == '-')
                            oth &= ~ussr;
                        else
                            oth = ussr;
                    }
                }
                if (perm_target[j] == 'g')
                {
                    if (u)
                    {
                        if (op == '+')
                            ussr |= grp;
                        else if (op == '-')
                            ussr &= ~grp;
                        else
                            ussr = grp;
                    }
                    if (o)
                    {
                        if (op == '+')
                            oth |= grp;
                        else if (op == '-')
                            oth &= ~grp;
                        else
                            oth = grp;
                    }
                }
                if (perm_target[j] == 'o')
                {
                    if (u)
                    {
                        if (op == '+')
                            ussr |= oth;
                        else if (op == '-')
                            ussr &= ~oth;
                        else
                            ussr = oth;
                    }
                    if (g)
                    {
                        if (op == '+')
                            grp |= oth;
                        else if (op == '-')
                            grp &= ~oth;
                        else
                            grp = oth;
                    }
                }
            }
        }
        for (size_t i = 0; i < strlen(target); i++)
        {
            if (target[i] == 'a')
            {
                handle_op(file, file_mode, spe | ussr | grp | oth, '=', mask_f, mask);
            }
            if (target[i] == 'u')
            {
                handle_op(file, file_mode, spe | ussr | grp | oth, '=', mask_f, mask);
            }
            if (target[i] == 'g')
            {
                handle_op(file, file_mode, spe | ussr | grp | oth, '=', mask_f, mask);
            }
            if (target[i] == 'o')
            {
                handle_op(file, file_mode, spe | ussr | grp | oth, '=', mask_f, mask);
            }
        }
        //printf("%o %o %o %o\n", spe, ussr, grp, oth);
        char* after = mode_to_string(spe | ussr | grp | oth);
        //printf("%s\n",after );
        free(target);
        free(after);
        free(before);
        free(perm_target);
        mode = strtok(NULL, ",");
    }
    return 0;
}
int choct(const char *file, char *mode)
{
    struct stat stbf;
    stat(file, &stbf);
    mode_t file_mode = stbf.st_mode;
    char op = '=';
    int ops = 0;
    for (size_t i = 0; i < strlen(mode); i++)
        if (!isdigit(mode[i]))
        {
            ops++;
            op = mode[i];
        }
    mode_t omode = strtoul(mode + ops, NULL, 8);

    handle_op(file, file_mode, omode, op, false, 0);
    return 0;
}

void handle_op(const char *file, mode_t l_mode, mode_t r_mode, char op, bool maskf, mode_t mask)
{   
    if (op == '=')
    {

        chmod(file, r_mode);
    
    }
    if (op == '+')
    {
        chmod(file, l_mode | r_mode);
    }
    if (op == '-')
    {
        chmod(file, l_mode & ~r_mode);
    }
}
// this can also be used for ls -l
char *mode_to_string(mode_t mode)
{
    char *smode = malloc(sizeof(char) * 10 + 1);
    int i = 0;
    smode[i++] = (S_ISDIR(mode)) ? 'd' : '-';

    smode[i++] = (mode & S_IRUSR) ? 'r' : '-';
    smode[i++] = (mode & S_IWUSR) ? 'w' : '-';
    

    if ((mode & S_IXUSR) && (mode & S_ISUID))  
        smode[i++] = 's';
    else if (!(mode & S_IXUSR) && (mode & S_ISUID))
        smode[i++] = 'S';
    else if(mode & S_IXUSR)
        smode[i++] = 'x';
    else
        smode[i++] = '-';
    

    smode[i++] = (mode & S_IRGRP) ? 'r' : '-';
    smode[i++] = (mode & S_IWGRP) ? 'w' : '-';
    
    if ((mode & S_IXGRP) && (mode & S_ISGID))  
        smode[i++] = 's';
    else if (!(mode & S_IXGRP) && (mode & S_ISGID))
        smode[i++] = 'S';
    else if(mode & S_IXGRP)
        smode[i++] = 'x';
    else
        smode[i++] = '-';
    

    smode[i++] = (mode & S_IROTH) ? 'r' : '-';
    smode[i++] = (mode & S_IWOTH) ? 'w' : '-';
    
    if ((mode & S_IXOTH) && (mode & S_ISVTX))  
        smode[i++] = 's';
    else if (!(mode & S_IXOTH) && (mode & S_ISVTX))
        smode[i++] = 'S';
    else if(mode & S_IXOTH)
        smode[i++] = 'x';
    else
        smode[i++] = '-';
    smode[i] = '\0';
    return smode;
}