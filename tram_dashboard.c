#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

#include <assert.h>
#include <stdbool.h>
#include <limits.h>

/*
    The Tram data server (server.py) publishes messages over a custom protocol.

    These messages are either:

    1. Tram Passenger Count updates (MSGTYPE=PASSENGER_COUNT)
    2. Tram Location updates (MSGTYPE=LOCATION)

    It publishes these messages over a continuous byte stream, over TCP.

    Each message begins with a 'MSGTYPE' content, and all messages are made up in the format of [CONTENT_LENGTH][CONTENT]:

    For example, a raw Location update message looks like this:

        7MSGTYPE8LOCATION7TRAM_ID7TRAMABC5VALUE4CITY

        The first byte, '7', is the length of the content 'MSGTYPE'.
        After the last byte of 'MSGTYPE', you will find another byte, '8'.
        '8' is the length of the next content, 'LOCATION'.
        After the last byte of 'LOCATION', you will find another byte, '7', the length of the next content 'TRAM_ID', and so on.

        Parsing the stream in this way will yield a message of:

        MSGTYPE => LOCATION
        TRAM_ID => TRAMABC
        VALUE => CITY

        Meaning, this is a location message that tells us TRAMABC is in the CITY.

        Once you encounter a content of 'MSGTYPE' again, this means we are in a new message, and finished parsing the current message

    The task is to read from the TCP socket, and display a realtime updating dashboard all trams (as you will get messages for multiple trams, indicated by TRAM_ID), their current location and passenger count.

    E.g:

        Tram 1:
            Location: Williams Street
            Passenger Count: 50

        Tram 2:
            Location: Flinders Street
            Passenger Count: 22

    To start the server to consume from, please install python, and run python3 server.py 8081

    Feel free to modify the code below, which already implements a TCP socket consumer and dumps the content to a string & byte array
*/

#define MESSAGE_BUFFER_SIZE 512  // The maximum size of any message. Actually the real maximum is slightly lower because we insert a null byte after each content.

struct message {
    char    buffer[MESSAGE_BUFFER_SIZE];
    int     buffer_index;

    enum {
        UNKNOWN,
        LOCATION,
        PASSENGER_COUNT,
    }       type;

    // We could put these fields into structs to make this like a tagged union if they vary more depending on the message type.
    char   *tram_id;
    char   *value;
};

struct tram {
    // It's wasteful to store these IDs as strings if we know they're always numbers,
    // but until we know that for sure...
    char id[255];
    int  id_length;

    char location[255];
    int  location_length;

    int  passenger_count;
};

struct tram_array {
    struct tram *data;
    int          limit;
    int          count;
};

char *read_content(struct message *message, int sockfd)
{
    uint8_t length;
    int n = read(sockfd, &length, 1);
    if (n < 0) {
        perror("Error reading from the server");
        exit(1);
    }

    int *index = &message->buffer_index;

    if (*index + length + 2 >= MESSAGE_BUFFER_SIZE) { // The 2 bytes are the content-length byte (which we keep before the content) and a null byte (which we add after it).
        fprintf(stderr, "There's not enough space in the buffer.\n");
        exit(1);
    }

    // Add the content-length byte.
    message->buffer[*index] = length;
    *index += 1;

    char *content = &message->buffer[*index];

    // Read in a loop in case the OS gives us the data on the socket before we've read the content fully.
    int read_count = 0;
    while (read_count < length) {
        int n = read(sockfd, content+read_count, length-read_count);
        if (n <= 0) {
            perror("Error reading from the server");
            exit(1);
        }
        read_count += n;
    }
    *index += length;

    // Add a null byte for the small convenience of being able to use strcmp().
    content[length] = '\0';
    *index += 1;

    return content;
}

void read_message(struct message *message, int sockfd)
{
    memset(message, 0, sizeof(*message));

    char *msgtype_key = read_content(message, sockfd);
    if (strcmp(msgtype_key, "MSGTYPE")) {
        fprintf(stderr, "Unexpected content. Expected \"MSGTYPE\". Got \"%s\".\n", msgtype_key);
        exit(1);
    }

    char *msgtype_val = read_content(message, sockfd);
    if (!strcmp(msgtype_val, "LOCATION")) {
        message->type = LOCATION;
    } else if (!strcmp(msgtype_val, "PASSENGER_COUNT")) {
        message->type = PASSENGER_COUNT;
    } else {
        fprintf(stderr, "Unexpected content. Expected a message type. Got \"%s\".\n", msgtype_val);
        exit(1);
    }

    char *tram_id_key = read_content(message, sockfd);
    if (strcmp(tram_id_key, "TRAM_ID")) {
        fprintf(stderr, "Unexpected content. Expected \"TRAM_ID\". Got \"%s\".\n", tram_id_key);
        exit(1);
    }
    message->tram_id = read_content(message, sockfd);

    char *value_key = read_content(message, sockfd);
    if (strcmp(value_key, "VALUE")) {
        fprintf(stderr, "Unexpected content. Expected \"VALUE\". Got \"%s\".\n", value_key);
        exit(1);
    }
    message->value = read_content(message, sockfd);
}

