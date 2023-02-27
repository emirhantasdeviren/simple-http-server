#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "defines.h"
#include "tcp_listener.h"
#include "tcp_stream.h"

#define BACKLOG 10

TcpListener *listener_bind(const char *addr) {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        return NULL;
    }

    //const u8 HOST[4] = {10, 200, 10, 67};
    //172.20.10.2
    const u8 HOST[4] = {172, 20, 10, 2};
    const u16 PORT = 8080;

    struct sockaddr_in sock_addr = {
        .sin_family = AF_INET,
        .sin_addr = {
            .s_addr = *(u32 *)HOST,
        },
        .sin_port = htons(PORT)
    };

    if (bind(fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != 0) {
        perror("Hmm");
        close(fd);
        return NULL;
    }

    TcpListener *listener = (TcpListener *)malloc(sizeof(TcpListener));
    if (listener) {
        listener->fd = fd;
    } else {
        close(fd);
    }
    return listener;
}

void listener_destroy(TcpListener *listener) {
    close(listener->fd);
    free(listener);
}

TcpStream *listener_accept(TcpListener *listener) {
    if (listen(listener->fd, BACKLOG) == -1) {
        return NULL;
    }

    struct sockaddr_in client = { 0 };
    socklen_t len = sizeof(client);

    int fd = accept(listener->fd, (struct sockaddr *)&client, &len);
    if (fd == -1) {
        return NULL;
    }

    TcpStream *stream = (TcpStream *)malloc(sizeof(TcpStream));
    if (stream) {
        stream->fd = fd;
    }

    return stream;
}
