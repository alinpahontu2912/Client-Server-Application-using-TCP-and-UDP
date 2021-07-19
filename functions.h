#ifndef FUNCTION_H
#define FUNCTIONS_H
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/tcp.h>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

using namespace std;

#define BUFFER_LEN 2000
#define MAX_CLIENTS 500
#define SEND_SIZE 256
#define CLIENT_BUFFER 100
#define LEN_SIMPLE 51

struct client {
    int sock;
    uint8_t active;
    char id[11];
    vector < string > oldmsg;
    map < string, int > topic_sf;
};

// function that sends messages to the clients
void send_messages(vector < struct client > * clients, char to_send[], string topic);

// subscribes a client to a new topic
void subscribe_to_topic(vector < struct client > * clients, string topic, int sf, int sock);

//unsubscribe a client from a topic
void unsubscribe_from_topic(vector < struct client > * clients, string topic, int sock);

// updates fd
int new_fd(vector < struct client > clients, int tcp_sock, int udp_sock);

// sends messages received while client was disconnected
void send_old_messages(struct client * wanted_client);

#endif // FUNCTIONS_H