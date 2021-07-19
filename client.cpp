#include<functions.h>

int main(int argc, char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int server_sock;
    fd_set wanted_set, copy_set;
    struct sockaddr_in server_addr;
    int check;
    int fdmax;
    char buffer[CLIENT_BUFFER];
    if (argc < 4) {
        fprintf(stderr, "Main arguments");
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        fprintf(stderr, "TCP socket error");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[3]));
    inet_aton(argv[2], & server_addr.sin_addr);
    check = connect(server_sock, (struct sockaddr * ) & server_addr, sizeof(server_addr));
    if (check < 0) {
        fprintf(stderr, "Connect error");
    }
    // disabling Nagle's algorithm
    int no_delay_flag = 1;
    setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, (void * ) & no_delay_flag, sizeof(no_delay_flag));

    check = send(server_sock, argv[1], strlen(argv[1]) + 1, 0);
    if (check < 0) {
        fprintf(stderr, "unable to send");
    }
    // making sure the sets will be empty
    FD_ZERO( & wanted_set);
    FD_ZERO( & copy_set);
    // adding the sockets and the stdin
    FD_SET(0, & wanted_set);
    FD_SET(server_sock, & wanted_set);
    fdmax = server_sock;
    while (1) {
        copy_set = wanted_set;
        check = select(fdmax + 1, & copy_set, NULL, NULL, NULL);
        if (check < 0) {
            fprintf(stderr, "Select error");
        }
        if (FD_ISSET(0, & copy_set)) {
            memset(buffer, '\0', CLIENT_BUFFER);
            fgets(buffer, CLIENT_BUFFER - 1, stdin);
            // if exit is received, close down
            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            } else {
                // check for correct command 
                char buffer_copy[CLIENT_BUFFER];
                memset(buffer_copy, '\0', CLIENT_BUFFER);
                strcpy(buffer_copy, buffer);
                char * token = strtok(buffer, " ");
                // check if it is a valid subscribe command with required
                // topic len and store forward 
                if (strncmp(token, "subscribe", 9) == 0) {
                    token = strtok(NULL, " ");
                    if (token == NULL || strlen(token) > 50) {
                        fprintf(stderr, "Topic Problem");
                        break;
                    }
                    token = strtok(NULL, " ");
                    int sf = atoi(token);
                    if (sf != 0 && sf != 1) {
                        fprintf(stderr, "SF not good");
                        break;
                    }
                    check = send(server_sock, buffer_copy, CLIENT_BUFFER, 0);
                    if (check < 0) {
                        fprintf(stderr, "Not being able to send");
                        break;
                    }
                    // print message only if command is correct
                    printf("Subscribed to topic.\n");
                } else if (strncmp(token, "unsubscribe", 11) == 0) {
                    // check if it is a valid unsubscribe command with required
                    // topic len 
                    token = strtok(NULL, " ");
                    if (token == NULL || strlen(token) > 50) {
                        fprintf(stderr, "Topic Len");
                        break;
                    }
                    check = send(server_sock, buffer_copy, CLIENT_BUFFER, 0);
                    if (check < 0) {
                        fprintf(stderr, "Not being able to send");
                        break;
                    }
                    // print message only if command is correct
                    printf("Unsubscribed from topic.\n");
                }
            }
        }
        if (FD_ISSET(server_sock, & copy_set)) {
            // receive size first
            memset(buffer, '\0', BUFFER_LEN);
            check = recv(server_sock, buffer, 10, 0);
            if (check < 0) {
                fprintf(stderr, "Size not properly received");
                break;
            }
            uint32_t len = atoi(buffer);
            memset(buffer, '\0', BUFFER_LEN);
            // get wanted message content
            check = recv(server_sock, buffer, BUFFER_LEN, 0);
            if (check == 0) {
                break;
            }
            if (check < 0) {
                fprintf(stderr, "Message not properly received");
                break;
            }
            char buffer2[BUFFER_LEN];
            memset(buffer2, '\0', BUFFER_LEN);
            strncpy(buffer2, buffer, len);
            printf("%s\n", buffer2);
        }
    }

    close(server_sock);
    return 0;

}