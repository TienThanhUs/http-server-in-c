#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>

#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>

#define BUFFER_SIZE 8192

typedef enum{
    PARSE_STATUS_LINE,
    PARSE_HEADERS,
    PARSE_BODY,
    PARSE_DONE
}ParseState;



typedef struct{
    char version[32];
    char status[64];

    char content_type[64];
    int content_length;

    char body[1024];

}HttpResponse;



typedef struct{

    int fd;

    char read_buffer[BUFFER_SIZE];

    int bytes_read;
    int parsed_offset;

    ParseState parse_state;

    HttpResponse response;

}ClientConnection;



// =====================================
// PARSER UTILS
// =====================================

int find_crlf(
    char *buffer,
    int start,
    int end
)
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



// =====================================
// PARSE STATUS LINE
// =====================================

int parse_status_line(ClientConnection *conn)
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
        "%31s %63[^\r]",
        conn->response.version,
        conn->response.status
    );

    conn->parsed_offset = line_end + 2;

    conn->parse_state = PARSE_HEADERS;

    return 1;
}



// =====================================
// PARSE HEADERS
// =====================================

int parse_headers(ClientConnection *conn)
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

        // empty line
        if(line_end == conn->parsed_offset)
        {
            conn->parsed_offset += 2;

            if(conn->response.content_length > 0)
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

        if(strncmp(header_line,"Content-Type:",13) == 0)
        {
            sscanf(
                header_line,
                "Content-Type: %63s",
                conn->response.content_type
            );
        }

        else if(strncmp(header_line,"Content-Length:",15) == 0)
        {
            conn->response.content_length =
                atoi(header_line + 16);
        }

        conn->parsed_offset = line_end + 2;
    }
}



// =====================================
// PARSE BODY
// =====================================

int parse_body(ClientConnection *conn)
{
    int remain =
        conn->bytes_read - conn->parsed_offset;

    if(remain < conn->response.content_length)
    {
        return 0;
    }

    memcpy(
        conn->response.body,
        conn->read_buffer + conn->parsed_offset,
        conn->response.content_length
    );

    conn->response.body[
        conn->response.content_length
    ] = '\0';

    conn->parsed_offset +=
        conn->response.content_length;

    conn->parse_state = PARSE_DONE;

    return 1;
}



// =====================================
// MAIN HTTP PARSER
// =====================================

int parse_http_response(ClientConnection *conn)
{
    while(1)
    {
        if(conn->parse_state == PARSE_STATUS_LINE)
        {
            if(!parse_status_line(conn))
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



// =====================================
// INIT
// =====================================

void init_client_connection(
    ClientConnection *conn,
    int fd
)
{
    memset(conn,0,sizeof(ClientConnection));

    conn->fd = fd;

    conn->parse_state = PARSE_STATUS_LINE;
}



// =====================================
// BUILD REQUEST
// =====================================

void build_http_request(
    char *request_buffer,
    const char *method,
    const char *body
)
{
    int body_length = strlen(body);

    sprintf(
        request_buffer,
        "%s /Course HTTP/1.1\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        method,
        body_length,
        body
    );
}



// =====================================
// RECEIVE FULL RESPONSE
// =====================================

int receive_http_response(ClientConnection *conn)
{
    while(1)
    {
        int n = recv(
            conn->fd,
            conn->read_buffer + conn->bytes_read,
            BUFFER_SIZE - conn->bytes_read,
            0
        );

        if(n <= 0)
        {
            return 0;
        }

        conn->bytes_read += n;

        if(parse_http_response(conn))
        {
            return 1;
        }
    }
}



// =====================================
// MAIN
// =====================================

int main()
{
    int client_fd =
        socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr =
        inet_addr("127.0.0.1");

    connect(
        client_fd,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );

    while(1)
    {
        char method_input;

        printf("\nChoose method:\n");
        printf("g -> GET\n");
        printf("p -> POST\n");
        printf("d -> DELETE\n");
        printf("> ");

        scanf("%c",&method_input);

        while(getchar() != '\n');

        char method[16] = {0};

        if(method_input == 'g')
        {
            strcpy(method,"GET");
        }
        else if(method_input == 'p')
        {
            strcpy(method,"POST");
        }
        else if(method_input == 'd')
        {
            strcpy(method,"DELETE");
        }
        else{
            printf("Invalid method\n");
            continue;
        }

        char body[1024] = {0};

        printf("Body: ");

        fgets(body,sizeof(body),stdin);

        body[strcspn(body,"\n")] = '\0';

        char request[2048] = {0};

        build_http_request(
            request,
            method,
            body
        );

        send(
            client_fd,
            request,
            strlen(request),
            0
        );

        ClientConnection conn;

        init_client_connection(
            &conn,
            client_fd
        );

        if(!receive_http_response(&conn))
        {
            printf("Server disconnected\n");
            break;
        }

        printf("\n===== SERVER RESPONSE =====\n");

        printf(
            "VERSION : %s\n",
            conn.response.version
        );

        printf(
            "STATUS  : %s\n",
            conn.response.status
        );

        printf(
            "TYPE    : %s\n",
            conn.response.content_type
        );

        printf(
            "BODY    : %s\n",
            conn.response.body
        );
    }

    close(client_fd);

    return 0;
}
