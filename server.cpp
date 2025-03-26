// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <cstdio>
#include "picohttpparser.h"
#include <assert.h>
#include <string>
#include <vector>
#include <memory>

using namespace std;

bool SetSocketBlockingEnabled(int fd, bool blocking)
{
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0);
}

int CreateServer() {
    const int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        throw std::runtime_error("Socket creation failed: " + std::string(strerror(errno)));
    }

    if(!SetSocketBlockingEnabled(serverSocket, false)) return -1;

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        close(serverSocket);
        throw std::runtime_error("Binding failed: " + std::string(strerror(errno)));
    }
    
    if (listen(serverSocket, 5) == -1) {
        close(serverSocket);
        throw std::runtime_error("Listening failed: " + std::string(strerror(errno)));
    }
    
    return serverSocket;
}

void ParseHttpRequest(int sock) {
    char buf[4096];
    const char *method, *path; 
    int pret, minor_version;
    struct phr_header headers[100];
    size_t buflen = 0, prevbuflen = 0, method_len, path_len, num_headers;
    ssize_t rret;
    int i;

    while (1) {
        while ((rret = read(sock, buf + buflen, sizeof(buf) - buflen)) == -1 && errno == EINTR);
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



struct Req {
    std::string method;
    std::string path;
    std::string contentType;
    bool upgrade = false;
};

struct Res {
    
};

template <typename Callback>
void get(pair<Req, Res>& http , const Callback& callback) {
    
}

int main()
{
    const int serverSocket = CreateServer();
    
    
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
		printf("Couldn't create epoll instance!\n");
        return 1;
	}
    
    struct epoll_event serverEvent;
    serverEvent.data.fd = serverSocket;
    serverEvent.events = EPOLLIN;

    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &serverEvent) == -1) {
        printf("Couldn't add to epoll!\n");
        return 1;
    }

    struct epoll_event events[10];
    
    while(true) {
        int eventsReady = epoll_wait(epollFd, events, 10, -1);

        for(int i = 0;i < eventsReady;i++) {
            if(events[i].data.fd == serverSocket) {
                while(true) {
                    int clientFd = accept(serverSocket, nullptr, nullptr);
                    if (clientFd < 0) {
                        perror("accept failed");
                        break;
                    }

                    printf("Client Accepted");
                    
                    struct epoll_event clientEvent;
                    clientEvent.data.fd = clientFd;
                    clientEvent.events = EPOLLIN;
                    
                    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent) == -1) {
                        throw std::runtime_error("Error cleitn can't be added to epooll: " + std::string(strerror(errno)));
                    }

                    printf("client added to epoll tree");
                }
            } else {
                char buffer[1024] = {0};
                int bytesRead = recv(events[i].data.fd, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0) {
                    printf("Client disconnected: %d\n", events[i].data.fd);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                    close(events[i].data.fd);
                } else {
                    printf("Received: %s\n", buffer);
                } 
            }
        }
    }
    
    return 0;
}


template <typename Callback> 
class Node {
public:
    Node() : neighbors(26,nullptr), callback(nullptr), (false) { }
    std::vector<std::shared_ptr<Node>> neighbors;
    Callback callback;
    bool path;
    
};

template <typename Callback> 
class Trie {
public:
    Trie() : root(std::make_shared<Node>()) {}

    void insert(std::string& path, Callback& function) {
        std::shared_ptr<Node> curr = root;
        for(char& c : path) {
            if(!curr->neighbors[c - 'a']) {
                curr->neighbors[c - 'a'] = std::make_shared<Node>();
            } 
            curr = curr->neighbors[c - 'a'];
        }

        curr->path = true;
        if(function)
            curr->callback = function;

    }

    bool search(string& path) {
        std::shared_ptr<Node> curr = root;

        for(char& c : path) {
            if(!curr->neighbors[c - 'a']) return false;
        }

        if(!curr->path) return false;

        return true;
    }

private:
    std::shared_ptr<Node> root;
};