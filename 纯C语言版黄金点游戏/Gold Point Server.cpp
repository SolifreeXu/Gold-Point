#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <WinSock2.h>
#include <thr\threads.h>

#pragma comment(lib,"ws2_32.lib")

#define MAX_CONNECTIONS 10
#define ESC 27
#define MIN_INT (1 << 31)

SOCKET serverSocket;	// 服务器套接字

struct ClientData
{
	thrd_t receiveThread;
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int number;
}clientInfo[MAX_CONNECTIONS];	// 客户端信息
int clientCounter = 0;	// 客户端连接数

int numberIndex[MAX_CONNECTIONS];	// 按顺序记载 选择数字 对应的 客户端信息 数组下标
int numberCounter = 0;	// 数字个数
int defaultPlayerNumber = MAX_CONNECTIONS / 2;	// 开始游戏的默认数字个数
int numberBackup = defaultPlayerNumber;

mtx_t startUpGameMutex;
cnd_t startUpGameCondition;

void initClientData()
{
	memset(clientInfo, 0, sizeof(clientInfo));
	for (int i = 0; i < MAX_CONNECTIONS; ++i)
		clientInfo[i].number = MIN_INT;
}

int receiveNumber(void *argv)
{
	char buffer[BUFSIZ];
	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		if (recv(clientInfo[(int)argv].clientSocket, buffer, sizeof(buffer), NULL) == SOCKET_ERROR)
		{
			clientInfo[(int)argv].clientSocket = 0;
			if (clientInfo[(int)argv].number != MIN_INT)
			{
				for (int i = 0; i < MAX_CONNECTIONS; ++i)
				if (numberIndex[i] == (int)argv)
				{
					for (int n = MAX_CONNECTIONS - 1; i < n; ++i)
						numberIndex[i] = numberIndex[i + 1];
					break;
				}
				clientInfo[(int)argv].number = MIN_INT;
			}
			--clientCounter;
			printf("用户断开连接，当前在线人数：%d\n", clientCounter);
			return -1;
		}
		if (clientInfo[(int)argv].number == MIN_INT)
		{
			clientInfo[(int)argv].number = atoi(buffer);
			numberIndex[numberCounter++] = (int)argv;
			if (numberCounter >= defaultPlayerNumber)
				cnd_signal(&startUpGameCondition);	//启动游戏条件变量
		}
		else
			send(clientInfo[(int)argv].clientSocket, "选择数字无效", 12, NULL);
	}
	return 0;
}

int acceptClients(void *argv)
{
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(SOCKADDR);
	int clientIndex = 0;
	char repeatFlag;

	while (1)
	{
		repeatFlag = 0;
		// 接收客户端请求
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket != SOCKET_ERROR)
		{
			// 检测重复连接
			for (clientIndex = 0; clientIndex < MAX_CONNECTIONS; ++clientIndex)
			{
				if (clientInfo[clientIndex].clientSocket != 0 \
					&& clientInfo[clientIndex].clientAddr.sin_addr.s_addr == clientAddr.sin_addr.s_addr)
				{
					send(clientSocket, "相同ip重复登录", 14, NULL);
					repeatFlag = 1;
					break;
				}
			}
			if (!repeatFlag)
			{
				for (clientIndex = 0; clientIndex < MAX_CONNECTIONS; ++clientIndex)
				if (clientInfo[clientIndex].clientSocket == 0)
					break;
				// 创建客户端连接线程
				if (clientIndex < MAX_CONNECTIONS)
				{
					clientInfo[clientIndex].clientSocket = clientSocket;
					memcpy(&clientInfo[clientIndex].clientAddr, &clientAddr, sizeof(SOCKADDR));
					thrd_create(&clientInfo[clientIndex].receiveThread, receiveNumber, (void*)clientIndex);
					thrd_detach(clientInfo[clientIndex].receiveThread);
					++clientCounter;
					printf("用户新建连接，当前在线人数：%d\n", clientCounter);
				}
				else
					send(clientSocket, "服务器连接数已达上限", 20, NULL);
			}
		}
	}
}

