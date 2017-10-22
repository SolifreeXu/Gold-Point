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

SOCKET serverSocket;	// �������׽���

struct ClientData
{
	thrd_t receiveThread;
	SOCKET clientSocket;
	SOCKADDR_IN clientAddr;
	int number;
}clientInfo[MAX_CONNECTIONS];	// �ͻ�����Ϣ
int clientCounter = 0;	// �ͻ���������

int numberIndex[MAX_CONNECTIONS];	// ��˳����� ѡ������ ��Ӧ�� �ͻ�����Ϣ �����±�
int numberCounter = 0;	// ���ָ���
int defaultPlayerNumber = MAX_CONNECTIONS / 2;	// ��ʼ��Ϸ��Ĭ�����ָ���
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
			printf("�û��Ͽ����ӣ���ǰ����������%d\n", clientCounter);
			return -1;
		}
		if (clientInfo[(int)argv].number == MIN_INT)
		{
			clientInfo[(int)argv].number = atoi(buffer);
			numberIndex[numberCounter++] = (int)argv;
			if (numberCounter >= defaultPlayerNumber)
				cnd_signal(&startUpGameCondition);	//������Ϸ��������
		}
		else
			send(clientInfo[(int)argv].clientSocket, "ѡ��������Ч", 12, NULL);
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
		// ���տͻ�������
		clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket != SOCKET_ERROR)
		{
			// ����ظ�����
			for (clientIndex = 0; clientIndex < MAX_CONNECTIONS; ++clientIndex)
			{
				if (clientInfo[clientIndex].clientSocket != 0 \
					&& clientInfo[clientIndex].clientAddr.sin_addr.s_addr == clientAddr.sin_addr.s_addr)
				{
					send(clientSocket, "��ͬip�ظ���¼", 14, NULL);
					repeatFlag = 1;
					break;
				}
			}
			if (!repeatFlag)
			{
				for (clientIndex = 0; clientIndex < MAX_CONNECTIONS; ++clientIndex)
				if (clientInfo[clientIndex].clientSocket == 0)
					break;
				// �����ͻ��������߳�
				if (clientIndex < MAX_CONNECTIONS)
				{
					clientInfo[clientIndex].clientSocket = clientSocket;
					memcpy(&clientInfo[clientIndex].clientAddr, &clientAddr, sizeof(SOCKADDR));
					thrd_create(&clientInfo[clientIndex].receiveThread, receiveNumber, (void*)clientIndex);
					thrd_detach(clientInfo[clientIndex].receiveThread);
					++clientCounter;
					printf("�û��½����ӣ���ǰ����������%d\n", clientCounter);
				}
				else
					send(clientSocket, "�������������Ѵ�����", 20, NULL);
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
		printf("1. �趨��Ϸ����\n");
		printf("2. ǿ�ƿ�ʼ��Ϸ\n");
		printf("0. �رշ�����\n");
		key = getchar();
	} while (key<'0' || key>'2');
	return key - '0';
}

int main()
{
	printf("�ƽ����Ϸ��������ӭ��\n");

	// ��ʼ����̬��
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("��ʼ����̬��ʧ��\n");
		return -1;
	}

	// �����׽���
	serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		printf("�����׽���ʧ��\n");
		return -2;
	}

	// ��ȡ�������˵��ַ
	char serverIP[14];
	int port;
	printf("���������IP��ַ��");
	scanf("%s", serverIP);
	printf("����������˿ںţ�");
	scanf("%d", &port);

	// ��ʼ���˵��ַ
	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));	//����
	sockAddr.sin_family = PF_INET;	//ʹ��IPv4��ַ
	sockAddr.sin_addr.s_addr = inet_addr(serverIP);	//�����IP��ַ
	sockAddr.sin_port = htons(port);	//�˿�

	// ���׽���
	if (bind(serverSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
	{
		printf("�޷����׽�����˶˵��ַ�����\n");
		return -3;
	}

	// �������״̬
	if (listen(serverSocket, MAX_CONNECTIONS) == SOCKET_ERROR)
	{
		printf("����ģʽ��ʼ��ʧ��\n");
		return -4;
	}

	initClientData();

	// ��ʼ������������������
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
			printf("������Ϸ����ʱ������");
			while (scanf("%d", &defaultPlayerNumber) != 1)
				fflush(stdin);
			numberBackup = defaultPlayerNumber;
			break;
		case 2:
			defaultPlayerNumber = numberCounter;
			cnd_signal(&startUpGameCondition);
			break;
		case 0:
			// �ر��׽���
			for (int i = 0; i < MAX_CONNECTIONS; ++i)
			if (clientInfo[clientCounter].clientSocket != 0)
				closesocket(clientInfo[clientCounter].clientSocket);
			closesocket(serverSocket);
			// ��ֹ��̬���ʹ��
			WSACleanup();
			return 0;
		default:
			break;
	}

	return -1;
}