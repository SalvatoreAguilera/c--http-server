#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include "picohttpparser.h"  // Include picohttpparser

#define PORT 8080
#define BACKLOG 10  // Max queued connections
#define MAX_HEADERS 100

void handle_client(int sock) {
    char buf[4096];
    const char *method, *path;  // <-- Change char* to const char*
    int pret, minor_version;
    struct phr_header headers[100];
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;
    int i;

    while (1) {
        while ((rret = read(sock, buf + buflen, sizeof(buf) - buflen)) == -1 && errno == EINTR)
            ;
        if (rret <= 0) {
            perror("Read error or client disconnected");
            close(sock);
            return;
        }
        prevbuflen = buflen;
        buflen += rret;

        num_headers = sizeof(headers) / sizeof(headers[0]);
        pret = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                                 &minor_version, headers, &num_headers, prevbuflen);
        if (pret > 0)
            break;
        else if (pret == -1) {
            fprintf(stderr, "Parsing error\n");
            close(sock);
            return;
        }

        assert(pret == -2);
        if (buflen == sizeof(buf)) {
            fprintf(stderr, "Request is too long\n");
            close(sock);
            return;
        }
    }

    printf("Request is %d bytes long\n", pret);
    printf("Method: %.*s\n", (int)method_len, method);
    printf("Path: %.*s\n", (int)path_len, path);
    printf("HTTP Version: 1.%d\n", minor_version);
    printf("Headers:\n");
    for (i = 0; i < num_headers; ++i) {
        printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name,
               (int)headers[i].value_len, headers[i].value);
    }

    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    write(sock, response, strlen(response));

    close(sock);
}


/* Basic TCP Server */
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    /* Create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Bind socket */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Listen for connections */
    if (listen(server_fd, BACKLOG) < 0) {
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    /* Accept connections */
    while ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) >= 0) {
        printf("Accepted new connection\n");
        handle_client(client_fd);
    }

    perror("Accept failed");
    close(server_fd);
    return 0;
}