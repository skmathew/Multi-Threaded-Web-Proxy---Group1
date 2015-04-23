//129.120.151.96 // CSE03
//129.120.151.94 // CSE01
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
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#define BACKLOG		5
#define BUF_SIZE	1024
#define LISTEN_PORT	9003

#define BLOCKED_LIST "blocked.txt" // List of blocked sites

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
    
    // Enable the socket to reuse the address
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
    
    while ((msg_size = recv(sock, buffer, BUF_SIZE, 0)) > 0)
	{    
        pch = strtok (buffer,"GET /");
        printf ( "%s \n", pch );
        
        if ( (blocked_websites(pch))  == 0 )
        {
			// Checks to see of the website is on the blocked list
            sprintf(buffer, "<html><body>Blocked website!<br><br></body></html>\0");
			send(sock, buffer, sizeof(buffer), 0);
        }
		else
		{
			// If page not found in chache...
			if( access( pch, F_OK ) == -1 ) {
				// Get page...
				if( GET_page( pch ) == 0 ) {
					printf( "Error: could not GET_page\n" );
					close(sock);
					free(sock_desc);
					threadCount++;
					return;
				}
			}
			
			// Get file from cache
			FILE *fptr;
			if( (fptr = fopen( pch, "r" )) == NULL ) {
				printf( "Error: could not open file: %s \n", pch );
				close(sock);
				free(sock_desc);
				threadCount++;
				return;
			}
			
			// Send page to client
			char buffer[BUF_SIZE];
			while( (fread(buffer, 1, BUF_SIZE, fptr)) > 0 ) {
				send(sock, buffer, BUF_SIZE, 0);
			}
		}
	}
	close(sock);
    free(sock_desc);
    threadCount++;
    return;
}



/*----------------------------------------------
 *	CHECK FOR BLOCKED WEBSITE
 *	Return 0 if blocked, 1 if not blocked
 *----------------------------------------------*/
int blocked_websites(char *pch) {
    FILE *fptr;
    char temp[1024];
    int value;

	// Open read-only file
    if( (fptr = fopen( BLOCKED_LIST, "r" )) == NULL ) {
        printf( "Error: blocked_websites could not open file: %s \n", BLOCKED_LIST );
        exit(1);
    }
    
	// Read through blocked sites list
    while( fscanf(fptr, "%s ",temp) != EOF ) {
        value = strcmp(temp, pch);
        if ( value == 0 )
        {
            fclose(fptr);
            return 0; // Site is blocked
        }
    }
    fclose(fptr);
    return 1; // Site is not blocked
}



/*----------------------------------------------
 *	HTTP GET REQUEST
 *	Return 0 on fail, 1 on success
 *----------------------------------------------*/
int GET_page(char *host)
{
	char buffer[BUF_SIZE];       
	struct hostent *host_ent;
	struct sockaddr_in addr;
	int on = 1, get_socket;     
 
	// Get host entity
	if( (host_ent = gethostbyname(host)) == NULL ) {
		herror("gethostbyname");
		exit(1);
	}
	bcopy( host_ent->h_addr, &addr.sin_addr, host_ent->h_length );
	addr.sin_port = htons(80); // Use port 80 for http get request
	addr.sin_family = AF_INET;
	get_socket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
	setsockopt( get_socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int) );
	
	// Error checking
	if(get_socket == -1){
		printf( "Error: GET_page could not create socket \n" ); // get_socket not created
		return 0;
	}
	if( connect(get_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1 ){
		printf( "Error: get_socket could not connect to %s\n", host);
		return 0;
	}
	
	// Send page request
	write(get_socket, "GET /\r\n", strlen("GET /\r\n"));  
	bzero(buffer, BUF_SIZE);
	
	// Create & open new file to write the page
	FILE *fptr;
    if( (fptr = fopen( host, "ab+" )) == NULL ) {
        printf( "Error: could not create file: %s \n", host );
		shutdown(get_socket, SHUT_RDWR); // Shut down socket
		close(get_socket); // Close socket
        return 0;
    }
	
	// Read page to file
	while( read(get_socket, buffer, BUF_SIZE - 1) != 0 ) {
		fprintf( fptr, "%s", buffer );
		bzero( buffer, BUF_SIZE );
	}
 
	fclose( fptr ); // Close file
	shutdown(get_socket, SHUT_RDWR); // Shut down socket
	close(get_socket); // Close socket
 
	return 1;
}