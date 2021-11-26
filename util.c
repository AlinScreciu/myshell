#include "util.h"
void envset(char** env, char* name, char* value)
{
    int i = 0;
    while(*(env+i)!=NULL)
    {
        if(strncmp(*(env+i),name,strlen(name))==0)
        {
            //printf("found value: %s with the name: %s\n",*(env+i),name);
            //printf("replacing\n");
            *(env + i) = malloc((strlen(value)+1)*sizeof(char));
            sprintf(*(env+i),"%s=%s",name,value);
            //printf("new value: %s with the name: %s\n",*(env+i),name);

        }
        i++;
    }
}

void remove_whitespace(char *str)
{
    //printf("\e[48;5;253m\e[38;5;0m%s\e[0m\n", str);
    char *rw_str = malloc(strlen(str) * sizeof(char) + 1);
    int it = 0;

    while (isspace(*(str + it)))
    {
        it++;
    }
    strcpy(rw_str, str + it);
    strcpy(str, rw_str);
    int rit = strlen(str) - 1;
    while (isspace(*(str + rit)))
    {
        rit--;
    }

    strncpy(rw_str, str, rit + 1);
    rw_str[rit + 1] = '\0';
    strcpy(str, rw_str);
}
int count_args(char *line, char* delim)
{
    int count = 0;
    char line_copy[strlen(line) + 1];
    strcpy(line_copy, line);

    char *it = strtok(line_copy, delim);

    while (it != NULL)
    {
        count++;
        it = strtok(NULL, delim);
    }
    free(it);
    return count;
}
void parse_args(char *line, char *args[], int argc)
{
    char line_copy2[strlen(line) + 1];
    strcpy(line_copy2, line);

    args[argc] = malloc(sizeof(char));
    args[argc] = NULL;

    char *token = strtok(line_copy2, " ");
    int i = 0;
    while (token != NULL)
    {
        remove_whitespace(token);
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;
        token = strtok(NULL, " ");
    }
    free(token);
}
void parse_pipe(char *line, char *args[], int argc)
{
    char line_copy2[strlen(line) + 1];
    strcpy(line_copy2, line);
    
    char *token = strtok(line_copy2, "|");
    int i = 0;
    while (token != NULL)
    {
        remove_whitespace(token);
        args[i] = malloc(strlen(token) + 1);
        strcpy(args[i], token);
        i++;
        token = strtok(NULL, "|");
    }
    free(token);
}