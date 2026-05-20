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

#define MAX_CLIENT 50
#define PORT 8080
int counter_client = 0;
void * handle_client(void * client_fd)
{
    int fd_client = *(int *)client_fd;
    
    while(1)
    {
    
        char buffer[1024] = {0};
    
        recv(fd_client,buffer,1024,0);
        
        if(strcmp(buffer,"q!") == 0)
        { 
            printf("Closed !!!\n");
            close(fd_client);
            break;
        }

        printf("Client_%d:%s\n",fd_client,buffer);
    

        char  * msg = "successfully connect !!!";
        printf("Server_%d:%s\n",*(int*)client_fd,msg);
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
 
        send(*(int *)client_fd,msg,strlen(msg),0); 
    }
    close(*(int *)client_fd);
    counter_client--;
    free(client_fd);
    
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
    int client_fds[MAX_CLIENT] = {0};
    listen(fd_server,5);

    
    
    struct sockaddr_in client; 
    socklen_t client_size = sizeof(client);
    // tao 1 socket dung rieng cho viec giao tiep
        
    while(1)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_server,&readfds);
        int max_fd = fd_server;
        int * client_fd = malloc(sizeof(int));// cap phat dong nham tranh viec 2 thread cung dung chung 1 dia chi 
        for(int i = 0; i < MAX_CLIENT; i++)
        {
            if(client_fds[i] == 0)
            {
                continue;
            }
            FD_SET(client_fds[i],&readfds);
            if(client_fds[i] > max_fd)
            {
                max_fd = client_fds[i];
            }
        }
        select(max_fd + 1,&readfds, NULL,NULL,NULL);
        if(FD_ISSET(fd_server,&readfds))
        {
            *client_fd = accept(fd_server,(struct sockaddr *)&client,&client_size);
            fcntl(*client_fd,F_SETFL,O_NONBLOCK);
            for(int i = 0; i < MAX_CLIENT; i++)
            {
                if(client_fds[i] == 0)
                {
                    client_fds[i] = *client_fd;
                    break;
                }
            }
        }
        for(int i = 0; i < MAX_CLIENT; i++)
        {
            if(client_fds[i] > 0 && FD_ISSET(client_fds[i],&readfds))
            {
                handle_client(&client_fds[i]);
            }
        }
        
        counter_client++;
        printf("COUNTER_CLIENT:%d\n",counter_client);
    }
        
    close(fd_server);

}
