/*
 * NTP Client
 * MIT License Copyright (c) 2021 Dave Asta
 *
 * Description: Connects to a specified NTP server to get the
 *              current UTC date and time, and then changes the
 *              system values (QDAY, QMONTH, QCENTURY, QYEAR, QTIME)
 * Usage: CALL PGM(NTPCLI) PARM('<ntpserver_url>' '<debug>')
 *    where: <ntpserver_url> is for example pool.ntp.org
 *           <debug> is 0=No, 1=Yes; to output extra information
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

/* ---- DEFINES ---- */
#define NTP_PORT     123
#define NTP_VERSION		4
#define NTP_MODE     3
#define TIME_OFFSET		2208988800L

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
* Receives two parameters:
*                          NTP server address
*                          display debug messages (0=No, 1=Yes)
*/
int main(int argc, char *argv[]){
    int i, udpsock, debug, century;
    unsigned int seconds_rcvd;
    time_t seconds_offseted;
    char ip_str[15], oscmd[64];
    struct hostent *hend;
    struct in_addr **addr_list;
    struct sockaddr_in server;
    struct tm *now;
    ntp_packet packet;

    /* Did we receive the mandatory parameters? */
   	if(argc < 3){
        printf("ERROR: missing parameters\n\n");
        printf("usage: CALL PGM(NTPCLI) PARM('<ntpserver_url>' '<debug>')\n");
        printf("  where: <debug> is 0=No, 1=Yes; for extra info.\n");
        printf("       e.g.: CALL PGM(NTPCLI) PARM('pool.ntp.org' '1')\n");
        return EXIT_FAILURE;
   	}

    debug = (int)argv[2];

   	/* Create a zeroed out variable from the
     * Packet Header Format structure specified in RFC 5905
     */
    memset(&packet, 0, sizeof(ntp_packet));

   	/* Change bits to configure mode client (NTP_MODE)
   	 * latest version (NTP_VERSION) and no warnings (0)
   	 * for leap indicator
     */
   	packet.header = htonl((NTP_VERSION << 27) | (NTP_MODE << 24));

    if(debug) printf("Getting IP from %s\n", argv[1]);

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

   	if(debug) printf("Creating UDP socket\n");

   	/* Create an UDP socket */
    udpsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
   	if(udpsock == -1){
      		printf("ERROR: couldn't create socket.\n");
      		return EXIT_FAILURE;
   	}

    if(debug) printf("Connecting to %s\n", ip_str);

    /* Connect to the server, using IP and Port */
   	server.sin_addr.s_addr = inet_addr(ip_str);
   	server.sin_family = AF_INET;
   	server.sin_port = htons(NTP_PORT);

    if(connect(udpsock, (struct sockaddr *)&server,
               sizeof(server)) < 0){
        printf("ERROR: couldn't connect to %s\n", ip_str);
        return EXIT_FAILURE;
   	}

   	if(debug) printf("Sending packet to server\n");

   	/* Send packet to server */
    if(send(udpsock, (char *)&packet, sizeof(packet), 0) < 0){
      		printf("ERROR: couldn't write to the socket.\n");
      		return EXIT_FAILURE;
   	}

   	if(debug) printf("Receiving response from server\n");

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
    if(debug){
        printf("%02d/%02d/%d %02d:%02d:%02d\n",
                now->tm_mday,
                now->tm_mon + 1,
                now->tm_year + 1900,
                now->tm_hour,
                now->tm_min,
             	now->tm_sec);
    }

   	/* Close the socket */
   	close(udpsock);

    /* Change QDAY */
    sprintf(oscmd, "CHGSYSVAL SYSVAL(QDAY) VALUE('%02d')",
              now->tm_mday);
    system(oscmd);
    if(debug) printf("%s\n", oscmd);

    /* Change QMONTH */
    sprintf(oscmd, "CHGSYSVAL SYSVAL(QMONTH) VALUE('%02d')",
              now->tm_mon + 1);
    system(oscmd);
    if(debug) printf("%s\n", oscmd);

    /* Change QCENTURY */
    if((now->tm_year + 1900) >= 2000) century = 1;
    else century = 0;
    sprintf(oscmd, "CHGSYSVAL SYSVAL(QCENTURY) VALUE('%d')", century);
    system(oscmd);
    if(debug) printf("%s\n", oscmd);

    /* Change QYEAR */
    sprintf(oscmd, "CHGSYSVAL SYSVAL(QYEAR) VALUE('%d')",
              (now->tm_year + 1900) % 100);
    system(oscmd);
    if(debug) printf("%s\n", oscmd);

    /* Change QTIME */
    sprintf(oscmd, "CHGSYSVAL SYSVAL(QTIME) VALUE('%02d%02d%02d')",
              now->tm_hour,
              now->tm_min,
              now->tm_sec);
    system(oscmd);
    if(debug) printf("%s\n", oscmd);

   	if(debug) printf("All done!\n");

   	return EXIT_SUCCESS;
}	

