#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <iostream>
#include <fcntl.h>
#include<sys/wait.h>
using namespace std;

#define MAXSIZE 1024
#define IPADDRESS "127.0.0.1"
#define SERV_PORT 7000
#define FDSIZE 1024
#define EPOLLEVENTS 20
#define client_nums 50

int setSocketNonBlocking1(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}
int main(int argc, char *argv[])
{

    for (int i = 0; i < client_nums; ++i)
    {
        pid_t fpid;
        fpid = fork();
        if (fpid < 0)
            printf("error in fork \n");
        else if (fpid == 0)
        {
            int sockfd;
            struct sockaddr_in servaddr;
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(SERV_PORT);
            inet_pton(AF_INET, IPADDRESS, &servaddr.sin_addr);

            if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0)
            {

                setSocketNonBlocking1(sockfd);
                printf("begin long sleep...");
                sleep(25);
                close(sockfd);
            }
            return 0;
        }
        else{
        }
    }
    wait(NULL);
    return 0;
}
