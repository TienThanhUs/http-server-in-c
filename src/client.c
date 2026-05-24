#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>


typedef struct{
    char status_line[20];
    char content_type[20];
    int content_length;
    char payload[200];
    char version[20];
}Connection;

void parseHTTPmsg(Connection * conn, char * buffer)
{
    char * pkg = buffer;
    int idx = 0;
    //PARSE STATUS LINE
    while(*pkg != ' ')
    {
        conn->version[idx++] = *pkg; 
        pkg++; 
    }
    pkg++;// bo qua dau cach
   
    idx = 0; 
    while(*pkg != '\r')
    {
        conn->status_line[idx++] = *pkg;
        pkg++;
    }
    pkg += 2;
    // bo qua dau \n;


    //PARSE CONTENT TYPE
    while(*pkg != ':')
    {
        pkg++;
    }
    pkg += 2;


    idx = 0;
    while(*pkg != '\r')
    {
        conn->content_type[idx++] = *pkg;
        pkg++;
    }
    pkg += 2; 
    //PARSE CONTENT_LENGTH
    while(*pkg != ':')
    {
        
        pkg++;
    }
    
    pkg += 2;// bo qua dau cach
    
    idx = 0;
    char content_length[10] = {0};
    while(*pkg != '\r')
    {
        content_length[idx++] = *pkg;
        pkg++;
    }
    pkg += 2;// bo qua dau \n
    pkg += 2;// bo qua \r\n vi theo cau truc goi tin thi sau content length se la 2 phan \r\n


    //PARSE PAYLOAD
    idx = 0; 
    while(*pkg != '\0') 
    {
        conn -> payload[idx++] = *pkg;
        pkg++;
    }
}

int main(){
    struct sockaddr_in client_addr;
    int fd_client = socket(AF_INET,SOCK_STREAM,0);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(3000);
    client_addr.sin_addr.s_addr = INADDR_ANY;

    bind(fd_client,(struct sockaddr *)&client_addr,(socklen_t)sizeof(client_addr));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    
    Connection conn = {0};
    
    

    connect(fd_client,(struct sockaddr *)&server_addr,sizeof(server_addr));
    while(1)  
    {
        printf("Choose your method:");
        char method = getc(stdin) ;
        int c;
        while(( c = getchar()) != '\n'){}
        fflush(stdout);

        
        printf("Client:");
        fflush(stdout);
        char msg[1024] = {0};
        int i = 0;
        while((c = getc(stdin)) != '\n' && c != EOF)
        {
           if(i >= 1024) 
           {
               break;
           }
           msg[i++] = c;
        }
        if(strcmp(msg,"q!") == 0) 
        {
            
            send(fd_client , msg, strlen(msg),0);
            printf("Dang thoat ...\n");
            break;
        }
        int n = strlen(msg);
        char req[1024] = {0};
        char * str_method = "method";
        if(method == 'p')
        {
            str_method = "POST";
        }
        else if(method == 'g')
        {
            str_method = "GET";
        }
        else if(method == 'd'){
            str_method = "DELETE";
        }
        
        sprintf(req,"%s /Course HTTP/1.1\r\ncontent_length : %d\r\n\r\n%s",str_method,n,msg);
        send(fd_client ,req, strlen(req),0);

        char buffer[1024] = {0};

        recv(fd_client,buffer,1024,0);
        parseHTTPmsg(&conn,buffer);

        printf("Server:%s\n",conn.payload);
    }

    close(fd_client);
}
