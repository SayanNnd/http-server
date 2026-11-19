#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192

static void response_generator(char *response, const char* error, const char *method, const char* body);

int main() {
    char buffer[BUFFER_SIZE];

    //Socket creation
    const int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket<0) {
        perror("socket failed to load");
        exit(1);
    }

    //Address Creation
    struct sockaddr_in server_address;
    constexpr socklen_t server_address_len = sizeof(server_address);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(server_address);

    //Binding the socket
    if (bind(server_socket, (struct sockaddr *) &server_address, server_address_len)<0) {
        perror("bind failed");
        exit(1);
    }

    //Listening to Socket
    if (listen(server_socket, SOMAXCONN)<0) {
        perror("listen failed");
        exit(1);
    }

    while (true) {
        //Waiting and Accepting on Socket
        const int incoming = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len);
        *buffer = '\0';
        if (incoming<0) {
            perror("accept failed");
            exit(1);
        }
        printf("connection accepted\n");

        //Reading from socket
        ssize_t total = 0;
        char error=0;
        while (true) {
            const ssize_t valread = read(incoming, buffer+total, BUFFER_SIZE-1-total);
            if (valread<0) {
                perror("read failed");
                error = 1;
                break;
            }
            if (valread==0) break;
            total += valread;
            buffer[total] = '\0';
            const char *found = strstr(buffer, "\r\n\r\n");
            if (found) {
                break;
            }
            if (total >= BUFFER_SIZE-1) {
                error = 1;
                break;
            }
        }
        printf("[%s : %u] ", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        //Parsing the request
        char method[BUFFER_SIZE] = {0};
        char uri[BUFFER_SIZE] = {0};
        char version[BUFFER_SIZE] = {0};
        if (!error) {
            char *eol = strstr(buffer, "\r\n");
            if (!eol) {
                error = 2;
            }
            else {
                char *bufferLine = malloc(eol-buffer+1);
                if (!bufferLine) {
                    perror("malloc failed");
                    exit(1);
                }
                memcpy(bufferLine, buffer, eol-buffer);
                *(bufferLine + (eol-buffer)) = '\0';
                char *first = strchr(bufferLine, ' ');
                if (!first) {
                    error = 2;
                }
                else {
                    memcpy(method, bufferLine, first-bufferLine);
                    *(method + (first-bufferLine)) = '\0';
                    char *second = strchr(first+1, ' ');
                    if (!second) {
                        error = 2;
                    }
                    else {
                        memcpy(uri, first+1, second-first-1);
                        *(uri + (second-first-1)) = '\0';
                        memcpy(version, second+1, strlen(second+1));
                        *(version + (strlen(second+1))) = '\0';
                    }
                }
                free(bufferLine);
            }
        }
        if (!error) {
            printf("%s %s %s\n", method, uri, version);
        }

        //Generating a response
        char *response = malloc(BUFFER_SIZE);
        if (!response) {
            perror("malloc failed");
            exit(1);
        }
        const char *body = "<html>Hello world</html>\r\n";
        response_generator(response, &error, method, body);

        //Writing to socket
        const int valwrite = (int)write(incoming, response, strlen(response));
        free(response);
        if (valwrite<0) {
            perror("write failed");
            close(incoming);
            continue;
        }
        close(incoming);
    }
}

void response_generator(char *response, const char* error, const char *method, const char* body) {
    strcpy(response, "HTTP/1.0 ");
    if (*error==1) strcat(response, "431 Request Header Fields Too Large");
    else if (*error==2) strcat(response, "400 Bad Request");
    else if (strcmp(method,"GET")!=0) strcat(response, "405 Method Not Allowed");
    else strcat(response, "200 OK");

    strcat(response, "\r\n");

    if (*error==0) {
        if (strcmp(method,"GET")!=0) {
            strcat(response, "Allow: GET\r\n");
        }
        else {
            char* len = malloc(12);
            snprintf(len, 12, "%zu", strlen(body));
            strcat(response, "Content-Type: text/html\r\n"
                "Content-Length: ");
            strcat(response, len);
            strcat(response, "\r\n\r\n");
            strcat(response, body);
            free(len);
            return;
        }
    }
    strcat(response, "Content-Length: 0\r\n\r\n");
}
