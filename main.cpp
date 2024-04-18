#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
	// 创建一个TCP套接字
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
	// 接受客户端的连接请求，并创建一个新的套接字用于与客户端进行通信。
	struct sockaddr_in client;
	socklen_t client_addrlength = sizeof( client );
	int sockfd = accept( listenfd, (struct sockaddr*)( &address ), &client_addrlength );
	// 执行基本的接收和发送数据的操作
	char buf_size[1024] = {0};
	int recv_size = 0;
	recv_size = recv( sockfd, buf_size, sizeof( buf_size ) , 0);
	
	int send_size = 0;
	send_size = send( sockfd, buf_size , recv_size , 0 );

	// 关闭套接字
	close(sockfd);
	close(listenfd);

	return 0;
}
