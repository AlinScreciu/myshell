#ifndef __MY_UTILS
#define __MY_UTILS
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
void remove_whitespace(char *);
void envset(char **, char *, char *);
int count_args(const char *, char *);
void parse_args(char *, char *[], int);
void parse_pipe(char *, char *[], int);
char *read_file(const char *, long *);
char** get_lines( const char* file, long size,int linesize[]);
int count_lines(const char* file, long size, bool*);
#endif