#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct Node {
    char *line;
    struct Node *next;
    struct Node *book_next;
    struct Node *next_frequent_search;
} Node;

typedef struct {
    int id;
    int socket_fd;
    Node *book_head;
    char search_term[100];
} ThreadData;

Node *shared_list_head = NULL;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int book_count = 0;
int search_count = 0;

// Function to create a new node
Node *create_node(const char *line) {
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->line = strdup(line);
    new_node->next = NULL;
    new_node->book_next = NULL;
    new_node->next_frequent_search = NULL;
    return new_node;
}

// Function to add a node to the shared list
void add_to_shared_list(Node *node) {
    pthread_mutex_lock(&list_mutex);
    if (shared_list_head == NULL) {
        shared_list_head = node;
    } else {
        Node *temp = shared_list_head;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
    pthread_mutex_unlock(&list_mutex);
    printf("Added node: %s\n", node->line);
}

// Function to handle client connections
void *handle_client(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    Node *book_last = NULL;
    FILE *book_file;
    char filename[20];
    snprintf(filename, sizeof(filename), "book_%02d.txt", data->id);

    book_file = fopen(filename, "w");
    if (!book_file) {
        perror("Failed to open file");
        close(data->socket_fd);
        free(data);
        pthread_exit(NULL);
    }

    // Read from the client socket and add lines to the book
    while ((bytes_read = read(data->socket_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        Node *new_node = create_node(buffer);
        
        // Add to shared list
        add_to_shared_list(new_node);
        
        // Link book nodes
        if (book_last == NULL) {
            data->book_head = new_node;
        } else {
            book_last->book_next = new_node;
        }
        book_last = new_node;

        // Check for search term
        if (strstr(buffer, data->search_term) != NULL) {
            search_count++;
        }

        // Write to book file
        fprintf(book_file, "%s", buffer);
    }

    // Close connection and file
    fclose(book_file);
    close(data->socket_fd);
    
    printf("Connection closed. Book written to %s\n", filename);
    free(data);
    pthread_exit(NULL);
}

// Main function
int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -l <port> -p <search_term>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    char *search_term = argv[4];

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t threads[MAX_CLIENTS];
    int client_count = 0;

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // Set socket to non-blocking mode
    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    // Bind the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Failed to listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd > 0) {
            printf("Accepted connection\n");

            ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
            data->id = ++book_count;
            data->socket_fd = client_fd;
            strncpy(data->search_term, search_term, sizeof(data->search_term) - 1);

            pthread_create(&threads[client_count++], NULL, handle_client, data);
            if (client_count >= MAX_CLIENTS) {
                client_count = 0;
            }
        }
        usleep(1000); // Reduce CPU usage
    }

    close(server_fd);
    return 0;
}
