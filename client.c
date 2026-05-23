#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>

void parseHTTPmsg(const char * buffer, char * payload)
{
    while(*buffer != '=')
    {
        buffer++;
    }
    buffer++;
    while(*buffer != '\n')
    {
        buffer++;
    }
    buffer++;
    while(*buffer != '=')
    {
        buffer++;
    }
    buffer++;
    while(*buffer != '\n')
    {
        buffer++;
    }
    buffer++;
    int idx = 0; 
    while(*buffer != '\r') 
    {
        payload[idx++] = *buffer;
        buffer++;
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
    
    

    connect(fd_client,(struct sockaddr *)&server_addr,sizeof(server_addr));
    while(1)  
    {
        printf("Client:");
        fflush(stdout);
        char msg[1024] = {0};
        int c;
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
        sprintf(req,"method=POST\ncontent_length=%d\n%s\r\n",n,msg);
        send(fd_client ,req, strlen(req),0);

        char buffer[1024] = {0};

        recv(fd_client,buffer,1024,0);
        char res[512] = {0};
        parseHTTPmsg(buffer,res);

        printf("Server:%s\n",res);
    }

    close(fd_client);
}
