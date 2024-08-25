#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Function to handle sending a file to the server
void func_put(int cl_socket, char *filename) {
    FILE *fptr;
    char buffer[BUFFER_SIZE];
    int n;

    // Open the file in binary read mode
    fptr = fopen(filename, "rb");
    if (fptr == NULL) {
        printf("[-]Error: File %s not found.\n", filename);
        return;
    }

    // Read and send the file in chunks
    while ((n = fread(buffer, sizeof(char), BUFFER_SIZE, fptr)) > 0) {
        send(cl_socket, buffer, n, 0);
    }

    printf("[+]File %s uploaded\n", filename);
    fclose(fptr);
}

// Function to handle receiving a file from the server
void func_get(int cl_socket, char *filename) {
    FILE *fptr;
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;

    // Open the file in binary write mode
    fptr = fopen(filename, "wb");
    if (fptr == NULL) {
        printf("[-]Error : Unable to open file\n");
        return;
    }

    // Receive and write the file in chunks
    while ((bytesReceived = recv(cl_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        fwrite(buffer, 1, bytesReceived, fptr);
        if (bytesReceived < BUFFER_SIZE) {
            break;
        }
    }

    printf("[+]File %s downloaded\n", filename);
    fclose(fptr);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int client_socket;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];

    // Create a socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP client socket created.\n");

    // Set up the server address
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // Use htons to convert port to network byte order
    address.sin_addr.s_addr = inet_addr(ip);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("[-]Connection error");
        exit(1);
    }
    printf("[+]Connected to the server.\n");

    while (1) {
        printf("ftp_client> ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // Remove newline character

        // Send the user's command to the server
        send(client_socket, buffer, strlen(buffer), 0);

        if (strncmp(buffer, "put ", 4) == 0) {
            func_put(client_socket, buffer + 4);
        } else if (strncmp(buffer, "get ", 4) == 0) {
            func_get(client_socket, buffer + 4);
        } else if (strcmp(buffer, "close") == 0) {
            // close the client socket
            close(client_socket);
            printf("[-]Disconnected from the server.\n");
            break;
        } else if (strncmp(buffer, "cd ", 3) == 0 || strcmp(buffer, "ls") == 0) {
            bzero(buffer, BUFFER_SIZE);
            recv(client_socket, buffer, sizeof(buffer), 0);
            printf("%s\n", buffer);
        } else {
            printf("[-]Invalid command.\n");
        }
    }

    return 0;
}
