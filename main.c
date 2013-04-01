#include <stdint.h>
#include <pthread.h>

void *serve_client(void *ptr) {
  // Converts ptr back to integer.
  int client_fd = (int) (intptr_t) ptr;
  // ...
}

int main() {
    // ...

    // Lines 13-32 from beej's 5.1
int status;
struct addrinfo hints;
struct addrinfo *servinfo;  // will point to list of struct addrinfos each
                            // which contain a struct socket address.

memset(&hints, 0, sizeof hints); // make sure the struct is empty
hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
hints.ai_flags = AI_PASSIVE;     // fill in my IP for me


if ((status = getaddrinfo(NULL, "5555", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
}

// servinfo now points to a linked list of 1 or more struct addrinfos

// ... do everything until you don't need servinfo anymore ....




    // Required variables
    int client_fd;    // Socket returned by accept
    pthread_t thread; // Thread to be created
    // Creates the thread and calls the function serve_client.
    pthread_create(&thread, NULL, serve_client, (void *) (intptr_t) client_fd);
    // Detaches the thread. This means that, once the thread finishes, it is destroyed.
    pthread_detach(thread);
    // ...

freeaddrinfo(servinfo); // free the linked-list here

}