int startUpGame(void *argv)
{
	static double goldPoint;
	static double gap[MAX_CONNECTIONS];
	static double min;
	while (1)
	{
		mtx_lock(&startUpGameMutex);
		cnd_wait(&startUpGameCondition, &startUpGameMutex);
		if (numberCounter < defaultPlayerNumber)
			continue;
		goldPoint = 0;
		min = abs(clientInfo[numberIndex[0]].number);
		for (int i = 1; i < defaultPlayerNumber; ++i)
		if (abs(clientInfo[numberIndex[i]].number) - abs(min) > 0)
			min = abs(clientInfo[numberIndex[i]].number);

		for (int i = 0; i < defaultPlayerNumber; ++i)
			goldPoint += clientInfo[numberIndex[i]].number;
		goldPoint *= (sqrt((double)5) - 1) / 2;
		goldPoint /= defaultPlayerNumber;

		for (int i = 0; i < defaultPlayerNumber; ++i)
		{
			gap[i] = abs(goldPoint - clientInfo[numberIndex[i]].number);
			if (gap[i] - min < 0)
				min = gap[i];
		}

		for (int i = 0; i < defaultPlayerNumber; ++i)
		{
			if (gap[i] - min == 0)
				send(clientInfo[numberIndex[i]].clientSocket, "1", 1, NULL);
			else
				send(clientInfo[numberIndex[i]].clientSocket, "0", 1, NULL);
			clientInfo[numberIndex[i]].number = MIN_INT;
		}

		for (int i = 0, n = MAX_CONNECTIONS - defaultPlayerNumber; i < n; ++i)
			numberIndex[i] = numberIndex[i + defaultPlayerNumber];
		numberCounter -= defaultPlayerNumber;

		defaultPlayerNumber = numberBackup;
	}
	return 0;
}

int displayMenu()
{
	char key;
	do
	{
		fflush(stdin);
		printf("1. 设定游戏人数\n");
		printf("2. 强制开始游戏\n");
		printf("0. 关闭服务器\n");
		key = getchar();
	} while (key<'0' || key>'2');
	return key - '0';
}

int main()
{
	printf("黄金点游戏服务器欢迎你\n");

	// 初始化动态库
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("初始化动态库失败\n");
		return -1;
	}

	// 创建套接字
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("创建套接字失败\n");
		return -2;
	}

	// 获取服务器端点地址
	char serverIP[14];
	int port;
	printf("输入服务器IP地址：");
	scanf("%s", serverIP);
	printf("输入服务器端口号：");
	scanf("%d", &port);

	// 初始化端点地址
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));	//清零
	sockAddr.sin_family = PF_INET;	//使用IPv4地址
	sockAddr.sin_addr.s_addr = inet_addr(serverIP);	//具体的IP地址
	sockAddr.sin_port = htons(port);	//端口

	// 绑定套接字
	if (bind(serverSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("无法将套接字与此端点地址相关联\n");
		return -3;
	}

	// 进入监听状态
	if (listen(serverSocket, MAX_CONNECTIONS) == SOCKET_ERROR)
	{
		printf("监听模式初始化失败\n");
		return -4;
	}

	initClientData();

	// 初始化互斥锁和条件变量
	mtx_init(&startUpGameMutex, NULL);
	cnd_init(&startUpGameCondition);

	thrd_t gameThread;
	thrd_create(&gameThread, startUpGame, NULL);
	thrd_detach(gameThread);

	thrd_t receiveThread;
	thrd_create(&receiveThread, acceptClients, NULL);
	thrd_detach(receiveThread);

	while (1)
		switch (displayMenu())
	{
		case 1:
			printf("输入游戏进行时人数：");
			while (scanf("%d", &defaultPlayerNumber) != 1)
				fflush(stdin);
			numberBackup = defaultPlayerNumber;
			break;
		case 2:
			defaultPlayerNumber = numberCounter;
			cnd_signal(&startUpGameCondition);
			break;
		case 0:
			// 关闭套接字
			for (int i = 0; i < MAX_CONNECTIONS; ++i)
			if (clientInfo[clientCounter].clientSocket != 0)
				closesocket(clientInfo[clientCounter].clientSocket);
			closesocket(serverSocket);
			// 终止动态库的使用
			WSACleanup();
			return 0;
		default:
			break;
	}

	return -1;
}