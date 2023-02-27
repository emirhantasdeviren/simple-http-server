#pragma once

#include "tcp_stream.h"

typedef struct {
    int fd;
} TcpListener;

TcpListener *listener_bind(const char *addr);
void listener_destroy(TcpListener *listener);
TcpStream *listener_accept(TcpListener *listener);
