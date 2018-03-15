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

struct sockaddr_in TelnetDaemonAddress;
struct sockaddr_in CproxyAddress;
struct hostent *hp;
struct timeval tv;

int setUpCproxyConnection(int port)
{
    bzero((char *)&CproxyAddress, sizeof(CproxyAddress));
    CproxyAddress.sin_family = AF_INET;
    CproxyAddress.sin_addr.s_addr = INADDR_ANY;
    CproxyAddress.sin_port = htons(port);
    
    // Create  socket.
    int cproxySocket = socket(PF_INET, SOCK_STREAM, 0);
    if (cproxySocket < 0) {
        fprintf(stderr, "ERROR: cproxy socket.\n");
        exit(EXIT_FAILURE);
    }
    
    // Bind socket
    if (bind(cproxySocket, (struct sockaddr *) &CproxyAddress, sizeof(CproxyAddress)) < 0) {
        fprintf(stderr, "ERROR: cproxy bind.\n");
        exit(EXIT_FAILURE);
    }
    
    return cproxySocket;
}

int setUpTelnetDaemonConnection()
{
	hp = gethostbyname("127.0.0.1");
    bzero((char *)&TelnetDaemonAddress, sizeof(TelnetDaemonAddress));
    TelnetDaemonAddress.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&TelnetDaemonAddress.sin_addr, hp->h_length);
    TelnetDaemonAddress.sin_port = htons(23);  //change 6200
    
 
    // Create the socket.
    int telnetDaemonSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (telnetDaemonSocket < 0) {
        fprintf(stderr, "ERROR: telnetDaemonSocket.\n");
        exit(1);
    }
    
    return telnetDaemonSocket;
}

int main(int argc, char * argv[]){
    
    fd_set readfds;
    int rv;
    int n, len = 0;
    struct timeval tv;
    
    char buf1[MAX_LINE];
	char buf2[MAX_LINE];
    socklen_t len1;
	
    int port;
    int cproxySocket;
    int telnetDaemonSocket;
   
    
    if(argc == 2){
        port = atoi(argv[1]);
        cproxySocket = setUpCproxyConnection(port);
        telnetDaemonSocket = setUpTelnetDaemonConnection();
    }
    else{
        fprintf(stderr, "illegal argc\n");
        exit(1);
    }
    
    // Connect cproxy
    listen(cproxySocket, MAX_PENDING);
   // int len = sizeof(CproxyAddress);
    int cproxy_s = accept(cproxySocket, (struct sockaddr *) &CproxyAddress, &len1);
	
	
    if (cproxy_s < 0) {
        fprintf(stderr, "ERROR: cproxy accept.\n");
        exit(1);
    }
    
    // Connect telnet daemon
    if (connect(telnetDaemonSocket, (struct sockaddr *) &TelnetDaemonAddress, sizeof(TelnetDaemonAddress)) < 0) {
        fprintf(stderr, "ERROR: telnet daemon connect.\n");
        exit(1);
    }
    
    while(1){
        // clear the set ahead of time
        FD_ZERO(&readfds);

        // add our descriptors to the set
        FD_SET(cproxy_s, &readfds);
        FD_SET(telnetDaemonSocket, &readfds);
        
        
        // find the largest descriptor, and plus one.
        
        if (cproxy_s > telnetDaemonSocket) n = cproxy_s + 1;
        else n = telnetDaemonSocket +1;

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
            if (FD_ISSET(cproxy_s, &readfds)) {			//data from client
                len = recv(cproxy_s, buf1, sizeof(buf1), 0);
				if ( len <= 0 ){
					break;
				}
                send(telnetDaemonSocket, buf1, len, 0);	//send data to daemon
            }
            if (FD_ISSET(telnetDaemonSocket, &readfds)) {	//data from daemon
                len = recv(telnetDaemonSocket, buf2, sizeof(buf2), 0);
				if ( len <= 0 ){
					break;
				}
                send(cproxy_s, buf2, len, 0);		//send data to client		
            }
        }
    }
	close(cproxySocket);
	close(telnetDaemonSocket);
    return 0;
}




