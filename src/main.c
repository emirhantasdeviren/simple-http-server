#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tcp_listener.h"
#include "tcp_stream.h"

#define BUFLEN 1024

typedef struct {
    u8 *data;
    usize len;
} HttpResponse;
typedef u8 *HttpRequest;

typedef enum {
    GET,
    POST
} HttpMethod;

typedef struct {
    char *route;
    HttpMethod method;
    HttpResponse (*handle)(HttpRequest);
} HttpService;

typedef struct {
    TcpListener *listener;
    HttpService *services;
    usize service_count;
} HttpServer;

void add_service(
    HttpService **services,
    usize *service_count,
    const char *route,
    HttpMethod method,
    HttpResponse (*handle)(HttpRequest))
{
    if (service_count) {
        *services = realloc(*services, ((*service_count) + 1) * sizeof(HttpService));
        if (*services) {
            *service_count += 1;
            HttpService *new_service = *services + *service_count - 1;
            new_service->route = (char *)malloc(sizeof(char) * strlen(route) + 1);
            if (new_service->route) {
                strcpy(new_service->route, route);
            }
            new_service->method = method;
            new_service->handle = handle;
        }
    }
}

HttpResponse index(HttpRequest req) {
    int fd = open("public/index.html", O_RDONLY);
    if (fd != -1) {
        struct stat statbuf;
        fstat(fd, &statbuf);

        char header[BUFLEN];
        i32 len = snprintf(
            header,
            BUFLEN,
            "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %zi\nAccept-Ranges: bytes\n\n",
            statbuf.st_size
        );

        u8 *data = (u8 *)malloc(sizeof(u8) * (len + statbuf.st_size));
        if (data) {
            memcpy(data, header, len);
            read(fd, data + len, statbuf.st_size);

            HttpResponse res = {
                .data = data,
                .len = len + statbuf.st_size
            };
            return res;
        } else {
            HttpResponse res = {
                .data = NULL,
                .len = 0
            };
            return res;
        }
    } else {
        HttpResponse res = {
            .data = NULL,
            .len = 0
        };
        return res;
    }
}

HttpServer server_bind(const char *addr) {
    TcpListener *listener = listener_bind(addr);
    HttpServer server = {
        .listener = listener,
        .services = NULL,
        .service_count = 0
    };

    return server;
}

HttpRequest recv_request(TcpStream *stream) {
    char buf[1024];
    isize bytes_read = stream_read(stream, (u8 *)buf, 1024);
    if (bytes_read != -1) {
        HttpRequest req = (u8 *)malloc(sizeof(u8) * bytes_read);
        if (req) {
            memcpy(req, buf, bytes_read);
        }
        return req;
    } else {
        return NULL;
    }
}

void server_serve(HttpServer *server, HttpService *services, usize service_count) {
    server->services = services;
    server->service_count = service_count;

    TcpStream *stream = listener_accept(server->listener);
    while (stream) {
        HttpRequest req = recv_request(stream);
        if (req) {
            HttpMethod method;
            const char *end = (const char *)req;
            while (*end++ != ' ') {}
            if (end - (const char *)req == 4) {
                method = GET;
            } else {
                method = POST;
            }
            while(*end++ != '/') {}
            const char *begin = end;
            while(*end++ != ' ') {}

            const char *route = begin - 1;
            usize route_len = end - begin;
            
            char *path = malloc((7 + route_len) * sizeof(char));
            if (path) {
                memcpy(path, "public", 6);
                memcpy(path + 6, route, route_len);
                *(path + 6 + route_len) = 0;

                int found_service = 0;
                for (usize i = 0; i < service_count; i++) {
                    if (server->services[i].method == method
                        && strlen(server->services[i].route) == route_len
                        && strncmp(server->services[i].route, route, route_len) == 0)
                    {
                        found_service = 1;
                        HttpResponse res = server->services[i].handle(req);
                        stream_write(stream, res.data, res.len);
                        free(res.data);
                        break;
                    }
                }

                if (!found_service && method == GET) {
                    char *content_type = NULL;
                    for (usize i = route_len - 1; i > 0; i--) {
                        if (*(route + i) == '.') {
                            if (strncmp(route + i + 1, "css", 3) == 0) {
                                content_type = "text/css";
                            } else if (strncmp(route + i + 1, "ico", 3) == 0) {
                                content_type = "image/x-icon";
                            } else if (strncmp(route + i + 1, "html", 4) == 0) {
                                content_type = "text/html";
                            }
                            break;
                        }
                    }
                    int fd = open(path, O_RDONLY);
                    if (fd != -1) {
                        struct stat statbuf;
                        fstat(fd, &statbuf);
                        char header[BUFLEN];
                        i32 len = snprintf(
                            header,
                            BUFLEN,
                            "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %zi\nAccept-Ranges: bytes\n\n",
                            content_type, statbuf.st_size
                        );
                        
                        char *data = malloc((len + statbuf.st_size) * sizeof(char));
                        memcpy(data, header, len);
                        read(fd, data + len, statbuf.st_size);
                        stream_write(stream, (const u8 *)data, len + statbuf.st_size);
                        free(data);
                    }
                }

                free(path);
            }
            free(req);
        }
        stream_destroy(stream);
        stream = listener_accept(server->listener);
    }
}

void server_destroy(HttpServer *server) {
    listener_destroy(server->listener);
    free(server->services);
}

int main(void) {
    HttpServer server = server_bind("127.0.0.1:8080");

    if (server.listener) {
        HttpService *services = NULL;
        usize service_count = 0;

        add_service(&services, &service_count, "/", GET, index);

        server_serve(&server, services, service_count);
        server_destroy(&server);
    }

    return 0;
}
