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
    char op, target;
    bool default_target;
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
    handler->target = 'a';
    handler->default_target = true;
    return handler;
}
void perm_sub(perm* left, perm* right)
{

    left->r = left->r & !right->r;
    left->w = left->w & !right->w;
    left->x = left->x & !right->x;
}
void perm_add(perm* left, perm* right)
{

    left->r = left->r | right->r;
    left->w = left->w | right->w;
    left->x = left->x | right->x;
}
char perm_tochar(perm prm)
{
    int iperm=0;
    if (prm.r) iperm+=4;
    if (prm.w) iperm+=2;
    if (prm.x) iperm+=1;
    return iperm + 48;
}
void op_handler(char* target, mode_handler* handler)
{
    char op = handler->op;
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

    if (op == '-')
    {
        perm_sub(special, handler->special_perm);
        perm_sub(own, handler->usr_perm);
        perm_sub(grp, handler->grp_perm);
        perm_sub(oth, handler->oth_perm);
    }
    if (op == '+')
    {
        perm_add(special, handler->special_perm);
        perm_add(own, handler->usr_perm);
        perm_add(grp, handler->grp_perm);
        perm_add(oth, handler->oth_perm);
    }

    char final_perm[] = { perm_tochar(*special), perm_tochar(*own), perm_tochar(*grp), perm_tochar(*oth),'\0'};
    printf("final: %o\n",(mode_t)strtoul(final_perm, NULL, 8));

    chmod(target, (mode_t)strtoul(final_perm, NULL, 8));

}
void handle_mode(char *mode, char *path)
{
    regex_t octal, symbolic;
    bool oct_f = false;
    regcomp(&octal, "^([-+=]*[0-7]+)$", REG_EXTENDED);
                    //  target=a  op=0 (perm)
                    
    regcomp(&symbolic, "[ugoa]*([-+=]([rwxXst]*|[ugo]))+", REG_EXTENDED);
    oct_f = regexec(&octal, mode, 0, NULL, 0); // 0 if valid

    if (!oct_f)
    {
        
        char op = '=';
        int ops = 0;
        for (size_t i = 0; i < strlen(mode); i++)
            if (!isdigit(mode[i]))
            {
                ops++;
                op = mode[i];
            }
        // 
        char *w_mode = malloc(sizeof(char) * strlen(mode+ops) + 2);
        sprintf(w_mode, "%.4lo", strtoul(mode+ops, NULL, 8));
        printf("new: %s\n", w_mode);
        mode_handler *handler;
        handler = make_mode_handler(w_mode, op);


        if (op != '=')
            op_handler(path, handler);
        else
            chmod(path, (mode_t) (strtoul(mode, NULL,8)));

    }
    else
    {
        if (regexec(&symbolic, mode, 0, NULL, 0) != 0)
            EXIT_FAILURE;
        perm *special, *usr, *grp, *oth;
        char default_target = 'a';
        char* target = malloc(sizeof(char)*5);
        char* nparsed_perm = malloc(sizeof(char)*5);
        char op = '=';
        struct stat stbf;
        char *old_perm = malloc(sizeof(char) * 8);
        stat(path, &stbf);
        sprintf(old_perm, "%o", stbf.st_mode);
        old_perm = old_perm + 2;
        special = parse_perm(old_perm[0]);
        usr = parse_perm(old_perm[1]);
        grp = parse_perm(old_perm[2]);
        oth = parse_perm(old_perm[3]);
        char final_perm[] = { perm_tochar(*special), perm_tochar(*usr), perm_tochar(*grp), perm_tochar(*oth),'\0'};
        printf("final: %o\n",(mode_t)strtoul(final_perm, NULL, 8));


        printf("to parse: %s\n",mode);
        target = strtok(strdup(mode),"+=-");
        nparsed_perm = strtok(NULL, "+=-");
        printf("target: %s\nperm: %s\n",target,nparsed_perm);
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
    mode = strdup(argv[1]);
    mode = strtok(mode, ",");
    while (mode != NULL)
    {
        
        valid = regexec(&regex, mode, 0, NULL, 0);
        if (valid)
            return 1;
        handle_mode(mode, argv[2]);
        mode = strtok(NULL, ",");
    }

    regfree(&regex);
    return 0;
}