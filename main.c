#include <stdint.h>
#include <pthread.h>

void *serve_client(void *ptr) {
  // Converts ptr back to integer.
  int client_fd = (int) (intptr_t) ptr;
  // ...
}

int main() {
    // ...
    // Required variables
    int client_fd;    // Socket returned by accept
    pthread_t thread; // Thread to be created
    // Creates the thread and calls the function serve_client.
    pthread_create(&thread, NULL, serve_client, (void *) (intptr_t) client_fd);
    // Detaches the thread. This means that, once the thread finishes, it is destroyed.
    pthread_detach(thread);
    // ...
}