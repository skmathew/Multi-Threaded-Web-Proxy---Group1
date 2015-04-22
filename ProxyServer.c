//129.120.151.96:9003 //CSE03
/*----------------------------------------------
 *	GROUP 1: Olawale Akinnawo, Sherin Mathew, Viivi Raina
 *	Project 2: Proxy Server
 *	CSCE 3530, 4/23/2015
 *----------------------------------------------*/
 
/* Multiple simultaneous clients handled by threads; */
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>	/* system type defintions */
#include <sys/socket.h>	/* network system functions */
#include <netinet/in.h>	/* protocol & struct definitions */

#define BACKLOG	5
#define BUF_SIZE	1024
#define LISTEN_PORT	9003

int threadCount = BACKLOG;
void *client_handler(void *sock_desc);
int blocked_websites(char *name);


/*----------------------------------------------
 *	MAIN
 *----------------------------------------------*/

int main(int argc, char *argv[]){
    int status, *sock_tmp;
    pthread_t a_thread;
    void *thread_result;
    
    struct sockaddr_in addr_mine;
    struct sockaddr_in addr_remote;
    int sock_listen;
    int sock_aClient;
    int addr_size;
    int reuseaddr = 1;
    
	// Create socket for listening
    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen < 0) {
        perror("socket() failed");
        exit(0);
    }
    
    memset(&addr_mine, 0, sizeof (addr_mine));
    addr_mine.sin_family = AF_INET;
    addr_mine.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_mine.sin_port = htons((unsigned short)LISTEN_PORT);
    
    // /* Enable the socket to reuse the address */
     if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
     	sizeof(int)) == -1) {
         perror("setsockopt");
     	close(sock_listen);
         exit(1);
     }
    
    
    status = bind(sock_listen, (struct sockaddr *) &addr_mine,
                  sizeof (addr_mine));
    if (status < 0) {
        perror("bind() failed");
        close(sock_listen);
        exit(1);
    }
    
	// Listen for connections
    status = listen(sock_listen, 5);
    if (status < 0) {
        perror("listen() failed");
        close(sock_listen);
        exit(1);
    }
    
    addr_size = sizeof(struct sockaddr_in);
	
	// Server feedback
    printf("Group 1 proxy server witing for client...\n");
	
    printf("waiting for a client\n");
    while(1) {
        if (threadCount < 1) {
            sleep(1);
        }
        
		// Accept connection
        sock_aClient = accept(sock_listen, (struct sockaddr *) &addr_remote,
                              &addr_size);
        if (sock_aClient == -1){
            close(sock_listen);
            exit(1);
        }
        
		// Create thread
        printf("Got a connection from %s on port %d\n",
               inet_ntoa(addr_remote.sin_addr),
               htons(addr_remote.sin_port));
        sock_tmp = malloc(1);
        *sock_tmp = sock_aClient;
        printf("thread count = %d\n", threadCount);
        threadCount--;
        status = pthread_create(&a_thread, NULL, client_handler,
                                (void *) sock_tmp);
        if (status != 0) {
            perror("Thread creation failed");
            close(sock_listen);
            close(sock_aClient);
            free(sock_tmp);
            exit(1);
        }
    }
    
    return 0;
}


/*----------------------------------------------
 *	CLIENT HANDLER
 *----------------------------------------------*/

void *client_handler(void *sock_desc) {
    int msg_size;
    char buf[BUF_SIZE];
    int sock = *(int*)sock_desc;
    char buffer[BUF_SIZE], text[80],buf_send[BUF_SIZE];
    char * pch;
    
    while ((msg_size = recv(sock, buffer, BUF_SIZE, 0)) > 0) {
        
        printf("The buffer message is: %s \nlast line\n", buffer);
        pch = strtok (buffer,"GET /");
        printf ("%s\n",pch);
        
        //checks to see of the website is on the blocked list
        if ((blocked_websites(pch))==0)
        {
            sprintf(buffer, "<html><title>Blocked website</title><body><p><big>This Website is Blocked!</big></p><body></html>\0");
            send(sock, buffer, sizeof(buffer), 0);
            close(sock);
            free(sock_desc);
            threadCount++;
            return;
        }
        
        
        else
        {
            sprintf(buffer, "<html><title>Good website</title><body><p><big>This Website is not Blocked!</big></p><body></html>\0");
            send(sock, buffer, sizeof(buffer), 0);
            close(sock);
            free(sock_desc);
            threadCount++;
            return;
        }
        
		// FOLLOWING COMMENTS ARE FROM THE PYTHON CODE:
		
		// Extract the filename from the given message
		
		//if( file is in cache ) {
			// Check whether the file exist in the cache
			// ProxyServer finds a cache hit and generates a response message
		//}
		//else if( file is not in cache ) {
			// Error handling for file not found in cache:
			//if( file not found ) {
				// Create a socket on the proxyserver
				// Connect to the socket to port 80
				// Create a temporary file on this socket and ask port 80 for the file requested by the client
				// Read the response into buffer
				// Create a new file in the cache for the requested file.
				// Also send the response in the buffer to client socket and the corresponding file in the cache
			//}
			//else {
				//HTTP response message for file not found
			//}
			// close client socket?
		//}
        
        
        
        
        //strcpy(buffer, "POST /www.yahoo.com HTTP/1.0");
        //msg_size =strlen(buffer);
        //msg_size=send(sock,buffer,msg_size,0);
    }
    
    close(sock);
    free(sock_desc);
    threadCount++;
}

int blocked_websites(char *pch)
{
    FILE *fptr;
    char temp[1024];
    int value;
    fptr=fopen("blocked.txt","r");
    if(fptr==NULL){
        printf("Error!");
        exit(1);
}
    while(fscanf(fptr, "%s\n", temp)!= EOF) {
        value = strcmp(temp, pch);
        if (value==0)
        {
            fclose(fptr);
            return 0;
        }
    }
    
    fclose(fptr);
    return 1;
    
    
}

