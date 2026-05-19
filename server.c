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


pthread_mutex_t m;
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
    pthread_mutex_lock(&m);
    counter_client--;
    pthread_mutex_unlock(&m);
    free(client_fd);
    
    return NULL;


    
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
    
    // lang nghe yeu cau ket noi tu client
    listen(fd_server,5);

    fcntl(fd_server,F_SETFL,O_NONBLOCK);
    // thread quan ly socket server khong ngu de xu ly cac socket khac thay vi chi ngu de xu ly server socket 
    
    
    struct sockaddr_in client; 
    socklen_t client_size = sizeof(client);
    // tao 1 socket dung rieng cho viec giao tiep
        
    while(1)
    {
        int * client_fd = malloc(sizeof(int));// cap phat dong nham tranh viec 2 thread cung dung chung 1 dia chi 
        *client_fd = accept(fd_server,(struct sockaddr*)&client,&client_size);
        fcntl(*client_fd,F_SETFL,O_NONBLOCK);
        pthread_mutex_lock(&m);
        counter_client++;
        pthread_mutex_unlock(&m);
        printf("COUNTER_CLIENT:%d\n",counter_client);
        pthread_t thread;
        pthread_create(&thread,NULL,handle_client,client_fd);
        pthread_detach(thread); // Khong cho thread khac dung chung tai nguyen 

    }
        
    close(fd_server);

}
