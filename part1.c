#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12345
#define MAX_CONN 5
#define BUFFER_SIZE 1024

// Node structure for the multi-linked list
typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *book_next;
} Node;

// Shared list variables
Node *shared_list_head = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int book_counter = 1;  // Used to track book number

// Function to create a new node
Node* create_node(char *line) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->line = strdup(line);
    new_node->next = NULL;
    new_node->book_next = NULL;
    return new_node;
}

// Function to add a node to the shared list
void add_node_to_list(Node *new_node) {
    pthread_mutex_lock(&list_mutex);
    if (shared_list_head == NULL) {
        shared_list_head = new_node;
    } else {
        Node *temp = shared_list_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_node;
    }
    pthread_mutex_unlock(&list_mutex);
}

// Function to print a book to a file
void write_book_to_file(Node *book_head, int book_number) {
    char filename[20];
    snprintf(filename, sizeof(filename), "book_%02d.txt", book_number);
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return;
    }

    Node *temp = book_head;
    while (temp != NULL) {
        fprintf(file, "%s", temp->line);
        temp = temp->book_next;
    }

    fclose(file);
    printf("Book %02d written to %s\n", book_number, filename);
}

// Function to handle each client connection
void *client_handler(void *socket_desc) {
    int new_socket = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    Node *book_head = NULL, *last_book_node = NULL;

    // Read data from the client (simulate non-blocking I/O by using small delays)
    while (recv(new_socket, buffer, BUFFER_SIZE, 0) > 0) {
        buffer[strcspn(buffer, "\n")] = '\0';  // Remove newline character

        // Create a new node for each line received
        Node *new_node = create_node(buffer);

        // Add node to the shared list
        add_node_to_list(new_node);

        // Handle book-specific linked list
        if (book_head == NULL) {
            book_head = new_node;
        } else {
            last_book_node->book_next = new_node;
        }
        last_book_node = new_node;
    }

    // Save the book to a file once the connection is closed
    pthread_mutex_lock(&list_mutex);
    write_book_to_file(book_head, book_counter++);
    pthread_mutex_unlock(&list_mutex);

    close(new_socket);
    free(socket_desc);
    pthread_exit(NULL);
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t thread_id;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Prepare the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 1;
    }

    // Listen for incoming connections
    listen(server_socket, MAX_CONN);
    printf("Server listening on port %d\n", PORT);

    // Accept and handle incoming connections
    while ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size))) {
        printf("Connection accepted\n");

        int *new_sock = malloc(sizeof(int));
        *new_sock = new_socket;

        if (pthread_create(&thread_id, NULL, client_handler, (void *)new_sock) < 0) {
            perror("Could not create thread");
            free(new_sock);
            close(new_socket);
            return 1;
        }

        printf("Handler assigned\n");
    }

    if (new_socket < 0) {
        perror("Accept failed");
        close(server_socket);
        return 1;
    }

    close(server_socket);
    return 0;
}
