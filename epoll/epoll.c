#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/epoll.h>

#define LISTEN_BACK_LOG 10
//创建 监听套接字
int startup(char* ip,int port)
{
	int sock=socket(AF_INET,SOCK_STREAM,0);
	if(sock<0)
	{
		perror("socket");
		exit(1);
	}
	int op=1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&op,sizeof(int));
	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_port=htons(port);
	local.sin_addr.s_addr=inet_addr(ip);
	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind");
		exit(2);
	}
	if(listen(sock,LISTEN_BACK_LOG)<0)
	{
		perror("listen");
		exit(3);
	}
	return sock;
}

int main(int argc,char* argv[])
{
	if(argc !=3)
	{
		printf("please enter:%[ip][port]\n",argv[0]);
		exit(4);
	}
	int listen_sock=startup(argv[1],atoi(argv[2]));
    //创建 epoll 句柄 事件表
	int epfd=epoll_create(101);//这里数字不固定， 101只是告诉内核预计用101个文件描述符，但实际这个参数不起作用
    if(epfd<0)
	{
		perror("epoll_create");
		exit(5);
	}
	struct epoll_event event;
	event.events=EPOLLIN;//读 事件 列表
    event.data.fd=listen_sock;
	//向事件表中增加事件
	epoll_ctl(epfd,EPOLL_CTL_ADD,listen_sock,&event);
	struct epoll_event fd_events[100];
	int size=sizeof(fd_events)/sizeof(fd_events[0]);
	int i=0;
	for(i=0;i<size;i++)
	{
		fd_events[i].events=0;
		fd_events[i].data.fd=-1;
	}
	int nums=0;
	int timeout=10000;
	int done=0;
	while(!done)
	{
		//返回就绪但文件个数
		nums=epoll_wait(epfd,fd_events,size,timeout);
		switch(nums)
		{
			case 0:
				printf("timeout...\n");
				break;
			case -1:
				printf("epoll_wait\n");
				exit(6);
			default:
				{
					for(i=0;i<nums;i++)
					{
						int fd=fd_events[i].data.fd;
						if((fd==listen_sock)&&(fd_events[i].events & EPOLLIN))
						{
							//listen socket 有新的连接请求
							struct sockaddr_in peer;
							socklen_t len=sizeof(peer);
							int new_sock=accept(listen_sock,(struct sockaddr*)&peer,&len);
							if(new_sock<0)
							{
								perror("accept");
								continue;
							}
							printf("get a new client,socket->%s:%d\n",inet_ntoa(peer.sin_addr),ntohs(peer.sin_port));
							event.events=EPOLLIN;
							event.data.fd=new_sock;
							//将new_sock 添加进内核事件表
							epoll_ctl(epfd,EPOLL_CTL_ADD,new_sock,&event);
						}
						else
						{
							//other socket
							//读事件满足 处理客户端发送的 数据
							if(fd_events[i].events & EPOLLIN)
							{
								char buf[1024];
								memset(buf,'\0',sizeof(buf));
								ssize_t _s=recv(fd,buf,sizeof(buf)-1,0);
								if(_s>0)
								{
									printf("client:%s\n",buf);
									event.events= EPOLLOUT;//将fd事件改写 方便服务器 给请求的客户端发数据
									epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&event);
								}
								else if(_s==0)
								{
									printf("client close...\n");
									epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);//空表示不关心返回值
									close(fd);
								}
								else
								{
									perror("recv");
									continue;
								}
							}
							else if(fd_events[i].events & EPOLLOUT)
							{
								char *msg="HTTP/1.1 200 OK\r\n\r\n<html><h1>hello ^_^<h1></html>\r\n";
								send(fd,msg,strlen(msg),0);
								epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
								close(fd);
							}
							else
							{
							}
						}
					}
				}
				break;
		}
	}
	exit(0);
}  
