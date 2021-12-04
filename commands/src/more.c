#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    // const char *usage = "Usage: more [-num] [-d] [-s]";
    // int num_flag = 0;
    // int opt;
    // int d_flag = 0;
    // int s_flag = 0;
    // while ((opt = getopt(argc, argv, "n:ds")) != -1)
    // {

    //     switch (opt)
    //     {
    //     case 'n':
    //         if (strcmp(optarg, "um") == 0)
    //         {
    //             num_flag = 1;
    //         }
    //         else
    //             printf("%s\n", usage);
    //         break;
    //     case 'd':
    //         d_flag = 1;
    //         break;
    //     case 's':
    //         s_flag = 1;
    //         break;
    //     }
    // };
    // printf("%d %d %d\n", num_flag, d_flag, s_flag);
    FILE *fptr;
    fptr = fopen("../../test.txt","w");
    fprintf(fptr,"yoo\n");
    fclose(fptr);
}
