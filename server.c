#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int socket_fd;
void *pthread_routine(void *);
void signal_handler(int signal_number);
int main(int argc, char *argv[])
{
    int port, new_socket_fd;
    struct sockaddr_in *address;
    pthread_attr_t threads_Attr;
    pthread_t th;
    int opt = 1;
    port = atoi(argv[1]);
    int opterr, binderr, listenerr, initerr, detacherr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        fprintf(stderr, "myshell_server: couldn't open socket on port '%d': ",port);
        perror(NULL);
        exit(1);
    }
    
    address = (struct sockaddr_in *)calloc(1, sizeof(struct sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    address->sin_addr.s_addr = INADDR_ANY;

    opterr = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (opterr < 0)
    {
        fprintf(stderr, "myshell_server: couldn't set socket options: ");
        perror(NULL);
        exit(1);
    }
    binderr = bind(socket_fd, (struct sockaddr *)address, sizeof(struct sockaddr));
    if (binderr < 0)
    {
        fprintf(stderr, "myshell_server: couldn't bind on port '%d': ", port);
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    listenerr = listen(socket_fd, 5);
    if (listenerr < 0)
    {
        fprintf(stderr, "myshell_server: listen failed: ");
        perror(NULL);
        exit(1);
    }
    initerr = pthread_attr_init(&threads_Attr);
    if (initerr < 0)
    {
        fprintf(stderr, "myshell_server: cannot init thread attributes: ");
        perror(NULL);
        exit(1);
    }
    detacherr = pthread_attr_setdetachstate(&threads_Attr, PTHREAD_CREATE_DETACHED);
    if (detacherr < 0)
    {
        fprintf(stderr, "myshell_server: failed to set detach state: ");
        perror(NULL);
        exit(1);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    while (1)
    {
        new_socket_fd = accept(socket_fd, NULL, NULL);
        if (new_socket_fd == -1)
        {
            fprintf(stderr, "myshell_server: failed aceptting client: ");
            perror(NULL);
            continue;
        }
        printf("Got new connection on socket fd '%d'\n", new_socket_fd);
        if (pthread_create(&th, &threads_Attr, pthread_routine, &new_socket_fd) != 0)
        {
            fprintf(stderr, "myshell_server: cannot create thread: ");
            perror(NULL);
            continue;
        }
    }
    return 0;
}

void *pthread_routine(void *arg)
{

    int new_socket_fd = *((int *)arg);

    while (1)
    {
        char *line;
        size_t line_len;
        char len_str[1000000] = {0};
        read(new_socket_fd, len_str, sizeof(len_str)); // read size of command
        line_len = (size_t)strtoul(len_str, NULL, 10);
        printf("next command size: %ld\n", line_len);
        line = (char *)malloc(sizeof(char *) * (line_len + 1)); // read command
        usleep(50);
        read(new_socket_fd, line, line_len);
        line[line_len] = '\0';
        printf("socket '%d': command: %s\n", new_socket_fd, line);
        if (strcmp(line, "exit") == 0)
        {
            free(line);
            printf("Exit at: %d", new_socket_fd);
            close(new_socket_fd);
            return NULL;
        }
        if (line[0] == '\0')
            break;
        char name[] = "myshell";
        char get_err[] = "2>&1";
        char *com = (char *)malloc((strlen(line) + 2 + strlen(name) + strlen(get_err) + 1) * sizeof(char)); // " 2>&1"
        sprintf(com, "%s '%s' %s", name, line, get_err);
        printf("%s\n", com);
        FILE *fp = popen(com, "r");
        char c;
        char *output = (char *)calloc(1, sizeof(char));
        int i = 0;
        while (fread(&c, 1, 1, fp) != 0)
        {
            output[i++] = c;
            output = (char *)realloc(output, (i + 1) * sizeof(char));
        }
        output[i] = '\0';
        sprintf(len_str, "%ld", strlen(output));
        printf("socket '%d': sending size of output: %s\n", new_socket_fd, len_str);
        write(new_socket_fd, len_str, strlen(len_str));
        usleep(50);
        printf("socket '%d': sending output\n", new_socket_fd);
        write(new_socket_fd, output, strlen(output));

        pclose(fp);
        free(line);
        free(output);
        free(com);
    }
    printf("\nExit at: %d\n", new_socket_fd);
    close(new_socket_fd);
    return NULL;
}

void signal_handler(int signal_number)
{
    close(socket_fd);
    exit(0);
}