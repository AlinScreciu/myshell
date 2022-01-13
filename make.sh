gcc server.c -g -pthread -o server -Wall -Wextra -Wpedantic -Wno-unused-parameter 
gcc client.c -g -o client -Wall -Wextra -Wpedantic -lreadline -Wno-unused-parameter 