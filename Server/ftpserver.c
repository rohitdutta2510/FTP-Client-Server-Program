#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

// Function to handle uploading a file from the client to the server
void handle_put(int client_socket, char *filename) {
    FILE *fptr;
    char buffer[BUFFER_SIZE];
    int recvBytes = 0;

    fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        perror("[-]Error: Unable to open file");
        return;
    }
    while ((recvBytes = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, recvBytes, fptr);
        if (recvBytes < BUFFER_SIZE) {
            break;
        }
    }

    printf("[+]File %s received from client.\n", filename);
    fclose(fptr);
}

// Function to handle downloading a file from the server to the client
void handle_get(int client_sock, char *filename) {
    FILE *fptr;
    char buffer[BUFFER_SIZE];
    int n;

    fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        perror("[-]Error reading file");
        return;
    }
    while ((n = fread(buffer, sizeof(char), BUFFER_SIZE, fptr)) > 0) {
        send(client_sock, buffer, n, 0);
    }

    printf("[+]File %s sent to client.\n", filename);
    fclose(fptr);
}

// Function to handle changing the current directory on the server
void handle_cd(int client_sock, char *dirname) {
    chdir(dirname);
    // Send a confirmation message to the client
    send(client_sock, "Directory changed successfully", strlen("Directory changed successfully"), 0);
}

// Function to handle listing files in the current directory on the server
void handle_ls(int client_sock) {
    DIR *d_ptr;
    struct dirent *dir;
    d_ptr = opendir(".");
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    if (d_ptr) {
        while ((dir = readdir(d_ptr)) != NULL) {
            strcat(buffer, dir->d_name);
            strcat(buffer, "\n");
        }
        closedir(d_ptr);
    }
    send(client_sock, buffer, strlen(buffer), 0);
}

// Thread function to handle communication with a single client
void *client_thread(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        recv(client_socket, buffer, sizeof(buffer), 0);
        printf("Command received: %s\n", buffer);

        if (strncmp(buffer, "put ", 4) == 0) {
            handle_put(client_socket, buffer + 4);
        } else if (strncmp(buffer, "get ", 4) == 0) {
            handle_get(client_socket, buffer + 4);
        } else if (strcmp(buffer, "close") == 0) {
            close(client_socket);
            printf("[+]Client disconnected.\n\n");
            break;
        } else if (strncmp(buffer, "cd ", 3) == 0) {
            handle_cd(client_socket, buffer + 3);
        } else if (strcmp(buffer, "ls") == 0) {
            handle_ls(client_socket);
        } else {
            char message[] = "[-]Invalid command";
            send(client_socket, message, strlen(message), 0);
        }
    }
    free(arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int server_sock, client_sock;
    struct sockaddr_in server_address, client_address;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    int n;

    // Create a socket for the server
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    // Configure the server address
    memset(&server_address, '\0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip);

    // Bind the socket to the specified address and port
    n = bind(server_sock, (struct sockaddr*)&server_address, sizeof(server_address));
    if (n < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    // Listen for incoming connections
    listen(server_sock, 5);
    printf("[+]Listening...\n");

    while (1) {
        addr_size = sizeof(client_address);
        client_sock = accept(server_sock, (struct sockaddr*)&client_address, &addr_size);
        if (client_sock < 0) {
            perror("[-]Accept failed");
            continue;
        }
        printf("[+]Client connected.\n");

        // socket for the thread
        int *thread_sock = malloc(sizeof(int));
        *thread_sock = client_sock;

        // Create a thread to handle communication with the client
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, thread_sock) != 0) {
            perror("[-]Failed to create thread");
            free(thread_sock);
        }
    }

    close(server_sock);
    return 0;
}
