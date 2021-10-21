/*
 * NTP Client
 * MIT License Copyright (c) 2021 Dave Asta
 *
 * RFC 5905 Packet Header Format
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |LI | VN  |Mode |    Stratum     |     Poll      |  Precision   |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Root Delay                            |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                         Root Dispersion                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                          Reference ID                         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   +                     Reference Timestamp (64)                  +
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   +                      Origin Timestamp (64)                    +
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   +                      Receive Timestamp (64)                   +
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   +                      Transmit Timestamp (64)                  +
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   .                    Extension Field 1 (variable)               .
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   .                    Extension Field 2 (variable)               .
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                          Key Identifier                       |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                            dgst (128)                         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * timestamps represent time since 1900
 *   the offset for Unix (1970) would be timestamp + 2208988800 seconds
 *
 */

/* ---- INCLUDES ---- */
#include <stdio.h>
#include <stdlib.h>      /* EXIT_SUCCESS, EXIT_FAILURE */
#include <string.h>      /* strlen(), strcpy() */
#include <sys/socket.h>
#include <arpa/inet.h>   /* inet_addr(), htons() */
#include <netdb.h>       /* gethostbyname() */
#include <unistd.h>      /* close() */
#include <time.h>        /* struct tm */
#include <xxdtaa.h>      /* QXXCHGDA() */

/* ---- DEFINES ---- */
#define NTP_PORT     123
#define NTP_VERSION		4
#define NTP_MODE     3
#define TIME_OFFSET		2208988800L

#define DEBUG 1

/* ---- STRUCTURES ---- */
typedef struct{
    unsigned int seconds;
    unsigned int fraction;
}ntp_timestamp;

typedef struct{        /* RFC 5905 - Packet Header Format */
    unsigned int header;   /* LI, VN, Mode, Stratum, Poll, Precision */
    unsigned int rootdelay;/* root delay */
    unsigned int rootdisp; /* root dispersion */
    unsigned int refid;    /* reference ID */
    ntp_timestamp reftime; /* reference time */
    ntp_timestamp org;     /* origin timestamp */
    ntp_timestamp rec;     /* receive timestamp */
    ntp_timestamp xmt;     /* transmit timestamp */
}ntp_packet;

/*
* Main function
*
* Receives one parameter: NTP server address
*/
int main(int argc, char *argv[]){
    int i, udpsock;
    unsigned int seconds_rcvd;
    time_t seconds_offseted;
    char ip_str[15];
    struct hostent *hend;
    struct in_addr **addr_list;
    struct sockaddr_in server;
    struct tm *now;
    ntp_packet packet;
    _DTAA_NAME_T dtaname = {"*LDA      ", "          "};
    char newdata[14];

    /* Did we receive the mandatory parameter? */
   	if(argc < 2){
        printf("ERROR: missing parameter\n\n");
        printf("usage: ntpcli <ntp_server_address>\n");
        printf("       e.g.: ntpcli de.pool.ntp.org\n\n");
        return EXIT_FAILURE;
   	}

   	/* Create a zeroed out variable from the
     * Packet Header Format structure specified in RFC 5905
     */
    memset(&packet, 0, sizeof(ntp_packet));

   	/* Change bits to configure mode client (NTP_MODE)
   	 * latest version (NTP_VERSION) and no warnings (0)
   	 * for leap indicator
     */
   	packet.header = htonl((NTP_VERSION << 27) | (NTP_MODE << 24));

    if(DEBUG) printf("Getting IP from %s\n", argv[1]);

   	/* Get the IP address of the hostname that came in argv */
   	if((hend = gethostbyname(argv[1])) == NULL){
      		printf("ERROR: gethostbyname failed.\n");
      		return EXIT_FAILURE;
    	}

   	addr_list = (struct in_addr **) hend->h_addr_list;

   	/* The IP is the first addr */
   	for(i=0; addr_list[i] != NULL; i++){
        strcpy(ip_str, inet_ntoa(*addr_list[i]));
   	}

   	if(DEBUG) printf("Creating UDP socket\n");

   	/* Create an UDP socket */
    udpsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   	if(udpsock == -1){
      		printf("ERROR: couldn't create socket.\n");
      		return EXIT_FAILURE;
   	}

    if(DEBUG) printf("Connecting to %s\n", ip_str);

    /* Connect to the server, using IP and Port */
   	server.sin_addr.s_addr = inet_addr(ip_str);
   	server.sin_family = AF_INET;
   	server.sin_port = htons(NTP_PORT);

    if(connect(udpsock, (struct sockaddr *)&server,
               sizeof(server)) < 0){
        printf("ERROR: couldn't connect to %s\n", ip_str);
      		return EXIT_FAILURE;
   	}

   	if(DEBUG) printf("Sending packet to server\n");

   	/* Send packet to server */
    if(send(udpsock, (char *)&packet, sizeof(packet), 0) < 0){
      		printf("ERROR: couldn't write to the socket.\n");
      		return EXIT_FAILURE;
   	}

   	if(DEBUG) printf("Receiving response from server\n");

   	/* Receive response from server */
    if(recv(udpsock, (char *)&packet, sizeof(packet), 0) < 0){
      		printf("ERROR: couldn't read from the socket.\n");
      		return EXIT_FAILURE;
   	}
	
   	/* RFC 5905
   	 * Transmit Timestamp (xmt): Time at the server when the
     * response left for the client
     */
   	packet.xmt.seconds = ntohl(packet.xmt.seconds);
   	seconds_offseted = (time_t)(packet.xmt.seconds - TIME_OFFSET);
   	now = localtime(&seconds_offseted);
    if(DEBUG){
        printf("%02d/%02d/%d %02d:%02d:%02d\n",
                now->tm_mday,
                now->tm_mon + 1,
                now->tm_year + 1900,
                now->tm_hour,
                now->tm_min,
             			now->tm_sec);
    }

    /* Put received date/time in *LDA */
    sprintf(newdata, "%02d%02d%d%02d%02d%02d",
            now->tm_mday,
            now->tm_mon + 1,
            now->tm_year + 1900,
            now->tm_hour,
            now->tm_min,
         			now->tm_sec);
    QXXCHGDA(dtaname, 1, 14, newdata);

   	/* Close the socket */
   	close(udpsock);

   	if(DEBUG) printf("All done!\n");
   	
   	return EXIT_SUCCESS;
}	
