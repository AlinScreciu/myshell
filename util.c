
#include "util.h"
void remove_whitespace(char *str)
{
    printf("\e[48;5;253m\e[38;5;0m%s\e[0m\n", str);
    char *rw_str = malloc(strlen(str) * sizeof(char) + 1);
    int it = 0;

    while (isspace(*(str + it)))
    {
        it++;
    }
    strcpy(rw_str, str + it);
    strcpy(str, rw_str);
    int rit = strlen(str)-1;
    while (isspace(*(str + rit)))
    {
        rit--;
    }
    
    strncpy(rw_str,str,rit+1);
    rw_str[rit+1] = '\0';
    strcpy(str, rw_str);
}
