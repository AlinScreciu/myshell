#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <math.h>
typedef struct
{
    bool r, w, x; // read, write, execute 
} perm;
typedef struct
{
    perm *usr_perm;
    perm *grp_perm;
    perm *oth_perm;
    perm *special_perm;
    char op;
} mode_handler;
perm *make_perm(bool r, bool w, bool x)
{
    perm *prm = (perm *)malloc(sizeof(perm));
    prm->r = r;
    prm->w = w;
    prm->x = x;
    return prm;
}
perm *parse_perm(char mode)
{
    bool r = false, w = false, x = false;
    switch (mode)
    {
    case '1':
        x = true;
        break;
    case '2':
        w = true;
        break;
    case '3':
        x = true;
        w = true;
        break;
    case '4':
        r = true;
        break;
    case '5':
        r = true;
        x = true;
        break;
    case '6':
        r = true;
        w = true;
        break;
    case '7':
        w = true;
        r = true;
        x = true;
        break;
    }
    return make_perm(r, w, x);
}


mode_handler *make_mode_handler(char *modes, char op)
{
    mode_handler *handler = (mode_handler *)malloc(sizeof(mode_handler));
    handler->special_perm = parse_perm(modes[0]);
    handler->usr_perm = parse_perm(modes[1]);
    handler->grp_perm = parse_perm(modes[2]);
    handler->oth_perm = parse_perm(modes[3]);
    handler->op = op;
    return handler;
}
void perm_sub(perm* left, perm* right)
{
    printf("left: %d %d %d\n", left->r, left->w, left->x);
    printf("right: %d %d %d\n", right->r, right->w, right->x);
    left->r = left->r & !right->r;
    left->w = left->w & !right->w;
    left->x = left->x & !right->x;
    printf("left: %d %d %d\n", left->r, left->w, left->x);

}
char perm_tochar(perm prm)
{
    int ret;
    int remainder;
}
void substract(char* target, mode_handler* handler)
{
    
    struct stat stbf;

    stat(target, &stbf);

    char* old_perm = (char*)malloc(sizeof(char)*4 + 1);
    sprintf(old_perm, "%o", stbf.st_mode  );
    old_perm = old_perm + 2;
    printf("old: %s\n",old_perm);
    perm *special, *own, *grp, *oth;
    special = parse_perm(old_perm[0]);
    own = parse_perm(old_perm[1]);
    grp = parse_perm(old_perm[2]);
    oth = parse_perm(old_perm[3]);
    perm_sub(special, handler->special_perm);
    printf("special after sub: %d %d %d\n", special->r, special->w, special->x);

}
void handle_mode(char *mode, char *path)
{
    regex_t octal, symbolic;
    bool oct_f = false;
    regcomp(&octal, "^([-+=]*[0-7]+)$", REG_EXTENDED);
    oct_f = regexec(&octal, mode, 0, NULL, 0);

    if (!oct_f)
    {
        char op;
        int ops = 0;
        for (int i = 0; i < strlen(mode); i++)
            if (!isdigit(mode[i]))
            {
                ops++;
                op = mode[i];
            }
        printf("%d\n",ops);
        char *w_mode = malloc(sizeof(char) * strlen(mode+ops) + 2);
        sprintf(w_mode, "%.4lo", strtoul(mode+ops, NULL, 8));
        printf("%s\n", w_mode);
        mode_handler *handler;
        handler = make_mode_handler(w_mode, op);
        printf("sp: %d %d %d\n", handler->special_perm->r,handler->special_perm->w,handler->special_perm->x);
        printf("usr: %d %d %d\n", handler->usr_perm->r,handler->usr_perm->w,handler->usr_perm->x);
        printf("grp: %d %d %d\n", handler->grp_perm->r,handler->grp_perm->w,handler->grp_perm->x);
        printf("oth: %d %d %d\n", handler->oth_perm->r,handler->oth_perm->w,handler->oth_perm->x);

        if (op == '-')
            substract(path,handler);

    }
}
int main(int argc, char **argv)
{

    regex_t regex;
    int valid;
    char *mode;
    //  (for who)*(op([permission] or [group])) or [op][0-7]+
    // [ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=][0-7]+
    regcomp(&regex, "[ugoa]*([-+=]([rwxXst]*|[ugo]))+|[-+=]?[0-7]+", REG_EXTENDED);
    valid = regexec(&regex, argv[1], 0, NULL, 0);
    if (valid)
        return 1;

    mode = strdup(argv[1]);
    mode = strtok(mode, ",");
    while (mode != NULL)
    {
        handle_mode(mode, argv[2]);
        mode = strtok(NULL, ",");
    }

    regfree(&regex);
    return 0;
}