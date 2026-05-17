#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>

pthread_mutex_t m;
int counter_client = 0;
void handle_client(void * client_fd)
{
    while(1)
    {
    
        char buffer[1024] = {0};
    
        recv(*(int *)client_fd,buffer,1024,0);
        if(strcmp(buffer,"q!") == 0)
        { 
            close(*(int *)client_fd);
            break;
        }

        printf("Client_%d:%s\n",*(int *)client_fd,buffer);
    

        printf("Server:");
        fflush(stdout);
        char  msg[1024] = {0};
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
 
        send(*(int *)client_fd,msg,strlen(msg),0); 
    }
    close(*(int *)client_fd);
    pthread_mutex_lock(&m);
    counter_client--;
    pthread_mutex_unlock(&m);

    
}

#define PORT 8080
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
    

    listen(fd_server,5);

    
    struct sockaddr_in client; 
    socklen_t client_size = sizeof(client);
        
    while(1)
    {

        int client_fd = accept(fd_server,(struct sockaddr*)&client,&client_size);
        pthread_mutex_lock(&m);
        counter_client++;
        pthread_mutex_unlock(&m);
        printf("COUTER_CLIENT :%d\n",counter_client);
        char buffer[1024] = {0};
    
        recv(client_fd,buffer,1024,0);
        if(strcmp(buffer,"q!") == 0)
        { 
            close(client_fd);
            break;
        }

        printf("Client:%s\n",buffer);
    

        printf("Server:");
        fflush(stdout);
        char  msg[1024] = {0};
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
 
        send(client_fd,msg,strlen(msg),0);
    
    }
        
    close(fd_server);

}
