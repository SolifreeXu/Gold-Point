#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <WinSock2.h>
#include <thr\threads.h>

#pragma comment(lib, "ws2_32.lib")

int score = 0;
SOCKET clientSocket;
mtx_t printMutex, socketMutex, scoreMutex;

int sendNumber(double number)
{
	static char buffer[BUFSIZ];
	_snprintf(buffer, BUFSIZ, "%lf", number);
	if (send(clientSocket, buffer, strlen(buffer), 0) < 0)
	{
		mtx_lock(&printMutex);
		printf("连接出错\n");
		mtx_unlock(&printMutex);
		return -1;
	}
	return 0;
}

int receiveNumber(void *argv)
{
	char buffer[BUFSIZ];
	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		if (recv(clientSocket, buffer, sizeof(buffer), NULL) == SOCKET_ERROR)
			return -1;
		if (strcmp(buffer, "服务器连接数已达上限") == 0)
		{
			mtx_lock(&printMutex);
			printf("服务器连接数已达上限\n");
			mtx_unlock(&printMutex);
			return -2;
		}
		else if (strcmp(buffer, "相同ip重复登录") == 0)
		{
			mtx_lock(&printMutex);
			printf("使用同一ip重复登录\n");
			mtx_unlock(&printMutex);
			return -3;
		}
		else if (strcmp(buffer, "选择数字无效") == 0)
		{
			mtx_lock(&printMutex);
			printf("选择数字无效\n");
			mtx_unlock(&printMutex);
		}
		else if (strcmp(buffer, "") != 0)
		{
			int index = 0, size = strlen(buffer);
			for (; index < size;++index)
			if (!(buffer[index] == '-' || buffer[index] >= '0' && buffer[index] <= '9'))
				break;
			if (index >= size)
			{
				mtx_lock(&printMutex);
				mtx_lock(&scoreMutex);
				score += atoi(buffer);
				printf("一轮游戏结束，当前分数：%d\n", score);
				mtx_unlock(&scoreMutex);
				mtx_unlock(&printMutex);
			}
		}
	}
	return 0;
}

int displayMenu()
{
	static char key;
	do
	{
		fflush(stdin);
		mtx_lock(&printMutex);
		printf("1. 选择数字\n");
		printf("2. 查看分数\n");
		printf("0. 退出游戏\n");
		mtx_unlock(&printMutex);
		key = getchar();
	} while (key<'0' || key>'2');
	return key - '0';
}

int main()
{
	mtx_init(&printMutex, NULL);
	mtx_init(&socketMutex, NULL);
	mtx_init(&scoreMutex, NULL);

	printf("黄金点游戏客户端欢迎你\n");

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("初始化动态库失败\n");
		return -1;
	}

	char serverip[14];
	int port;
	printf("输入服务器IP地址：");
	scanf("%s", serverip);
	printf("输入服务器端口号：");
	while (scanf("%d", &port) != 1)
	{
		printf("输入有误\n输入服务器端口号：");
		fflush(stdin);
	}

	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = PF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(serverip);
	sockAddr.sin_port = htons(port);

	clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(clientSocket, (SOCKADDR*)&sockAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("无法连接到服务器，请检查网络\n");
		return -1;
	}
	printf("成功连接到服务器\n");

	thrd_t receiveThread;
	thrd_create(&receiveThread, receiveNumber, (void*)clientSocket);
	thrd_detach(receiveThread);

	while (1)
		switch (displayMenu())
	{
		case 1:
			double number;
			mtx_lock(&printMutex);
			printf("输入数字：");
			mtx_unlock(&printMutex);
			while (scanf("%lf", &number) != 1)
				fflush(stdin);
			sendNumber(number);
			break;
		case 2:
			mtx_lock(&printMutex);
			mtx_lock(&scoreMutex);
			printf("当前分数：%d\n", score);
			mtx_unlock(&scoreMutex);
			mtx_unlock(&printMutex);
			break;
		case 0:
			closesocket(clientSocket);
			WSACleanup();
			return 0;
		default:
			break;
	}

	return -1;
}