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
void * handle_client(int client_fd, int * client_fds, const int * idx)
{
    
    
        char buffer[1024] = {0};
    
        int n = recv(client_fd,buffer,1024,0);
        if(n == 0)
        {
            printf("Client disconnect !!!\n");
            close(client_fd);
            client_fds[*idx] = 0;
            counter_client--;
            return NULL;
        }
        if(n < 0)
        {
            if(errno = EAGAIN || errno = EWOULDBLOCK)
            {
                return NULL;
            }
            perror("recv");
            return NULL;
        }
        
        if(strcmp(buffer,"q!") == 0)
        { 
            printf("Closed !!!\n");
            close(client_fd);
            client_fds[*idx] = 0;
            counter_client--; 
            return NULL;
        }

        printf("Client_%d:%s\n",client_fd,buffer);
    

        char  * msg = "successfully connect !!!";
        printf("Server_%d:%s\n",client_fd,msg);
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
 
        send(client_fd,msg,strlen(msg),0); 
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
        int client_fd;
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
            client_fd = accept(fd_server,(struct sockaddr *)&client,&client_size);
            counter_client++;
            printf("COUNTER_CLIENT:%d\n",counter_client);
            fcntl(client_fd,F_SETFL,O_NONBLOCK);
            for(int i = 0; i < MAX_CLIENT; i++)
            {
                if(client_fds[i] == 0)
                {
                    client_fds[i] = client_fd;
                    break;
                }
            }
        }
        for(int i = 0; i < MAX_CLIENT; i++)
        {
            if(client_fds[i] > 0 && FD_ISSET(client_fds[i],&readfds))
            {
                handle_client(client_fds[i],client_fds,&i);
                break;
            }
        }
        
    }
        
    close(fd_server);

}
