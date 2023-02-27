#pragma once

#include "defines.h"

typedef struct {
    int fd;
} TcpStream;

isize stream_read(TcpStream *stream, u8 *buf, usize len);
isize stream_write(TcpStream *stream, const u8 *buf, usize len);
void stream_destroy(TcpStream *stream);
