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

#define BACKLOG		5
#define BUF_SIZE	1024
#define LISTEN_PORT	9003

#define BLOCKED_LIST "blocked.txt" // List of blocked sites

int threadCount = BACKLOG;
void *client_handler(void *sock_desc);
int blocked_websites(char *name);
char *str_replace ( const char *string, const char *substr, const char *replacement );


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
    int count =0;
    
    FILE *fp;
    FILE *fr;
    char *line;
    char *source = NULL;
    size_t len = 0;
    char badword[80];
 
    
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
			// Check if page is already in cache
			
			//if( page is NOT in the cache ) {
				// HTTP GET request for page
				if( GET_page( pch ) == 0 )
					printf(" ERROR!!!! \n ");
				else
                {
                 // Get page from cache
                    fp=fopen(pch, "r");
                    
                    if (fp != NULL) {
                        /* Go to the end of the file. */
                        if (fseek(fp, 0L, SEEK_END) == 0) {
                            /* Get the size of the file. */
                            long bufsize = ftell(fp);
                            if (bufsize == -1) { /* Error */ }
                            
                            /* Allocate our buffer to that size. */
                            source = malloc(sizeof(char) * (bufsize + 1));
                            count = count + (sizeof(char) * (bufsize + 1));
                            
                            /* Go back to the start of the file. */
                            if (fseek(fp, 0L, SEEK_SET) == 0) { /* Error */ }
                            
                            /* Read the entire file into memory. */
                            size_t newLen = fread(source, sizeof(char), bufsize, fp);
                            if (newLen == 0) {
                                fputs("Error reading file", stderr);
                            } else {
                                source[++newLen] = '\0'; /* Just to be safe. */
                            }
                        }
                        fclose(fp);
                        //printf("%s",source);
                        fr = fopen ("list.txt", "r");
                        if (fr!=NULL)
                        {
                             while((getline(&line, &len, fr)) >-1)
                             {
                                 //printf("%s",line);
                                 line = strtok (line," ");
                                 source=str_replace(source,line,"******");
                             }
                        }
                        free(line);
                        fclose(fr);
                        send(sock, source, count+1, 0);
                        //free(source);
                    }
                }
			//}
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






					char *build_get_query(char *host, char *page)
					{
					  char *query;
					  char *getpage = page;
					  char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
					  if(getpage[0] == '/'){
						getpage = getpage + 1;
						fprintf(stderr,"Removing leading \"/\", converting %s to %s\n", page, getpage);
					  }
					  // -5 is to consider the %s %s %s in tpl and the ending \0
					  query = (char *)malloc(strlen(host)+strlen(getpage)+strlen("HTMLGET 1.0")+strlen(tpl)-5);
					  sprintf(query, tpl, getpage, host, "HTMLGET 1.0");
					  return query;
					}	






/*----------------------------------------------
 *	HTTP GET REQUEST
 *	Return 0 on fail, 1 on success
 *----------------------------------------------*/
 
int GET_page( char *host )
{
	int GET_SOCK, tmpres;
	struct sockaddr_in *remote;
	char IP[15];
	struct hostent *hent;
	char *get;

	// Get IP address of host
	if( (hent = gethostbyname(host)) == NULL ) {
		printf( "Error: can't get host IP \n" );
		return 0;
	}
	if( inet_ntop(AF_INET, (void *)hent->h_addr_list[0], IP, 15) == NULL )
	{
		printf( "Error: can't resolve host \n" );
		return 0;
	}

	// Create socket for HTTP GET request 
	if( (GET_SOCK = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		printf( "Error: could not create GET_SOCK \n" );
		return 0;
	}
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	
	// ??
	tmpres = inet_pton(AF_INET, IP, (void *)(&(remote->sin_addr.s_addr)));
	if( tmpres < 0) {
		perror("Can't set remote->sin_addr.s_addr");
		exit(1);
	}
	else if(tmpres == 0) {
		fprintf(stderr, "%s is not a valid IP address\n", IP);
		exit(1);
	}
	remote->sin_port = htons(80);
	
	// Connect to remote host
	if( connect(GET_SOCK, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0 ) {
		perror("Could not connect");
		exit(1);
	}
	
	// Build GET message
	get = build_get_query(host, "");	// REWRITE
	
	// Send query to remote server
	int sent = 0;
	while(sent < strlen(get))
	{
		tmpres = send(GET_SOCK, get+sent, strlen(get)-sent, 0);
		if(tmpres == -1){
			perror("Can't send query");
			exit(1);
		}
		sent += tmpres;
	}
	
	// Receive the page
	char buf[BUFSIZ+1];
	memset(buf, 0, sizeof(buf));
	int htmlstart = 0;
	char * htmlcontent;
	
	// Create & open new file to write the page
	FILE *fptr;
    if( (fptr = fopen( host, "ab+" )) == NULL ) {
        printf( "Error: could not create file: %s \n", host );
        exit(1);
    }
	
	while((tmpres = recv(GET_SOCK, buf, BUFSIZ, 0)) > 0)
	{
        
		if(htmlstart == 0) {
			htmlcontent = strstr(buf, "\r\n\r\n");
			if(htmlcontent != NULL){
				htmlstart = 1;
				htmlcontent += 4;
			}
		}
		else
			htmlcontent = buf;
	
		if(htmlstart) {
			//fprintf(stdout, htmlcontent);
			
			// Redirect page to a new file
            //fputs(htmlcontent,fptr );
            fprintf(fptr,"%s",htmlcontent);
		}
		
		memset(buf, 0, tmpres);
	}	
	if(tmpres < 0)
		perror("Error receiving data");
 
	fclose(fptr);
	free(get);
	free(remote);
	close(GET_SOCK);
	return 1;
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
    return newstr;
}




