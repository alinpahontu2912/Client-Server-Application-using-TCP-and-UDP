#include <functions.h>

void send_messages(vector < struct client > * clients, char to_send[], string topic) {
    int check;
    for (auto client = ( * clients).begin(); client != ( * clients).end(); client++) {
        if (( * client).active == 1 && ( * client).topic_sf.count(topic) != 0) {
            uint32_t len = strlen(to_send);
            char dim[10];
            sprintf(dim, "%d", len);
            check = send(( * client).sock, dim, 10, 0);
            if (check < 0) {
                fprintf(stderr, "Sending error");
            }
            check = send(( * client).sock, to_send, BUFFER_LEN, 0);
            if (check < 0) {
                fprintf(stderr, "Sending error");
            }
        } else if (( * client).active == 0 && ( * client).topic_sf.count(topic) && ( * client).topic_sf[topic] == 1) {
            ( * client).oldmsg.push_back(to_send);
        }
    }
}

void subscribe_to_topic(vector < struct client > * clients, string topic, int sf, int sock) {
    for (auto client = ( * clients).begin(); client != ( * clients).end(); client++) {
        if (( * client).sock == sock) {
            if (( * client).topic_sf.count(topic) == 0) {
                ( * client).topic_sf.insert(pair < string, int > (string(topic), sf));
            } else {
                ( * client).topic_sf[topic] = sf;
            }
        }
    }
}

void unsubscribe_from_topic(vector < struct client > * clients, string topic, int sock) {
    for (auto client = ( * clients).begin(); client != ( * clients).end(); client++) {
        if (( * client).sock == sock) {
            if (( * client).topic_sf.count(topic) != 0) {
                ( * client).topic_sf.erase(topic);
            }
        }
    }
}

int new_fd(vector < struct client > clients, int tcp_sock, int udp_sock) {
    int fdmax = tcp_sock > udp_sock ? tcp_sock : udp_sock;
    for (auto client = clients.begin(); client != clients.end(); client++) {
        if (( * client).sock > fdmax && ( * client).active == 1) {
            fdmax = ( * client).sock;
        }
    }
    return fdmax;
}

void send_old_messages(struct client * wanted_client) {
    int check;
    for (auto msg = ( * wanted_client).oldmsg.begin(); msg != ( * wanted_client).oldmsg.end(); msg++) {
        uint32_t len = strlen(( * msg).c_str());
        char dim[10];
        sprintf(dim, "%d", len);
        check = send(( * wanted_client).sock, dim, 10, 0);
        if (check < 0) {
            fprintf(stderr, "Sending error");
        }
        check = send(( * wanted_client).sock, ( * msg).c_str(), BUFFER_LEN, 0);
        if (check < 0) {
            fprintf(stderr, "Sending error");
        }
    }
    ( * wanted_client).oldmsg.clear();
}