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
#define WORD_LIST "list.txt" // List of inappropriate words

// Boolean
#define TRUE 1
#define FALSE 0

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
    printf("Group 1 proxy server waiting for client...\n");
    
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


char *str_replace ( const char *string, const char *substr, const char *replacement ){
    char *tok = NULL;
    char *newstr = NULL;
    char *oldstr = NULL;
    char *head = NULL;
    
    /* if either substr or replacement is NULL, duplicate string a let caller handle it */
    if ( substr == NULL || replacement == NULL ) return strdup (string);
    newstr = strdup (string);
    head = newstr;
    while ( (tok = strstr ( head, substr ))){
        oldstr = newstr;
        newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 1 );
        /*failed to alloc mem, free old string and return NULL */
        if ( newstr == NULL ){
            free (oldstr);
            return NULL;
        }
        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) , 0, 1 );
        /* move back head right after the last replacement */
        head = newstr + (tok - oldstr) + strlen( replacement );
        free (oldstr);
    }
    //printf("returing string is %s", newstr);
    return newstr;
}


/*----------------------------------------------
 *	CLIENT HANDLER
 *----------------------------------------------*/

void *client_handler(void *sock_desc) {
    int msg_size;
    char buf[BUF_SIZE];
    int sock = *(int*)sock_desc;
    char text[80],buf_send[BUF_SIZE];
    char buffer[BUF_SIZE];
    char *sendBuffer;
    char *URL;
    char replace[81];
    
    while ( (msg_size = recv(sock, buffer, BUF_SIZE, 0)) > 0 )
    {
        // Get page URL from message in buffer
        URL = strtok ( buffer, "GET /" ); // Expected message format: "<method GET> /<page URL> <HTTP protocol> ..."
        printf ( "\tURL:  %s \n", URL ); // Server feedback
        
        // Check if site is blocked
        if ( (blocked_site(URL)) == TRUE )
        {
            memset( buffer, 0, BUF_SIZE );
            sprintf( buffer, "<html><body><br> Sorry! That is a blocked website! <br></html></body>", URL );
            send( sock, buffer, sizeof(buffer), 0 );
        }
        else
        {
            // Check if page is saved in cache
            if( access( URL, F_OK ) == -1 )
            {
                printf( "\tGET:  %s\n", URL ); // Server feedback
                
                // Get page (if not in cache)
                if( get_page( URL ) == FALSE ) {
                    close(sock);
                    free(sock_desc);
                    threadCount++;
                    return;
                }
            }
            
            // Get file from cache
            FILE *fptr;
            if( (fptr = fopen( URL, "r" )) == NULL ) {
                printf( "\tError: client_handler could not open file: %s \n", URL );
                close(sock);
                free(sock_desc);
                threadCount++;
                return;
            }
            
            FILE *fptr_words;
            if( (fptr_words = fopen( WORD_LIST, "r" )) == NULL ) {
                printf( "\tError: client_handler could not open file: %s \n", WORD_LIST );
                close(sock);
                free(sock_desc);
                threadCount++;
                return;
            }
            
            // Send page to client
            char *token;
            size_t len = 0;
            while( (fread(buffer, 1, BUF_SIZE, fptr)) > 0 ) {
                sendBuffer = buffer;
                while( (getline(&token, &len, fptr_words)) >-1 ) {
                    token = strtok (token," ");
                    //printf("%s",token);
                    //printf(" is %d\n",strlen(token));
                    strcpy(replace,token);
                    sendBuffer = str_replace(sendBuffer, replace, "*****");
                }
                //printf("%s",sendBuffer);
                send(sock, sendBuffer, BUF_SIZE, 0);
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
 *	Return TRUE if blocked, FALSE if not blocked
 *----------------------------------------------*/
int blocked_site( char *URL )
{
    FILE *fptr;
    char temp[1024];
    
    // Open list of blocked sites
    if( (fptr = fopen( BLOCKED_LIST, "r" )) == NULL ) {
        printf( "\tError: blocked_site() could not open file: %s\n", BLOCKED_LIST );
        exit(1);
    }
    
    // Read through blocked sites list
    while( fscanf(fptr, "%s ",temp) != EOF ) {
        if ( (strcmp(temp, URL)) == 0 )
        {
            printf( "\tBLOCKED: %s\n", URL ); // Server feedback
            fclose(fptr);
            return TRUE; // Site is blocked
        }
    }
    fclose(fptr);
    return FALSE; // Site is not blocked
}



/*----------------------------------------------
 *	HTTP GET REQUEST
 *	Return TRUE on success, FALSE on fail
 *----------------------------------------------*/
int get_page(char *host)
{
    char buffer[BUF_SIZE];
    struct hostent *host_ent;
    struct sockaddr_in addr;
    int on = 1, get_socket;
    FILE *fptr;
    
    // Get host entity
    if( (host_ent = gethostbyname(host)) == NULL ) {
        printf( "\tError: get_page could not gethostbyname\n" );
        return FALSE;
    }
    bcopy( host_ent->h_addr, &addr.sin_addr, host_ent->h_length );
    addr.sin_port = htons(80); // Use port 80 for http get request
    addr.sin_family = AF_INET;
    get_socket = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP );
    setsockopt( get_socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int) );
    
    // Error checking socket
    if(get_socket == -1) {
        printf( "\tError: get_page could not create socket\n" ); // get_socket not created
        return FALSE;
    }
    if( connect(get_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1 ){
        printf( "\tError: get_page could not connect socket to %s\n", host);
        return FALSE;
    }
    
    // Create & open new file to write the page
    if( (fptr = fopen( host, "ab+" )) == NULL ) {
        printf( "\tError: get_page could not create file: %s\n", host );
        shutdown(get_socket, SHUT_RDWR); // Shut down socket
        close(get_socket); // Close socket
        return FALSE;
    }
    
    // Send page request
    write( get_socket, "GET /\r\n", strlen("GET /\r\n") );  
    bzero( buffer, BUF_SIZE );
    
    // Read page...
    while( read(get_socket, buffer, BUF_SIZE - 1) != 0 ) {
        // Write page to file
        fprintf( fptr, "%s", buffer );
        bzero( buffer, BUF_SIZE );
    }
    
    fclose( fptr ); // Close file
    shutdown(get_socket, SHUT_RDWR); // Shut down socket
    close(get_socket); // Close socket
    return TRUE;
}



