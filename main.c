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

#include <cv.h>
#include <highgui.h>

#include <sys/time.h>
#include <signal.h>
#include <time.h>

// This struct is created to save information that will be needed by the timer,
// such as socket file descriptors, frame numbers and video captures.
struct send_frame_data {
  int cl_fd;
  // other fields
};

CvCapture *video;
IplImage *image;
CvMat *thumb;
CvMat *encoded;

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

// Made the following variables global because I need them in multiple funtions
  
  int len = 512;
  int flags = 0; //Beej's reccomends it we set to zero.

// Global timer variables because pause() doesn't know what play_interval and play_timer are
  timer_t play_timer;
  struct itimerspec play_interval;  

/* This function will be called when the timer ticks 
*/
void send_frame(union sigval sv_data) {
  
  struct send_frame_data *data = (struct send_frame_data *) sv_data.sival_ptr;
    // You may retrieve information from the caller using data->field_name
    
    // Obtain the next frame from the video file
    image = cvQueryFrame(video);
    if (!image) {
      printf("Next frame doesn't exist or can't be obtained");
    } else {

    // Convert the frame to a smaller size (WIDTH x HEIGHT)
    thumb = cvCreateMat(240, 320, CV_8UC3);
    cvResize(image, thumb, CV_INTER_AREA);

    // Encode the frame in JPEG format with JPEG quality 30%.
    const static int encodeParams[] = { CV_IMWRITE_JPEG_QUALITY, 30 };
    encoded = cvEncodeImage(".jpeg", thumb, encodeParams);
    // After the call above, the encoded data is in encoded->data.ptr
    // and has a length of encoded->cols bytes. GCC complains about variable sized obj not being initialized

    // Calculate payload + RTP length
    int payloadPlusHeader = 12 + encoded->cols;
    // Construct RTP packet & send the prefix
    char header[4]; // prefix of 4 bytes
            header[0] = (char ) 0x24;
            header[1] = (char ) 0x0;
            header[2] = (char ) ((payloadPlusHeader & 0xFF00) >> 8); 
            header[3] = (char ) (payloadPlusHeader & 0x00FF);

        int client_fd = data->cl_fd;

        int header_bytes = send(client_fd,header,4,flags);
        // error checks
        if (header_bytes == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (header_bytes == -1) {
        printf("There's an error while sending prefix header: %s\n", strerror(errno));
        printf("client_fd just after send() returns -1 = %d\n", client_fd); // to see if fd was over written
        } else
        if (header_bytes < 4) {
            printf("Response was not completely sent\n");
        } else {
            printf("%d bytes of the prefix were sent to the buffer\n", header_bytes);
        }

    // Send frame data
    char* frameBuf = encoded->data.ptr;
    int frameDataLength = encoded->cols;
    int data_sent = send(client_fd,frameBuf,frameDataLength,flags);
        // error checks
        if (data_sent == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (data_sent == -1) {
        printf("There's an error while sending frame data: %s\n", strerror(errno));
        printf("client_fd after send() returns -1 in send_frame = %d\n", client_fd); // to see if fd was over written
        } else
        if (data_sent < frameDataLength) {
            printf("Frame data was not completely sent\n");
        } else {
            printf("%d bytes were sent to the buffer\n", data_sent);
        }
    }
}
  
/* Deals with a setup request and opens the video file.
*/
void setup(int client_fd, char buf[]) {
    printf("code reaches setup\n");

    // Open the video file.
    
    video = cvCaptureFromFile("movie.Mjpeg");
    if (!video) {
        printf("File doesn't exist or can't be captured as a video file\n");

    } else {
        char *msg = "RTSP/1.0 200 OK\nCSeq: 1\nSession: 123456\n\n";
        int bytes_sent = send(client_fd,msg,strlen(msg),flags);
        // error checks
        if (bytes_sent == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (bytes_sent == -1) {
        printf("There's an error while sending setup response: %s\n", strerror(errno));
        printf("client_fd just after send() returns -1 = %d\n", client_fd); // to see if fd was over written
        } else
        if (bytes_sent < strlen(msg)) {
            printf("Response was not completely sent\n");
        } else {
            printf("%d bytes were sent to the buffer\n", bytes_sent);
        }
    }

}

/* Deals with play request, starts the timer, parses the frame
*/
void play(int client_fd, char buf[]) {
    printf("code reaches play\n");
    
    char *msg = "RTSP/1.0 200 OK\nCSeq: 2\nSession: 123456\n\n";
        int response_bytes = send(client_fd,msg,strlen(msg),flags);
        // error checks
        if (response_bytes == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (response_bytes == -1) {
        printf("There's an error while sending play response header: %s\n", strerror(errno));
        printf("client_fd just after send() returns -1 = %d\n", client_fd); // to see if fd was over written
        } else
        if (response_bytes < strlen(msg)) {
            printf("Response was not completely sent\n");
        } else {
            printf("%d bytes were sent to the buffer\n", response_bytes);
        }

    // The following snippet is used to create and start a new timer that runs
    // every 40 ms.
    struct send_frame_data data; // Set fields as necessary
    data.cl_fd = client_fd;
    struct sigevent play_event;
    
    memset(&play_event, 0, sizeof(play_event));
    play_event.sigev_notify = SIGEV_THREAD;
    play_event.sigev_value.sival_ptr = &data;
    play_event.sigev_notify_function = send_frame;

    play_interval.it_interval.tv_sec = 0;
    play_interval.it_interval.tv_nsec = 40 * 1000000; // 40 ms in ns
    play_interval.it_value.tv_sec = 0;
    play_interval.it_value.tv_nsec = 1; // can't be zero

    timer_create(CLOCK_REALTIME, &play_event, &play_timer);
    timer_settime(play_timer, 0, &play_interval, NULL);



}

/* Deals with pause request, stops the timer and deletes it
*/ 
void pauseVid(int client_fd, char buf[]) {
    printf("code reaches pause()\n");

     char *msg = "RTSP/1.0 200 OK\nCSeq: 2\nSession: 123456\n\n";
        int bytes_sent = send(client_fd,msg,strlen(msg),flags);
        // error checks
        if (bytes_sent == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (bytes_sent == -1) {
        printf("There's an error while sending pause response: %s\n", strerror(errno));
        printf("client_fd just after send() returns -1 = %d\n", client_fd); // to see if fd was over written
        } else
        if (bytes_sent < strlen(msg)) {
            printf("Response was not completely sent\n");
        } else {
            printf("%d bytes were sent to the buffer\n", bytes_sent);
        }

    // The following snippet is used to stop a currently running timer. The current
    // task is not interrupted, only future tasks are stopped.
    play_interval.it_interval.tv_sec = 0;
    play_interval.it_interval.tv_nsec = 0;
    play_interval.it_value.tv_sec = 0;
    play_interval.it_value.tv_nsec = 0;
    timer_settime(play_timer, 0, &play_interval, NULL);


    // The following line is used to delete a timer.
    timer_delete(play_timer);
    
}

/* Deals with teardown request. Close the video file (not the connection, disconnect does that)
*/ void teardown(int client_fd, char buf[]) {

    // Close the video file
    cvReleaseCapture(&video);

    char *msg = "RTSP/1.0 200 OK\nCSeq: 4\nSession: 123456\n\n";
        int bytes_sent = send(client_fd,msg,strlen(msg),flags);
        // error checks
        if (bytes_sent == 0) {
            printf("No bytes were sent or client closed connection\n");
        } else
        if (bytes_sent == -1) {
        printf("There's an error while sending teardown response: %s\n", strerror(errno));
        printf("client_fd just after send() returns -1 = %d\n", client_fd); // to see if fd was over written
        } else
        if (bytes_sent < strlen(msg)) {
            printf("Response was not completely sent\n");
        } else {
            printf("%d bytes were sent to the buffer\n", bytes_sent);
        }
}

/*
* serve_client() given by CS317 instructors
*/
void *serve_client(void *ptr) {
  
  // Converts ptr back to integer.
  int client_fd = (int) (intptr_t) ptr;
  
  // Every client has their own buffer and vice versa.
  char buf[len];
  int listening = 1;
  while (listening) {
  listening = recv(client_fd, &buf, len, flags);
  
  //error checks
  
  if (listening == 0) {
    printf("client closed connection\n");
  } else

  if (listening == -1) {
    printf("There's an error while reading RTSP requests: %s\n", strerror(errno));
    printf("client_fd just after recv() returns -1 = %d\n", client_fd);
  } else 
  { printf("%d bytes recieved in the buffer\n", listening);
    
    // Deal with a successful RTSP request
    
    printf("%c%c%c%c request recieved\n", buf[0], buf[1], buf[2], buf[3]);
    if ( 'S' == buf[0] ) {
        printf("deal with SETUP...\n");
        //const char* filename = // figure out how to get read file name from setup request
        setup(client_fd, buf); // passing the fd & buf so setup acts on that buf for that client
    } else
    if ( 'T' == buf[0] ) {
        printf("deal with TEARDOWN...\n");
        teardown(client_fd, buf);
    } else
    if ( ('P' == buf[0]) && ('A' == buf[1]) ) { // if first letter is p and second a
        printf("deal with PAUSE...\n");
        pauseVid(client_fd, buf);
    }
    else {
        printf("deal with PLAY...\n");
        printf("%s\n", buf);
        play(client_fd, buf);
    }
  }
  }
  printf("last line of serve_client()\n");
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
    //struct sigaction sa;
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
