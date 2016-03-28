#include "VideoSocket.h"

CVideoSocket::CVideoSocket() :ServerPort(3333), piAddress("192.168.0.5"), videoStreamUrl("tcp://192.168.0.5:2222")
{

	// ���� �ʱ�ȭ
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR)
	{
		printf("WinSock �ʱ�ȭ�κп��� ���� �߻�.n");
		WSACleanup();
		exit(0);
	}

	memset(&ToServer, 0, sizeof(ToServer));
	memset(&FromServer, 0, sizeof(FromServer));

	ToServer.sin_family = AF_INET;
	// �ܺξ����Ƿε� ��Ʈ�� �ϰ� �;��µ� ���� �������� �ܺξ����Ƿ� ��Ʈ�������� �ϸ� �����Ͱ� ������ �ȵȴ�.
	// �ƹ����� ��Ʈ�� �����ִ� ��Ŀ� ������ �ִ°� ������ �ذ��� ���� ������.
	ToServer.sin_addr.s_addr = inet_addr(piAddress);
	ToServer.sin_port = htons(ServerPort); // ��Ʈ��ȣ

	ClientSocket = socket(AF_INET, SOCK_DGRAM, 0);// udp 

	if (ClientSocket == INVALID_SOCKET)
	{
		printf("������ �����Ҽ� �����ϴ�.");
		closesocket(ClientSocket);
		WSACleanup();
		exit(0);
	}
}


CVideoSocket::~CVideoSocket()
{
}

