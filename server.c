#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    char *request_line = strtok(request, "\r\n"); // crlf
        // GET /index.html HTTP/1.1 
    char method[16], path[256], protocol[16];
    sscanf(request_line, "%s %s %s", method, path, protocol);
    
    free(request);

    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    printf("Protocol: %s\n", protocol);
    
    if (strcmp(method, "GET") != 0) {
        char response[] = "HTTP/1.1 400 Bad Request\r\n\n";
        send(client_socket, response, strlen(response), 0);
    }
    else {
        // Hint: if the requested path is "/" (root), default to index.html
        // char *file_name[] = (strcmp(path, "/") || strcmp(path, "/index.html")) ? path : "/index.html";
        char *file_name;
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            file_name = "/index.html";
        } 
        else {
            file_name = path;
        }

        printf("localhost: %s\n", file_name);
    
        // TODO: Implement proxy and call the function under condition
        // specified in the spec
        // if (need_proxy(...)) {
        //    proxy_remote_file(app, client_socket, file_name);
        // } else {
        serve_local_file(client_socket, file_name);
        //}
    }
    
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response

    char fileURL[256];
    strcpy(fileURL, ".");
    strcat(fileURL, path);

    printf("fileURL: %s\n", fileURL);

    FILE *file = fopen(fileURL, "r"); // file descriptor
    if (!file)
    {
        char response[] = "HTTP/1.1 404 Not Found\r\n\n";
        // printf("failure to open: %s\n", fileURL);
        send(client_socket, response, sizeof(response), 0);
    }
    else {
        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char* buf = malloc(size);
        memset(buf, '\0', file_size); // initialization of buf

        char response[] = "HTTP/1.1 200 OK\r\n"
                      "Content-Type: text/html; charset=UTF-8\r\n" // we need to be able to support many content types
                      "Content-Length: 15\r\n" // change content-length to be correct size
                      "\r\n"
                      "Happy Birthday"; // this must be index.html
        // we need to be able to write from *file into a container
        send(client_socket, response, strlen(response), 0);
        fclose(file);
        free(buf);
    }
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response
    
    char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    send(client_socket, response, strlen(response), 0);
}