struct tram *add_tram(struct tram_array *array, char *tram_id)
{
    if (array->count == array->limit) {
        array->limit = (array->limit) ? 2*array->limit : 4;
        array->data  = realloc(array->data, array->limit*sizeof(array->data[0])); //|Todo: Check if NULL?
    }

    struct tram *tram = &array->data[array->count];
    memset(tram, 0, sizeof(*tram));
    array->count += 1;

    // Using strlen() here is kind of dumb because this information is in the message and probably accessible in the byte just before the string. But we can't guarantee that the tram_id argument passed to this function is a pointer into the message data and not just some random string. We could pass a tram_id_length argument to this function explicitly... but this is fine.
    tram->id_length = strlen(tram_id);
    memcpy(tram->id, tram_id, tram->id_length);

    // Initialise these values to -1 to show they've never been set.
    tram->location_length = -1;
    tram->passenger_count = -1;

    return tram;
}

struct tram *find_tram(struct tram_array *array, char *tram_id)
{
    // |Speed: This is obviously very slow... If we expect more than a few trams, we should use a hash table or maybe just keep the array sorted so we can do a binary search.
    for (int i = 0; i < array->count; i++) {
        struct tram *tram = &array->data[i];
        if (!strcmp(tram->id, tram_id))  return tram;
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,"No port provided\n");
        exit(1);
    }

    char *host = "127.0.0.1";
    char *port = argv[1];

    struct sockaddr serv_addr;
    {
        struct addrinfo hints = {0};
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        struct addrinfo *addrinfo;
        int result = getaddrinfo(host, port, &hints, &addrinfo);
        if (result != 0) {
            fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(result));
            exit(1);
        }

        serv_addr = *addrinfo->ai_addr;

        freeaddrinfo(addrinfo);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Socket failed");
        exit(1);
    }

    int result = connect(sockfd, &serv_addr, sizeof(serv_addr));
    if (result < 0) {
        perror("Connection failed");
        exit(1);
    }

    struct tram_array trams = {0};
    struct message *message = malloc(sizeof(*message));

    while (true) {
        read_message(message, sockfd);

        struct tram *tram = find_tram(&trams, message->tram_id);
        if (!tram) {
            tram = add_tram(&trams, message->tram_id);
        }

        switch (message->type) {
            case LOCATION: {
                tram->location_length = (uint8_t)message->value[-1];
                memcpy(tram->location, message->value, tram->location_length);
                tram->location[tram->location_length] = '\0';
                break;
            }
            case PASSENGER_COUNT: {
                char *end = NULL;
                long number = strtol(message->value, &end, 10);

                bool valid = true;
                valid &= (0 <= number);
                valid &= (number <= INT_MAX); //|Fixme: Not sure 2 billion is the right place to draw the line here...
                valid &= (*message->value != '\0');
                valid &= (*end == '\0');
                valid &= (end - message->value == (uint8_t)message->value[-1]);
                if (!valid) {
                    fprintf(stderr, "Unexpected passenger count for %s: \"%s\"\n", tram->id, message->value);
                    exit(1);
                }

                tram->passenger_count = number;
                break;
            }
            default:
                fprintf(stderr, "Unexpected message type: %d\n", message->type);
                exit(1);
        }

        // Print the dashboard. We'd normally prefer to buffer the output ourselves, but for now we'll do this without importing a whole string-builder module.

        printf("\x1b[H\x1b[2J");  // Screen-clearing ANSI terminal codes.
        for (int i = 0; i < trams.count; i++) {
            struct tram *tram = &trams.data[i];
            printf("\n");
            printf("    %s:\n", tram->id);
            if (tram->location_length >= 0) {
                printf("        Location: %s\n", tram->location);
            }
            if (tram->passenger_count >= 0) {
                printf("        Passenger Count: %d\n", tram->passenger_count);
            }
        }
    }

    free(message);
    free(trams.data);
    return 0;
}
