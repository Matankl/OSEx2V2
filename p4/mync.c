#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#define index 3
#include <sys/wait.h>
#include <stdbool.h>
#include <getopt.h>
#include <errno.h>

// Function to execute a program with specific I/O redirection based on mode
void runprogram(pid_t pid, char mode, int client_sock, int server_sock, const char *exe, const char *arg)
{
    if (pid == 0) { // Child process
        // Redirect I/O based on mode (input, output, or both)
        switch (mode) {
            case 'i':
                dup2(client_sock, 0);  // Redirect input
                break;
            case 'o':
                dup2(client_sock, 1);  // Redirect output
                break;
            case 'b':
                dup2(client_sock, 0);  // Redirect both input and output
                dup2(client_sock, 1);
                break;
        }
        // Execute the specified program
        execlp(exe, exe, arg, NULL);
        // If execlp fails, print error and exit
        perror("Execution of exe failed");
        exit(EXIT_FAILURE);
    }
    
    // Wait for the child process to finish
    int status;
    waitpid(pid, &status, 0);
    
    // Close the sockets based on whether client_sock and server_sock are the same
    if (client_sock == server_sock) {
        close(client_sock);
    } else {
        close(server_sock);
        close(client_sock);
    }
}

// Buffer size for communication
#define BUFFER_SIZE 1024

// Function to handle communication over a TCP connection
void talking(int client_sock, pid_t pid, char *buffer)
{
    if (pid == 0) { // Child process: read from server
        while (1) {
            // Read data from the client
            ssize_t bytes_read = read(client_sock, buffer, BUFFER_SIZE - 1);
            if (bytes_read <= 0) {
                break; // Exit loop if no data or error occurs
            }
            buffer[bytes_read] = '\0'; // Null-terminate the string
            printf("Received from server: %s\n", buffer); // Print received data
        }
        close(client_sock); // Close the socket
        exit(0); // Exit child process
    } else { // Parent process: write to server
        while (1) {
            printf("Enter message (-1 to quit): \n");
            // Get user input
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

            // Exit loop if user enters "-1"
            if (strcmp(buffer, "-1") == 0) {
                break;
            }

            // Send user input to the server
            if (write(client_sock, buffer, strlen(buffer)) <= 0) {
                break;
            }
        }
        close(client_sock); // Close the socket

        // Kill the child process that is reading from the server
        kill(pid, SIGKILL);
    }
}

// Function to handle communication over a UDP connection
void talking_udp(int client_sock, pid_t pid, char *buffer, struct sockaddr_in server_addr)
{
    if (pid == 0) { // Child process: read from server
        while (1) {
            // Read data from the client using recvfrom (UDP)
            ssize_t bytes_read = recvfrom(client_sock, buffer, BUFFER_SIZE, 0, NULL, 0);
            if (bytes_read <= 0) {
                break; // Exit loop if no data or error occurs
            }
            buffer[bytes_read] = '\0'; // Null-terminate the string
            printf("Received from server: %s\n", buffer); // Print received data
        }
        close(client_sock); // Close the socket
        exit(0); // Exit child process
    } else { // Parent process: write to server
        while (1) {
            printf("Enter message (-1 to quit): \n");
            // Get user input
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Remove newline character

            // Exit loop if user enters "-1"
            if (strcmp(buffer, "-1") == 0) {
                break;
            }

            // Send user input to the server using sendto (UDP)
            if (sendto(client_sock, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) <= 0) {
                break;
            }
        }
        close(client_sock); // Close the socket

        // Kill the child process that is reading from the server
        kill(pid, SIGKILL);
    }
}

// Function to create a TCP client and initiate communication
int create_client(int port, char mode, const char *server_ip)
{
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create a TCP socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect to the server
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    // Create a child process to handle communication
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    // Call the talking function to handle TCP communication
    talking(client_sock, pid, buffer);
    return 0;
}

// Function to create a TCP server and handle client connections
int create_server(char mode, int port, const char *exe, const char *arg, int flag)
{
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    // Create a TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of the address
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    // Ensure the correct executable is specified
    if (strcmp(exe, "./ttt") != 0) {
        puts(exe);
        perror("exe is not ./ttt");
        exit(EXIT_FAILURE);
    }

    // Accept a client connection
    socklen_t client_addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_size);
    if (client_sock < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    // Fork a child process to handle the client
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    // Handle communication or run a program based on the flag
    if (flag) {
        talking(client_sock, pid, buffer);
    } else {
        runprogram(pid, mode, client_sock, server_sock, exe, arg);
    }
    return 0;
}

