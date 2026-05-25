#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>

#include<sys/socket.h>
#include<sys/types.h>
#include<sys/event.h>

#include<netinet/in.h>
#include<arpa/inet.h>

#define SERVER_PORT 8080
#define MAX_EVENTS 32
#define MAX_CONNECTION 8192
#define BUFFER_SIZE 8192

typedef enum{
    PARSE_REQUEST_LINE,
    PARSE_HEADERS,
    PARSE_BODY,
    PARSE_DONE
}ParseState;

typedef struct{
    char method[16];
    char path[256];
    char version[32];

    int content_length;

    char body[1024];
}HttpRequest;

typedef struct{
    int fd;

    char read_buffer[BUFFER_SIZE];

    int bytes_read;
    int parsed_offset;

    ParseState parse_state;

    HttpRequest request;

}Connection;

Connection connections[MAX_CONNECTION] = {0};



// =========================
// SOCKET UTILS
// =========================

void set_nonblocking(int fd)
{
    fcntl(fd,F_SETFL,O_NONBLOCK);
}



// =========================
// CONNECTION
// =========================

void init_connection(Connection *conn,int fd)
{
    memset(conn,0,sizeof(Connection));

    conn->fd = fd;
    conn->parse_state = PARSE_REQUEST_LINE;
}



// =========================
// PARSER
// =========================

int find_crlf(char *buffer,int start,int end)
{
    for(int i = start; i < end - 1; i++)
    {
        if(buffer[i] == '\r' && buffer[i + 1] == '\n')
        {
            return i;
        }
    }

    return -1;
}



int parse_request_line(Connection *conn)
{
    int line_end = find_crlf(
        conn->read_buffer,
        conn->parsed_offset,
        conn->bytes_read
    );

    if(line_end == -1)
    {
        return 0;
    }

    char line[1024] = {0};

    int length = line_end - conn->parsed_offset;

    memcpy(
        line,
        conn->read_buffer + conn->parsed_offset,
        length
    );

    sscanf(
        line,
        "%15s %255s %31s",
        conn->request.method,
        conn->request.path,
        conn->request.version
    );

    conn->parsed_offset = line_end + 2;
    conn->parse_state = PARSE_HEADERS;

    return 1;
}



int parse_headers(Connection *conn)
{
    while(1)
    {
        int line_end = find_crlf(
            conn->read_buffer,
            conn->parsed_offset,
            conn->bytes_read
        );

        if(line_end == -1)
        {
            return 0;
        }

        // empty line => end header
        if(line_end == conn->parsed_offset)
        {
            conn->parsed_offset += 2;

            if(conn->request.content_length > 0)
            {
                conn->parse_state = PARSE_BODY;
            }
            else{
                conn->parse_state = PARSE_DONE;
            }

            return 1;
        }

        char header_line[1024] = {0};

        int length = line_end - conn->parsed_offset;

        memcpy(
            header_line,
            conn->read_buffer + conn->parsed_offset,
            length
        );

        if(strncmp(header_line,"Content-Length:",15) == 0)
        {
            conn->request.content_length = atoi(header_line + 16);
        }

        conn->parsed_offset = line_end + 2;
    }
}



int parse_body(Connection *conn)
{
    int remain = conn->bytes_read - conn->parsed_offset;

    if(remain < conn->request.content_length)
    {
        return 0;
    }

    memcpy(
        conn->request.body,
        conn->read_buffer + conn->parsed_offset,
        conn->request.content_length
    );

    conn->request.body[conn->request.content_length] = '\0';

    conn->parsed_offset += conn->request.content_length;

    conn->parse_state = PARSE_DONE;

    return 1;
}



int parse_http_request(Connection *conn)
{
    while(1)
    {
        if(conn->parse_state == PARSE_REQUEST_LINE)
        {
            if(!parse_request_line(conn))
            {
                return 0;
            }
        }

        else if(conn->parse_state == PARSE_HEADERS)
        {
            if(!parse_headers(conn))
            {
                return 0;
            }
        }

        else if(conn->parse_state == PARSE_BODY)
        {
            if(!parse_body(conn))
            {
                return 0;
            }
        }

        else if(conn->parse_state == PARSE_DONE)
        {
            return 1;
        }
    }
}



