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
int counter_client = 0;
void * handle_client(const int * kq,int client_fd, struct kevent * events, const int * idx)
{
    
    
        char buffer[1024] = {0};
    
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


        
    while(1)
    {
        struct kevent events[36];
        int n = kevent(kq,NULL,0,events,36,NULL);
        for(int i = 0; i < n; i++)
        {
            if(events[i].ident == fd_server)
            {
                struct kevent clientEV;
                int client_fd = accept(fd_server,(struct sockaddr *)&client, &client_size);
                fcntl(client_fd,F_SETFL,O_NONBLOCK);
                EV_SET(&clientEV,client_fd,EVFILT_READ,EV_ADD,0,0,NULL);
                kevent(kq,&clientEV,1,NULL,0,NULL);
                continue;
            }
            handle_client(&kq,events[i].ident ,events,&i);
        } 
    }
        
    close(fd_server);

}
