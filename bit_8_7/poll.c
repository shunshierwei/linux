#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

const int fds_nums=64;
int fds_array[64];

static int startup(char* _ip,int _port)
{
	int sock=socket(AF_INET,SOCK_STREAM,0);
		if(sock<0)
		{
			perror("socket");
			exit(4);
		}
	int opt=1;
	//设置更加详细的socket信息
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
	struct sockaddr_in local;
	local.sin_family=AF_INET;
	local.sin_port=htons(_port);
	local.sin_addr.s_addr=inet_addr(_ip);

	if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0)
	{
		perror("bind");
		exit(5);
	}

	if(listen(sock,5)<0)
	{
		perror("listen");
		exit(6);
	}
	return sock;
}

int main(int argc,char* argv[])
{
	if(argc !=3)
	{
		printf("Usage:%s [ip] [port]\n",argv[0]);
		exit(1);
	}
     //对文件描述符数组进行初始化
	int i=0;
	for(;i<fds_nums;++i)
	{
		fds_array[i]=-1;
	}
	//创建监听套接字
	int listen_sock=startup(argv[1],atoi(argv[2]));
	//fd_set结构体仅包含一个整形数组，该数组的每个元素的每个比特位标记一个文件描述符
	//fd_set能够容纳的文件描述符数量由FD_SETSIZE指定，这就限制了select能够同时处理的文件描述符数量
	fd_set rset;
	FD_ZERO(&rset);//清除rset的所有位，即所有文件描述符
	FD_SET(listen_sock,&rset);//设置rset的listen_sock位

	int max_fd=-1;//最大的文件描述符的下标

	fds_array[0]=listen_sock;

	//设置select的延迟时间
    struct timeval timeout={5,0};//5秒 0微秒
	int done=0;
	while(!done)
	{
		max_fd=-1;
		i=0;
		for(;i<fds_nums;++i)
		{
			if(fds_array[i]>0)
			{
			FD_SET(fds_array[i],&rset);//	
			max_fd=max_fd>fds_array[i]?max_fd: fds_array[i];//计算max_fd
			}
		}
	
	// timeout.tv_sec=5;
	// timeout.tv_userc=0;
	// timeout={5,0}//每次都要重新设置
	// int select(int nfds,fd_set* readfds,fd_set* writefds,fd_set* exceptfds,
	//          struct timeval* timeval);
	//readfds、writefds、exceptfds分别指向可读、可写和异常事件对应的文件描述符集合进监听等待
	int ret=select(max_fd+1,&rset,NULL,NULL,NULL);//timeout为空表示阻塞等待
	
	switch(ret)
	{
		case 0:
		    printf("timeout...\n");//超时而且没有任何文件描述符就绪
			break;
		case -1:
		    perror("select");//出错
			exit(1);
		default:
			{
				i=0;
				for(;i<fds_nums;++i)
				{
		
					if(fds_array[i]==listen_sock && FD_ISSET(listen_sock,&rset))
					{
						//有可读的文件描述符就绪即迎宾的人已经就绪，可以让客人进来了
						//即可以连接客户端了accept
						struct sockaddr_in remote;
						socklen_t len=sizeof(remote);
						int new_sock=accept(listen_sock,(struct sockaddr*)&remote,&len);
						if(new_sock<0)
						{
							//连接失败
							perror("accept");
							exit(2);
						}
						printf("get a new client:socket ->%s:%d\n",inet_ntoa(remote.sin_addr),ntohs(remote.sin_port));
						//将可读的客户端加入rset
						int j=0;
						for(;i<fds_nums;++j)
						{
							if(fds_array[j]==-1)
							{
								fds_array[j]=new_sock;
								break;
							}

						}
						if(j==fds_nums)
						{
							//空间已用完，不能再添加新用户了
							close(new_sock);
						}
						
					}
					else
					{
						if(fds_array[i]>0&&FD_ISSET(fds_array[i],&rset))
						{
							char buf[1024];
							memset(buf,'\0',sizeof(buf));
							//从fds_array[i]这个客户端读取数据到buf
							ssize_t _s =read(fds_array[i],buf,sizeof(buf)-1);
							if(_s>0)
							{
								buf[_s-1]='\0';
								printf("client# %s\n",buf);

							}
							else if(_s==0)
							{
								printf("client close...\n");
								close(fds_array[i]);
								fds_array[i]=-1;
							}
							else
							{
								perror("read");
								exit(3);
							}
						}
					}
				}
				break;
			}
	   }
  }	
}
