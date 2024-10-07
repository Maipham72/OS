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
    struct Node *bookNext;
    struct Node *nextFrequentSearch;
} Node;

typedef struct {
    int id;
    int socketFd;
    Node *bookHead;
    char searchTerm[100];
} ThreadData;

Node *shareListHead = NULL;
pthread_mutex_t listMutex = PTHREAD_MUTEX_INITIALIZER;
int bookCount = 0;
int searchCount = 0;

// create new node
Node *createNode(const char *line) {
    Node *newNode = (Node *)malloc(sizeof(Node));

    if (newNode == NULL) {
        perror("Failed to allocate memory for new node");
        return NULL;
    }

    newNode->line = strdup(line);
    newNode->next = NULL;
    newNode->bookNext = NULL;
    newNode->nextFrequentSearch = NULL;
    return newNode;
}

// add node to a shared list
void addNodeSharedList (Node *node) {
    pthread_mutex_lock(&listMutex);
    if (shareListHead == NULL) {
        shareListHead = node;
    } else {
        Node *temp = shareListHead;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = node;
    }
    pthread_mutex_unlock(&listMutex);
    printf("Added node: %s\n", node->line);
}

// handle client connection
void *clientConnection (void *arg) {
    ThreadData *data = (ThreadData *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;

    Node *bookLast = NULL;
    FILE *bookFile;
    char filename[20];
    snprintf(filename, sizeof(filename), "book_%02d.txt", data->id);

    bookFile = fopen(filename, "w");
    if (!bookFile) {
        perror("Failed to open file");
        close(data->socketFd);
        free(data);
        pthread_exit(NULL);
    }

    // read from client socket and add lines to book
    while ((bytesRead = read(data->socketFd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        Node *newNode = createNode(buffer);

        if (newNode == NULL) {
            perror("Failed to create new node");
            break;
        }

        // add to shared list
        addNodeSharedList(newNode);

        // link book nodes
        if (bookLast == NULL) {
            data->bookHead = newNode;
        } else {
            bookLast->bookNext = newNode;
        }
        bookLast = newNode;

        // check for search term
        if (strstr(buffer, data->searchTerm) != NULL) {
            pthread_mutex_lock(&listMutex);
            searchCount++;
            pthread_mutex_unlock(&listMutex);
        }

        // write to book file
        fprintf(bookFile, "%s", buffer);
    }

    // close connection and file
    fclose(bookFile);
    close(data->socketFd);

    printf("Connection closed. Book written to %s\n", filename);
    free(data);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s -l <port> -p <searchTerm>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    char *searchTerm = argv[4];

    int serverFd;
    int clientFd;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;

    socklen_t clientLen = sizeof(clientAddr);
    pthread_t threads[MAX_CLIENTS];
    int clientCount = 0;

    // create a socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    // set socket to non-blocking mode
    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    // bind the socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    if (bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Failed to bind socket");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    if (listen(serverFd, MAX_CLIENTS) < 0) {
        perror("Failed to listen on socket");
        close(serverFd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", port);

    while(1) {
        clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientFd > 0) {
            printf("Accepted connection\n");

            ThreadData *data = (ThreadData *)malloc(sizeof(ThreadData));
            data->id = bookCount++;
            data->socketFd = clientFd;
            strncpy(data->searchTerm, searchTerm, sizeof(data->searchTerm) - 1);

            pthread_create(&threads[clientCount++], NULL, clientConnection, data);
            if (clientCount >= MAX_CLIENTS) {
                clientCount = 0;
            }
        }
        usleep(1000);
    }

    close(serverFd);
    return 0;
}