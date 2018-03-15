#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PENDING 5
#define MAX_LINE 1024

struct sockaddr_in LocalTelnetAddress;
struct sockaddr_in SproxyAddress;
struct hostent *hp;
struct timeval tv;

int setUpTelnetConnection(int port){
    bzero((char *)&LocalTelnetAddress, sizeof(LocalTelnetAddress));
    LocalTelnetAddress.sin_family = AF_INET;
    LocalTelnetAddress.sin_addr.s_addr = INADDR_ANY;
    LocalTelnetAddress.sin_port = htons(port); // Change 5200
    
    // Create socket.
    int localTelnetSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (localTelnetSocket < 0) {
        fprintf(stderr, "ERROR: c telnet socket.\n");
        exit(1);
    }
    
    // Bind socket.
    if (bind(localTelnetSocket, (struct sockaddr *) &LocalTelnetAddress, sizeof(LocalTelnetAddress)) < 0) {
        fprintf(stderr, "ERROR: c telnet bind.\n");
        exit(1);
    }
    
    return localTelnetSocket;
}

int setUpSproxyConnection(char *host, int port)
{
    hp = gethostbyname(host);
    bzero((char *)&SproxyAddress, sizeof(SproxyAddress));
    SproxyAddress.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&SproxyAddress.sin_addr, hp->h_length);
    SproxyAddress.sin_port = htons(port);  //change 6200
    
 
    // Create the socket.
    int sproxySocket = socket(PF_INET, SOCK_STREAM, 0);
    if (sproxySocket < 0) {
        fprintf(stderr, "ERROR: sproxy socket.\n");
        exit(1);
    }
    
    return sproxySocket;
}

int main(int argc, char * argv[]){
    
   	fd_set readfds;
    int rv;
    int n, len = 0;
    struct timeval tv;
    
    char buf1[MAX_LINE];
	char buf2[MAX_LINE];
	
    socklen_t len1;
    
    int port1;
    int port2;
    int localTelnetSocket;
    int sproxySocket;
    if(argc == 4){
        port1 = atoi(argv[1]);
        port2 = atoi(argv[3]);
        localTelnetSocket = setUpTelnetConnection(port1);
        sproxySocket = setUpSproxyConnection(argv[2],port2);
    }
    else{
        fprintf(stderr, "illegal argc\n");
        exit(1);
    }
    
    listen(localTelnetSocket, MAX_PENDING);
	
    // Connect local telnet
    int localTelnet_s = accept(localTelnetSocket, (struct sockaddr *)&LocalTelnetAddress, &len1);
    if (localTelnet_s < 0) {
        perror("simplex-talk: accept");
        exit(1);
    }
    
    // Connect sproxy
    int sproxy_s = connect(sproxySocket, (struct sockaddr *) &SproxyAddress, sizeof(SproxyAddress));
    if (sproxy_s < 0) {
        fprintf(stderr, "ERROR connecting to sproxy.\n");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        // clear the set ahead of time
        FD_ZERO(&readfds);
        
        // add our descriptors to the set
        FD_SET(localTelnet_s, &readfds);
        FD_SET(sproxySocket, &readfds);
        
        // find the largest descriptor, and plus one.
        if (localTelnet_s > sproxySocket) n = localTelnet_s + 1;
        else n = sproxySocket +1;
        
        
        // wait until either socket has data ready to be recv()d (timeout 10.5 secs)
        tv.tv_sec = 10;
        tv.tv_usec = 500000;
		
        rv = select(n, &readfds, NULL, NULL, &tv);

		
        if (rv == -1) {
            perror("select"); // error occurred in select()
        } else if (rv == 0) {
            printf("Timeout occurred!  No data after 10.5 seconds.\n");
        } else {
            // one or both of the descriptors have data
            if (FD_ISSET(localTelnet_s, &readfds)) {	//telnet has data
                len = recv(localTelnet_s, buf1, sizeof(buf1), 0);
				if ( len <= 0 ){
					break;
				}
				send(sproxySocket, buf1, len, 0);		//send data to server
            }
            if (FD_ISSET(sproxySocket, &readfds)) {		//server has data
                len = recv(sproxySocket, buf2, sizeof(buf2), 0);
				if ( len <= 0 ){
					break;
				}
				send(localTelnet_s, buf2, len, 0);		//send data to telnet
            }
        }
    }
	close(sproxySocket);
	close(localTelnet_s);
	return 0;
}


