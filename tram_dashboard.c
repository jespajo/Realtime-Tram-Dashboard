#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

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

void dump_buffer(char* name) {
    int e;
    size_t len = strlen(name);
    for (size_t i = 0; i < len; i++) {
        e = name[i];
        printf("%-5d", e);
    }
    printf("\n\n");
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        if (!isalpha(name[i]) && !isdigit(name[i]) && (c != '_') && (c != ' '))
            c = '*';
        printf("%-5c", c);
    }
    printf("\n\n");
}

int main(int argc, char *argv[]){
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

	char buffer[255];
	while (1) {
		memset(buffer, 0, sizeof(buffer));
		int n = read(sockfd, buffer, sizeof(buffer));
		if (n < 0) {
			perror("Error reading from Server");
            exit(1);
        }
		dump_buffer(buffer);
	}

	return 0;
}
