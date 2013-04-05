#include <stdint.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <errno.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10     // how many pending connections queue will hold

struct fdstruct {       // Jason recommended I store my file descriptors in a struct
    int filedescriptor;
};

/*
* serve_client() given by CS317 instructors
*/
void *serve_client(void *ptr) {
  printf("code reaches serve_client\n");
  // Need to get struct back.
  
  // Converts ptr back to integer.
  int client_fd = (int) (intptr_t) ptr;
  //int client_fd = clientfdst->filedescriptor; // read from this port until there isn't anything to read
  
    
  int len = 71;
  char *buf[len]; //Luca said it's supposed to be a pointer to char.
  // length of "SETUP movie.Mjpeg RTSP/1.0\nCSeq: 1\nTransport: RTP/UDP; client_port= 25000\n\n" including whitespace & /n
  
  int flags = 0; //Beej's reccomends it we set to zero.
  // ...start listening to RTP commands
 
  int listening = recv(client_fd, buf, len, flags);
  //error checks
  if (listening == 0) {
    printf("client closed connection\n");
  } 

  if (listening == -1) {
    printf("There's an error while reading RTSP requests: %s\n", strerror(errno));
    printf("client_fd just after recv() returns -1 = %d\n", client_fd);
  }   
  else {
    printf("%d bytes were written into the buffer\n", listening);

    char *s = "S";
    char *t = "T";
    char *p = "P";
    char *a = "A";
    
    char first = buf[0];
    char second = buf[1];
    printf("The first letter of the request is %s\n", first);
    if ( !strcmp(s,first) ) {
        //deal with "SETUP..."
    }
    if ( !strcmp(t,first) ) {
        //deal with "TEARDOWN..."
    }
    if ( !strcmp(p,first) && !strcmp(a,second) ) { // if first letter is p and second a
        //deal with "PAUSE..."
    }
    else {
        //deal with "PLAY..."
    }
  }  
  return;
}

// get sockaddr, IPv4 or IPv6: from Beej's guide
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/* Beej's main()
*  
*/
int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;   
        }
        // Reading socket addresses and printing them out.
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        //serve_client((void *) (intptr_t) new_fd); // single client non-threaded

        // pthread code given by CS317 instructors
        
        // Required variables
        /*truct fdstruct newfdst;
        newfdst.filedescriptor = new_fd;*/
        pthread_t thread; // Thread to be created
        // Creates the thread and calls the function serve_client.
        printf("new_fd before pthread_create() = %d\n", new_fd);
        pthread_create(&thread, NULL, serve_client, (void *) (intptr_t) new_fd); // (void *) &newfdst for last param for the struct way
        // Detaches the thread. This means that, once the thread finishes, it is destroyed.
        pthread_detach(thread);
        // ...

        //close(new_fd);  // parent doesn't need this
    }

    return 0;
}
