#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/event.h>
#include<sys/time.h>
#include<fcntl.h>
#include<sys/select.h>
#include<sys/errno.h>

#define MAX_CLIENT 36
#define PORT 8080
#define MAX_FD 8192

// da tao xong Struct Connection hoan chinh va khoi tao gia tri mac dinh
// Sua ham handle client de co the xu ly tung streambyte thay vi happy case là nhan het request trong 1 luot 
// Moi lan doc den \r\n thi doi state 
// Su dung cac method chinh nhu GET POST, DELETE 
// Cau truc goi tin 

typedef enum{
    PARSE_INIT,
    PARSE_REQUEST_LINE,
    PARSE_HEADERS,
    PARSE_BODY,
    PARSE_DONE
}ParseState;


typedef struct{
    char buffer[8192];
    int read_pos; 
    int parsed_pos;
    ParseState state;
    int content_length;
    char  payload[256];
    char version[16];
    char path[256];
    char method[16];

}Connection;


Connection Conns[MAX_FD] = {0};


void parseHTTPmsg(Connection * client,char * msg)
{
    char * temp = msg; 
    //PARSE PATH
    int idx = 0;
    while(*temp != ' ')
    {
        client->method[idx++] = *temp;
        temp++;
    }
    //temp dang o vi tri cua dau cach
    temp++;
    idx = 0;
    while(*temp != ' ') 
    {
        client->path[idx++] = *temp;
        temp++;
    }
    temp++;
    idx = 0;
    while(*temp != '\r')
    {
        client->version[idx++] = *temp;
        temp++; 
    }
    temp += 2;// bo qua ki tu \n
    idx = 0;
    //ket thuc phan method




    //PARSE CONTENT_LENGTH
    while(*temp != ':')
    {
        temp++;
    }
    //temp dang o vi tri cua dau bang
    temp += 2;
    char contentLength[10] = {0};
    idx = 0;
    while(*temp != '\r')
    {
        contentLength[idx++ ] = *temp; 
        temp++;
    }
    client->content_length = atoi(contentLength);
    temp += 4;// bo qua dau \n







    // PARSE PAYLOAD
    idx = 0;
    while(*temp != '\0') 
    {
        client->payload[idx++] = *temp;
        temp++; 
    }
    
}



int counter_client = 0;
void * handle_client(Connection * conn,const int * kq,int client_fd, struct kevent * events, const int * idx)
{
    
    
        char * buffer = conn->buffer;
    
        int n = recv(client_fd,buffer,1024,0);
        if(n == 0)
        {
            printf("Client disconnect !!!\n");
            EV_SET(&events[*idx],client_fd,EVFILT_READ,EV_DELETE,0,0,NULL);
            kevent(*kq,&events[*idx],1,NULL,0,NULL);
            close(client_fd);
            counter_client--;
            return NULL;
        }
        if(n < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return NULL;
            }
            perror("recv");
            return NULL;
        }
        
        if(strcmp(buffer,"q!") == 0)
        { 
            printf("Closed !!!\n");
            EV_SET(&events[*idx],client_fd,EVFILT_READ,EV_DELETE,0,0,NULL);
            kevent(*kq,&events[*idx],1,NULL,0,NULL);
            close(client_fd);
            counter_client--;

            return NULL;
        }
        parseHTTPmsg(conn,buffer);

        printf("Client_%d:%s\n",client_fd,conn->payload);
        char res[1024] = {0};

        char * response = "Loi ben server";
    
        if(strcmp(conn->method,"POST") == 0)
        {
            sprintf(res,"%s 201 Created\r\nContent-Type : text/plain\r\nContent-Length : 16\r\n\r\nResource created",conn->version);
            response = "Resource created";


        }


        
        else if(strcmp(conn->method,"GET") == 0)
        {

            sprintf(res,"%s 200 OK\r\nContent-Type : text/plain\r\nContent-Length : 17\r\n\r\nResource fetched",conn->version);
            response = "Resource fetched";

        }
        else if(strcmp(conn->method,"DELETE") == 0){
            sprintf(res,"%s 200 OK\r\nContent-Type : text/plain\r\nContent-Length : 16\r\n\r\nResource deleted",conn->version);
            response = "Resource deleted";
        }
        


        printf("Server_%d:%s\n",client_fd,response);
        fflush(stdout);// ep chuong trinh in ra man hinh thay vi giu trong buffer
//        int c;
//       int i = 0; 
       // while((c = getc(stdin)) != '\n' && c != EOF)
        //{
         //  if(i >= 1024)
           //{
            //   break;
           //}
           //msg[i++] = c;
        //}
 
        send(client_fd,res,strlen(res),0); 
    return NULL;


    
}

int main()
{
    int fd_server = socket(AF_INET,SOCK_STREAM,0);

    //them cau hinh cho server
    struct sockaddr_in server; 
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    // bind port vao cho socket object cua kernel
    bind(fd_server,(struct sockaddr*)&server,(socklen_t)sizeof(server));
    
    fcntl(fd_server,F_SETFL,O_NONBLOCK);
    // thread quan ly socket server khong ngu de xu ly cac socket khac thay vi chi ngu de xu ly server socket 
    // lang nghe yeu cau ket noi tu client
    listen(fd_server,5);

    
    
    struct sockaddr_in client; 
    socklen_t client_size = sizeof(client);
    // tao 1 socket dung rieng cho viec giao tiep
    

    //tao kqueue 
    int kq = kqueue();

    //tao struct kevent 
    struct kevent ke;
    EV_SET(&ke,fd_server,EVFILT_READ,EV_ADD,0,0,NULL);
    kevent(kq,&ke,1,NULL,0,NULL);


    // khoi tao version cho cac connection; 
    for(int i = 0;i < MAX_FD; i++)
    {
        strcpy(Conns[i].version,"HTTP/1.1");
    }


        
    while(1)
    {
        struct kevent events[MAX_CLIENT];
        int n = kevent(kq,NULL,0,events,MAX_CLIENT,NULL);
        for(int i = 0; i < n; i++)
        {
            if(events[i].ident == fd_server)
            {
                struct kevent clientEV;
                int client_fd = accept(fd_server,(struct sockaddr *)&client, &client_size);
                counter_client++;
                printf("COUNTER_CLIENT:%d\n",counter_client);
                fcntl(client_fd,F_SETFL,O_NONBLOCK);
                EV_SET(&clientEV,client_fd,EVFILT_READ,EV_ADD,0,0,NULL);
                kevent(kq,&clientEV,1,NULL,0,NULL);
                continue;
            }
            handle_client(&Conns[events[i].ident],&kq,events[i].ident ,events,&i);
        } 
    }
        
    close(fd_server);

}