// =========================
// RESPONSE
// =========================

void send_response(Connection *conn)
{
    char response[2048] = {0};

    char *message = "Unknown";

    if(strcmp(conn->request.method,"GET") == 0)
    {
        message = "Resource fetched";

        sprintf(
            response,
            "%s 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %ld\r\n"
            "\r\n"
            "%s",
            conn->request.version,
            strlen(message),
            message
        );
    }

    else if(strcmp(conn->request.method,"POST") == 0)
    {
        message = "Resource created";

        sprintf(
            response,
            "%s 201 Created\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %ld\r\n"
            "\r\n"
            "%s",
            conn->request.version,
            strlen(message),
            message
        );
    }

    else if(strcmp(conn->request.method,"DELETE") == 0)
    {
        message = "Resource deleted";

        sprintf(
            response,
            "%s 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %ld\r\n"
            "\r\n"
            "%s",
            conn->request.version,
            strlen(message),
            message
        );
    }

    send(conn->fd,response,strlen(response),0);
}



// =========================
// CLIENT HANDLER
// =========================

void close_client(
    int kq,
    int client_fd
)
{
    struct kevent ev;

    EV_SET(
        &ev,
        client_fd,
        EVFILT_READ,
        EV_DELETE,
        0,
        0,
        NULL
    );

    kevent(kq,&ev,1,NULL,0,NULL);

    close(client_fd);

    printf("Client %d disconnected\n",client_fd);
}



void handle_client_event(
    int kq,
    int client_fd
)
{
    Connection *conn = &connections[client_fd];

    int n = recv(
        client_fd,
        conn->read_buffer + conn->bytes_read,
        BUFFER_SIZE - conn->bytes_read,
        0
    );

    if(n == 0)
    {
        close_client(kq,client_fd);
        return;
    }

    if(n < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return;
        }

        perror("recv");
        close_client(kq,client_fd);
        return;
    }

    conn->bytes_read += n;

    if(!parse_http_request(conn))
    {
        return;
    }

    printf("\n===== HTTP REQUEST =====\n");

    printf("METHOD  : %s\n",conn->request.method);
    printf("PATH    : %s\n",conn->request.path);
    printf("VERSION : %s\n",conn->request.version);
    printf("BODY    : %s\n",conn->request.body);

    send_response(conn);

    close_client(kq,client_fd);
}



// =========================
// SERVER
// =========================

int main()
{
    int server_fd = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(
        server_fd,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );

    listen(server_fd,5);

    set_nonblocking(server_fd);

    int kq = kqueue();

    struct kevent server_event;

    EV_SET(
        &server_event,
        server_fd,
        EVFILT_READ,
        EV_ADD,
        0,
        0,
        NULL
    );

    kevent(kq,&server_event,1,NULL,0,NULL);

    printf("Server listening at port %d\n",SERVER_PORT);

    while(1)
    {
        struct kevent events[MAX_EVENTS];

        int event_count = kevent(
            kq,
            NULL,
            0,
            events,
            MAX_EVENTS,
            NULL
        );

        for(int i = 0; i < event_count; i++)
        {
            int current_fd = events[i].ident;

            // new client
            if(current_fd == server_fd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_size = sizeof(client_addr);

                int client_fd = accept(
                    server_fd,
                    (struct sockaddr*)&client_addr,
                    &client_size
                );

                set_nonblocking(client_fd);

                init_connection(
                    &connections[client_fd],
                    client_fd
                );

                struct kevent client_event;

                EV_SET(
                    &client_event,
                    client_fd,
                    EVFILT_READ,
                    EV_ADD,
                    0,
                    0,
                    NULL
                );

                kevent(kq,&client_event,1,NULL,0,NULL);

                printf(
                    "New client: %d\n",
                    client_fd
                );
            }

            else{
                handle_client_event(
                    kq,
                    current_fd
                );
            }
        }
    }

    close(server_fd);

    return 0;
}
