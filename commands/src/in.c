#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

int main() {
    int i = 0;
    char c;
    char *s = malloc(sizeof(char));
    while( ( c = getchar()) != EOF)
    {   
        s[i++] = c;
        if (realloc(s, sizeof(char)*i) == NULL) return 1;
    }
    if (realloc(s, sizeof(char)*(i+2)) == NULL) return 1;
    s[i++] = '\n';
    s[i] = '\0';
    free(s);
    return 0;
}