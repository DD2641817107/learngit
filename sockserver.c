#include <unistd.h>	
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ERR_EXIT(m) \
       do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

void handler()
{
        printf("recv a signal\n");
        exit(EXIT_SUCCESS);
}

int main(void)
{
    //声明主动套接字
    int listenfd;
    if((listenfd = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
        ERR_EXIT("socket");

    //设置端口可重复使用
    int on=1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    //本地地址
    struct sockaddr_in servaddr;
    //internet地址族
    servaddr.sin_family = AF_INET;
    //端口号 值为0，则linux帮选择合适的端口 不加htonl
    servaddr.sin_port = htons(5188);
    //因特网地址 值为 INNDDR_ANY 使用本机的IP
    servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
    //将剩下结构中的空间置0
    bzero(&(servaddr.sin_zero),8);
    //绑定本地地址
    if((bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)))<0)
        ERR_EXIT("bind");
    //监听，使listenfd变为被动套接字，用于接收连接请求
    if((listen(listenfd, SOMAXCONN))<0)
        ERR_EXIT("listen");
    //对等方地址
    struct sockaddr_in peeraddr;
    socklen_t peerlen=sizeof(peeraddr);

    pid_t pid;
    while(1)
    {
        //接收来自对等方的连接请求
        int conn;
        if( ( conn = accept(listenfd, (struct sockaddr*)&peeraddr, &peerlen) ) < 0 )
            ERR_EXIT("accept");
        //若连接成功，打印输出对等方信息 ntohl 网络字节顺序转换为主机字节顺讯
        //inet_addr 将ip地的字符串转换为一个无符号长整型
	//inet_ntoa 将struct in_addr 里面存储的网络地址以数字.数字.数字.数字 的格式显示出来
	printf("客户端的IP地址是：%s,端口号是：%d\n",
        inet_ntoa(peeraddr.sin_addr),ntohs(peeraddr.sin_port));
        //申请新的进程
        pid=fork();
        if(pid == -1)
            ERR_EXIT("fork");
        //若为子进程
        else if(pid == 0)
        {
            close(listenfd);
            while(1)
            {
                char recvbuf[1024]={0};
                //读取信息
                int ret=read(conn, recvbuf, sizeof(recvbuf));
                if(ret == -1)
                    ERR_EXIT("read");
                else if(ret == 0)
                {
                    printf("peer close\n");
                    break;
                }
                //打印输出
                fputs(recvbuf, stdout);
            }
            //读取完毕或对方退出，发送结束信号 当父进程结束发送SIGUSER1
	    //getppid返回父进程标识
            kill(getppid(),SIGUSR1);
            exit(EXIT_SUCCESS);
        }
        else
        {
	    //SIGUSR1 用户自定义信号 默认处理：进程终止 现在是执行函数指针
            signal(SIGUSR1,handler);
            char sendbuf[1024]={0};
            while(fgets(sendbuf,sizeof(sendbuf),stdin) != NULL)
            {
                write(conn, sendbuf, sizeof(sendbuf));
		//作用是在一段内存块中填充某个给定的值，它是对较大的结构体或数组进行清零操作的一种最快方法
                memset(sendbuf,0,sizeof(sendbuf));
            }
            exit(EXIT_SUCCESS);
        }
        close(conn);
    }

    return 0;
}
