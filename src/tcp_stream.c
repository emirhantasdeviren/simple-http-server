#include <unistd.h>
#include <stdlib.h>

#include "tcp_stream.h"

isize stream_read(TcpStream *stream, u8 *buf, usize len) {
    return read(stream->fd, buf, len);
}

isize stream_write(TcpStream *stream, const u8 *buf, usize len) {
    return write(stream->fd, buf, len);
}

void stream_destroy(TcpStream *stream) {
    close(stream->fd);
    free(stream);
}
