#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
void signal_handler(int signal_number);
int main(int argc, char *argv[]) {
    
    char server_name[256] = { 0 };
  
    struct sockaddr_in *addr_host;
    struct hostent *host_server;
    int port, socket_fd, connerr;
    
    strncpy(server_name, argv[1], 255);
    port =  atoi(argv[2]);

    host_server = gethostbyname(server_name);

    addr_host = (struct sockaddr_in*) calloc(1, sizeof(struct sockaddr_in));
    addr_host->sin_family = AF_INET;
    addr_host->sin_port = htons(port);
    memcpy(&addr_host->sin_addr.s_addr, host_server->h_addr, host_server->h_length);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0 )
    {
        fprintf(stderr, "myshell_client: couldn't open socket on port '%d': ",port);
        perror(NULL);
        exit(1);
    }

    connerr = connect(socket_fd, (struct sockaddr *)addr_host, sizeof (struct sockaddr));
    if (connerr < 0 ) {
		fprintf(stderr, "myshell_client: couldn't connect to socket on port '%d': ",port);
        perror(NULL);
        exit(1);
	}
    using_history();
    signal(SIGINT, signal_handler);

    while (1){

        char *input = readline("myshell$ ");
        if (input == NULL)
        {
            printf("\n");
            continue;
        }
        if (strcmp(input, "") == 0)
        {
            continue;
        }
        add_history(input);
        char len_str[1000000] = {0};
        sprintf(len_str, "%ld", strlen(input));
        write(socket_fd,len_str, strlen(len_str));
        usleep(50);
        write(socket_fd, input, strlen(input));
        if (strcmp(input,"exit") ==0)
        {
            free(input);
            break;
        }
        size_t res_size;

        read(socket_fd, len_str, sizeof(len_str));
        res_size = strtoul(len_str, NULL, 10);
        char res[res_size];
        read(socket_fd, res, res_size);
        res[res_size] = '\0';
        printf("%s", res);

        free(input);
    }

    free(addr_host);
    close(socket_fd);
}
void signal_handler(int signal_number)
{
    printf("\nmyshell$ ");
}