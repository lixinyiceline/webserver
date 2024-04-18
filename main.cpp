#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS_NUMBER 5


// 将文件描述符fd设置为非阻塞模式
int set_non_blocking( int fd )
{
	int old_state = fcntl( fd, F_GETFL );
	int new_state = old_state | O_NONBLOCK;
	fcntl( fd, F_SETFL, new_state );

	return old_state;	
}

// 将文件描述符添加到epoll实例
void addfd( int epollfd , int fd )
{
	epoll_event event;
	event.events = EPOLLIN | EPOLLET;  // 要监听的事件类型，EPOLLIN表示可读事件，EPOLLET表示使用边缘触发模式
    event.data.fd = fd;
	epoll_ctl( epollfd, EPOLL_CTL_ADD, fd, &event );
	set_non_blocking( fd );	
}

int main(int argc,char* argv[])
{
	// argc为命令行参数的数量，argv[0]是程序名称，argv[1]及之后的元素是传递给程序的命令行参数
	// 参数数量需 > 2
	if (argc <= 2) {
		printf("Usage: %s ip_address portname\n", argv[0]);
		return 0;
	}
	// 获取IP和端口号
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	// 创建一个监听TCP套接字
	int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
	assert( listenfd >= 1 );
	// 初始化一个IPv4地址结构体
	struct sockaddr_in address;
	memset( &address, 0, sizeof( address ) );
	address.sin_family = AF_INET;
	address.sin_port = htons( port );
	inet_pton( AF_INET, ip, &address.sin_addr );
	// 绑定套接字和地址结构体
	int ret = 0;
	ret = bind( listenfd, (struct sockaddr*)( &address ), 
				sizeof( address ) );
	assert( ret != -1 );
	// 监听
	ret = listen( listenfd, 5 );
	assert( ret != -1 );

	// 创建一个epoll实例
	epoll_event events[ MAX_EVENTS_NUMBER ];
    int epollfd = epoll_create( 5 );
	assert( epollfd != -1);
	addfd( epollfd, listenfd );

	while(1) {
		// 调用epoll_wait函数来等待就绪事件的发生,返回就绪事件的数量
		int number = epoll_wait( epollfd, events, MAX_EVENTS_NUMBER, -1 );  // events：用于存储就绪事件的数组
		if( number < 0 )
		{
			printf( "epoll_wait failed\n" );
			return -1;
		}
		// 遍历所有的就绪事件
		for( int i = 0; i < number; ++i ) {
			// 获取就绪事件和对应的文件描述符
			const auto& event = events[i];
			const auto eventfd = event.data.fd;

			// 新的连接事件
			if (eventfd == listenfd) {
				// 接受客户端的连接请求，并创建一个新的套接字用于与客户端进行通信。
				struct sockaddr_in client;
				socklen_t client_addrlength = sizeof( client );
				int sockfd = accept( listenfd, (struct sockaddr*)( &address ), &client_addrlength );
				addfd(epollfd, sockfd);
			}
			// 可读事件
			else if (event.events & EPOLLIN) {
				// 执行基本的接收和发送数据的操作
				char buf[1024] = {0};
				while (1) {
					memset( buf, '\0', sizeof( buf ) );
					int recv_size = recv( eventfd, buf, sizeof( buf ) , 0);  // 把eventfd中的数据读到用户缓冲区里
					if( recv_size < 0 )
					{
						// 使用非阻塞I/O模式时，如果当前没有数据可读，可以跳出循环继续等待下一个事件
						if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
						{
							break;
						}
						printf(" sockfd %d,recv msg failed\n", eventfd );
						close( eventfd );
						break;
					}
					// 对方关闭了连接
					else if( recv_size == 0)
					{
					   	close( eventfd );
						break;	
					}
					// 读到数据了
					else {
						send(eventfd, buf, recv_size, 0);
					}
				}
			}
		}
	}
	
	close(listenfd);

	return 0;
}
