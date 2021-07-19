#include <functions.h>

int main(int argc, char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int tcp_sock, udp_sock, port_number, new_tcp;
    fd_set wanted_set, copy_set;
    struct sockaddr_in tcp_addr, udp_addr, cli_addr;
    int check, fdmax, number_INT;
    char buffer[BUFFER_LEN];
    char to_send[BUFFER_LEN], topic[LEN_SIMPLE], content[1501], port[LEN_SIMPLE];
    char * token, aux[LEN_SIMPLE];
    double number_DOUBLE;
    bool go = true;
    socklen_t clilen;
    vector < struct client > clients;
    if (argc < 2) {
        fprintf(stderr, "Main arguments");
    }
    // TCP no delay flag
    int no_delay_flag = 1;
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        fprintf(stderr, "TCP socket error");
    }
    // disable Nagle algorithm
    setsockopt(tcp_sock, IPPROTO_TCP, TCP_NODELAY, & no_delay_flag,
        sizeof(no_delay_flag));

    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        fprintf(stderr, "UDP socket error");
    }

    port_number = atoi(argv[1]);
    if (port_number == 0) {
        fprintf(stderr, "Not proper port");
    }

    memset((char * ) & tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(port_number);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    check = bind(tcp_sock, (struct sockaddr * ) & tcp_addr, sizeof(struct sockaddr));
    if (check < 0) {
        fprintf(stderr, "Bind error");
    }

    memset((char * ) & udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port_number);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    check = bind(udp_sock, (struct sockaddr * ) & udp_addr, sizeof(struct sockaddr));
    if (check < 0) {
        fprintf(stderr, "Bind error");
    }

    check = listen(tcp_sock, MAX_CLIENTS);
    if (check < 0) {
        fprintf(stderr, "Listen error");
    }

    // making sure the sets are empty
    FD_ZERO( & wanted_set);
    FD_ZERO( & copy_set);

    // adding the sockets and the stdin
    FD_SET(0, & wanted_set);
    FD_SET(tcp_sock, & wanted_set);
    FD_SET(udp_sock, & wanted_set);
    fdmax = tcp_sock >= udp_sock ? tcp_sock : udp_sock;
    while (go) {
        // make temporary fd set and select
        copy_set = wanted_set;
        check = select(fdmax + 1, & copy_set, NULL, NULL, NULL);
        if (check < 0) {
            fprintf(stderr, "Select Error");
        }
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, & copy_set)) {
                if (i == 0) {
                    memset(buffer, '\0', BUFFER_LEN);
                    scanf("%s", buffer);
                    // exit received from stdin means stopping the server
                    if (strncmp(buffer, "exit", 4) == 0) {
                        go = false;
                        break;
                    }
                } else if (i == tcp_sock) {
                    int found = 0;
                    clilen = sizeof(cli_addr);
                    // accept new connection
                    new_tcp = accept(tcp_sock, (struct sockaddr * ) & cli_addr, & clilen);
                    if (new_tcp < 0) {
                        fprintf(stderr, "New tcp error");
                    }
                    memset(buffer, 0, BUFFER_LEN);
                    // receive the client id
                    check = recv(new_tcp, buffer, BUFFER_LEN - 1, 0);
                    if (check < 0) {
                        fprintf(stderr, "recv error");
                    }
                    // disabling Nagle algorithm
                    setsockopt(new_tcp, IPPROTO_TCP, TCP_NODELAY, & no_delay_flag,
                        sizeof(int));
                    //creating new client
                    struct client new_client;
                    memset(new_client.id, '\0', 11);
                    strncpy(new_client.id, buffer, 10);
                    new_client.sock = new_tcp;
                    new_client.active = 1;
                    // check if client already exists, is already connected or reconnected
                    for (auto client = clients.begin(); client != clients.end(); client++) {
                        if (strncmp(( * client).id, new_client.id, 10) == 0) {
                            found = 1;
                            if (( * client).active == 0) {
                                printf("New client %s connected from %s.\n", new_client.id,
                                    inet_ntoa(cli_addr.sin_addr));
                                ( * client).active = 1;
                                ( * client).sock = new_tcp;
                                FD_SET(new_tcp, & wanted_set);
                                fdmax = fdmax < new_tcp ? new_tcp : fdmax;
                                send_old_messages( & ( * client));
                            } else if (( * client).active == 1) {
                                printf("Client %s already connected.\n", new_client.id);
                                close(new_tcp);
                            }
                        }
                    }
                    // if client does not already exist, the new client is added
                    if (!found) {
                        printf("New client %s connected from %s.\n", new_client.id,
                            inet_ntoa(cli_addr.sin_addr));
                        clients.push_back(new_client);
                        FD_SET(new_tcp, & wanted_set);
                        fdmax = fdmax > new_tcp ? fdmax : new_tcp;
                    }
                } else if (i == udp_sock) {
                    memset(buffer, '\0', BUFFER_LEN);
                    clilen = sizeof(cli_addr);
                    // receive UDP message
                    check = recvfrom(udp_sock, buffer, BUFFER_LEN - 1, 0,
                        (sockaddr * ) & cli_addr, & clilen);
                    if (check < 0) {
                        fprintf(stderr, "Udp receive error");
                    }
                    // create the message
                    char * ip_addr = inet_ntoa(udp_addr.sin_addr);
                    memset(to_send, '\0', BUFFER_LEN);
                    memset(topic, '\0', LEN_SIMPLE);
                    memset(content, '\0', 1501);
                    memset(port, '\0', LEN_SIMPLE);
                    strncpy(topic, buffer, 49);
                    strncpy(content, buffer + LEN_SIMPLE, 1500);
                    strcat(to_send, ip_addr);
                    strcat(to_send, ":");
                    sprintf(port, "%d", htons(udp_addr.sin_port));
                    strcat(to_send, port);
                    strcat(to_send, " - ");
                    strncpy(topic, buffer, 50);
                    strcat(to_send, topic);
                    strcat(to_send, " - ");
                    int data = (int8_t) buffer[50];
                    if (data == 0) {
                        // extracting the wanted content from the udp buffer
                        memcpy( & number_INT, buffer + 52, sizeof(int));
                        number_INT = ntohl(number_INT);
                        strcat(to_send, "INT");
                        strcat(to_send, " - ");
                        if ((uint8_t) * (buffer + LEN_SIMPLE) == 1) {
                            number_INT *= -1;
                        }
                        memset(aux, '\0', LEN_SIMPLE);
                        sprintf(aux, "%d", number_INT);
                        strcat(to_send, aux);
                        to_send[strlen(to_send)] = '\0';
                        send_messages( & clients, to_send, string(topic));
                    } else if (data == 1) {
                        // extracting the wanted content from the udp buffer
                        number_DOUBLE = ntohs( * (uint16_t * )(buffer + LEN_SIMPLE));
                        strcat(to_send, "SHORT_REAL");
                        strcat(to_send, " - ");
                        number_DOUBLE /= 100;;
                        memset(aux, '\0', LEN_SIMPLE);
                        sprintf(aux, "%.2f", number_DOUBLE);
                        strcat(to_send, aux);
                        to_send[strlen(to_send)] = '\0';
                        send_messages( & clients, to_send, string(topic));
                    } else if (data == 2) {
                        // extracting the wanted content from the udp buffer
                        number_DOUBLE = ntohl( * (uint32_t * )(buffer + 52));
                        int power = ( * (int * )(buffer + 56));
                        while (power != 0) {
                            number_DOUBLE /= 10;
                            power--;
                        }
                        strcat(to_send, "FLOAT");
                        strcat(to_send, " - ");
                        if ((uint8_t) * (buffer + LEN_SIMPLE) == 1) {
                            number_DOUBLE *= -1;
                        }
                        memset(aux, '\0', LEN_SIMPLE);
                        sprintf(aux, "%f", number_DOUBLE);
                        strcat(to_send, aux);
                        to_send[strlen(to_send)] = '\0';
                        send_messages( & clients, to_send, string(topic));
                    } else if (data == 3) {
                        // extracting the wanted content from the udp buffer
                        strcat(to_send, "STRING");
                        strcat(to_send, " - ");
                        strcat(to_send, content);
                        to_send[strlen(to_send)] = '\0';
                        send_messages( & clients, to_send, string(topic));
                    }
                } else {
                    memset(buffer, 0, BUFFER_LEN);
                    check = recv(i, buffer, BUFFER_LEN - 1, 0);
                    if (check < 0) {
                        fprintf(stderr, "Recv error");
                    }
                    // if client exited
                    else if (check == 0) {
                        for (auto client = clients.begin(); client != clients.end(); client++) {
                            if (( * client).sock == i && ( * client).active == 1) {
                                ( * client).active = 0;
                                ( * client).sock = -1;
                                printf("Client %s disconnected.\n", ( * client).id);
                                FD_CLR(i, & wanted_set);
                                close(i);
                            }
                        }
                        // update fdmax
                        fdmax = new_fd(clients, tcp_sock, udp_sock);
                    } else {
                        // the client send a subscribe / unsubscribe request
                        token = strtok(buffer, " ");
                        if (strncmp(token, "subscribe", 9) == 0) {
                            token = strtok(NULL, " ");
                            memset(topic, 0, LEN_SIMPLE);
                            strncpy(topic, token, LEN_SIMPLE);
                            token = strtok(NULL, " ");
                            int sf = atoi(token);
                            subscribe_to_topic( & clients, string(topic), sf, i);
                        } else if (strncmp(token, "unsubscribe", 11) == 0) {
                            token = strtok(NULL, " ");
                            memset(topic, 0, LEN_SIMPLE);
                            strncpy(topic, token, 49);
                            topic[strlen(topic) - 1] = '\0';
                            unsubscribe_from_topic( & clients, string(topic), i);
                        }
                    }
                }
            }
        }
    }
    // closes all active connections, after "exit" is read
    for (int i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, & wanted_set)) {
            close(i);
        }
    }
    return 0;
}