#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
int main(int argc, char **argv, char **envp)
{
    char* cwd = malloc(PATH_MAX);
    getcwd(cwd, PATH_MAX);
    printf("%s\n",cwd);
    return 0;
}