// Function to create a UDP server and handle communication
int create_udp_server(char mode, int port, const char *exe, const char *arg, int run, int timeout)
{
    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    // Create a UDP socket
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of the address
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Ensure the correct executable is specified
    if (strcmp(exe, "./ttt") != 0) {
        puts(exe);
        perror("exe is not ./ttt");
        exit(EXIT_FAILURE);
    }

    // Receive data from the client
    socklen_t client_addr_size = sizeof(client_addr);
    memset(&client_addr, 0, client_addr_size);
    size_t bytes = recvfrom(server_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_size);
    printf("Received from client: %s\n", buffer);

    // Fork a child process to handle the client
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    // Set a timeout if specified
    if (timeout > 0) {
        alarm(timeout);
    }

    // Handle communication or run a program based on the run flag
    if (run) {
        talking_udp(server_sock, pid, buffer, client_addr);
    } else {
        runprogram(pid, mode, server_sock, server_sock, exe, arg);
    }
    return 0;
}

// Function to create a UDP client and initiate communication
int create_udp_client(int port, char mode, const char *server_ip, int timeout)
{
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create a UDP socket
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set server address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Send data to the server
    sendto(client_sock, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

    // Fork a child process to handle communication
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    // Set a timeout if specified
    if (timeout > 0) {
        alarm(timeout);
    }

    // Handle UDP communication
    memset(buffer, 0, BUFFER_SIZE);
    talking_udp(client_sock, pid, buffer, server_addr);
    return 0;
}

// Main function to handle command-line arguments and initiate server/client creation
int main(int argc, char *argv[])
{
    // the formats of the arguments are as follows:
    // ./p4 -e "./ttt 123456789" -i UDPS4050 -t 10                                  //with timeout only oneside 
    // ./p4 -e "./ttt 123456789" -i UDPS4050 -o TCPClocalhost,4050                  //without timeout both sides
    // over all it can be with or without timeout and one or both sides it can be udp or tcp
    int flag = 1;
    char mode = 0;
    int port = 0;
    char *exe = NULL;
    char *arg = NULL;
    int timeout = -1;

    int opt;
    // Parse command-line options
    while ((opt = getopt(argc, argv, "e:t:iob")) != -1) {
        switch (opt) {
            case 'e': // Executable argument
                printf("contains -e\n");
                flag = 0;
                break;
            case 'i': // Input mode
                printf("contains -i\n");
                mode = opt;
                break;
            case 'o': // Output mode
                printf("contains -o\n");
                mode = opt;
                break;
            case 'b': // Both input and output mode
                printf("contains -b\n");
                mode = opt;
                break;
            case 't': // Timeout argument
                printf("contains -t");
                timeout = atoi(optarg);
                printf("timeout: %d\n", timeout);
                if (timeout <= 0) {
                    fprintf(stderr, "Timeout must be a positive integer\n");
                    exit(EXIT_FAILURE);
                }
                break;
        }
    }

    // Process remaining command-line arguments
    char *l;
    for (int i = 0; i < argc; i++) {
        if (strstr(argv[i], "./ttt")) {
            exe = strtok(argv[i], " ");
            arg = strtok(NULL, " ");
        }
    //     if (strstr(argv[i], "TCPS")) { // TCP Server
    //         puts("hello man");
    //         isServer = true;
    //         l = strtok(argv[i], "S");
    //         port = atoi(strtok(NULL, "S"));
    //         create_server(mode, port, exe, arg, flag);
    //     }
    //     if (strstr(argv[i], "TCPC")) { // TCP Client
    //         puts("hello man 2");
    //         isServer = false;
    //         l = strtok(argv[i], "C");
    //         port = atoi(strtok(NULL, "C"));
    //         port = atoi(strtok(NULL, "C"));
    //         create_client(port, mode, "127.0.0.1");
    //     }
    //     if (strstr(argv[i], "UDPS")) { // UDP Server
    //         puts("hello man 3");
    //         l = strtok(argv[i], "S");
    //         port = atoi(strtok(NULL, "S"));
    //         create_udp_server(mode, port, exe, arg, flag, timeout);
    //     }
    //     if (strstr(argv[i], "UDPC")) { // UDP Client
    //         puts("hello man 4");
    //         l = strtok(argv[i], "C");
    //         port = atoi(strtok(NULL, "C"));
    //         create_udp_client(port, mode, "127.0.0.1", timeout);
    //     }
    // }
    return 0;
}
