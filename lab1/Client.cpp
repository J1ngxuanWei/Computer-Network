/*-----------------------------------------------------
				   Designed by wjx
	 This document and program use GBK encoding
	   And the powershell must be GBK encoding
   How to compile: g++ .\Client.cpp -o Client -lwsock32
------------------------------------------------------*/
#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>

using namespace std;

#define IP "127.0.0.1"
#define MaxClient 20
#define MaxBufSize 1024
#define key 24

const int PORT = 8888;
char FLAG[7] = {'2', '1', '1', '2', '4', '9', '5'};
int SOCKID;
int isalive;

class Message
{
	// ��ƣ���һλΪ�û���id��������MaxClient�����ڶ�λΪ����λ��1��ʾ��ȡ�����ܣ�0��ʾ�����ܣ�
	// ��������λΪ��־λ��Ҳ������������֤�ķ������Ϳͻ��˵ı�־�룩���������Ϊ�ҵ�ѧ��2112495��
	// ͬʱ��־λ��ÿһλת��Ϊʮ���ƣ�7�����ּ���������Ϊ�����key���������λΪ1���Ժ��������λִ�������ܲ���
	// ���浽MaxBufSize�Ķ�Ϊ����λ
private:
	char ID; // IDΪ������һ��Ҳ������ʾ��ID��
	char Encryption;
	char Flag[7];
	char Data[MaxBufSize - 9];

public:
	Message()
	{
		ID = Encryption = '0';
		Flag[0] = '2';
		Flag[1] = '1';
		Flag[2] = '1';
		Flag[3] = '2';
		Flag[4] = '4';
		Flag[5] = '9';
		Flag[6] = '5';
		memset(Data, 0, sizeof(Data));
	}
	Message(char *da)
	{
		int str = 0;
		ID = da[str++];
		Encryption = da[str++];
		for (int i = 0; i < 7; i++)
			Flag[i] = da[str++];
		for (int i = str; i < MaxBufSize; i++)
			Data[i - 9] = da[i];
	}
	void setID(int a)
	{
		ID = '0' + a;
	}
	int getID()
	{
		return ID - '0';
	}
	void set_Encryption(int a)
	{
		Encryption = '0' + a;
	}
	int get_Authorize()
	{
		for (int i = 0; i < 7; i++)
			if (Flag[i] != FLAG[i])
				return 0;
		return 1;
	}
	void setdata(char *buff)
	{
		if (Encryption == '0')
		{
			for (int i = 0; i < MaxBufSize - 9; i++)
				Data[i] = buff[i];
		}
		else
		{
			for (int i = 0; i < MaxBufSize - 9; i++)
				Data[i] = buff[i] ^ key;
		}
	}
	void getdata(char *buff)
	{
		if (Encryption == '0')
		{
			strcpy(buff, Data);
			return;
		}
		int i = 0;
		for (; i < MaxBufSize - 10; i++)
			buff[i] = Data[i] ^ key;
		buff[i] = 0;
	}
	void tobuff(char *buff)
	{
		int str = 0;
		buff[str++] = ID;
		buff[str++] = Encryption;
		for (int i = 0; i < 7; i++)
			buff[str++] = Flag[i];
		for (int i = str; i < MaxBufSize; i++)
			buff[i] = Data[i - 9];
	}
};

DWORD WINAPI mysend(LPVOID Cliso)
{
	SOCKET *Cli = (SOCKET *)Cliso;
	char buff[MaxBufSize];
	memset(buff, 0, MaxBufSize);
	while (1)
	{
		cin.getline(buff, MaxBufSize, '\n');
		// ���ַ�������
		if (buff[0] == 0)
			continue;
		cout << "------------------------------------------------------------------------------------" << endl;
		// ������Ϣ
		char sendbuff[MaxBufSize];
		memset(sendbuff, 0, MaxBufSize);
		Message ssend;
		ssend.setID(SOCKID);
		ssend.set_Encryption(1);
		ssend.setdata(buff);
		ssend.tobuff(sendbuff);
		// ����
		if (send(*Cli, sendbuff, MaxBufSize, 0) == SOCKET_ERROR)
		{
			cout << "����ʧ�ܣ������������ӣ�" << endl;
			return 1;
		}
		if (strcmp(buff, "/quit") == 0)
		{
			// �˳�
			SYSTEMTIME st = {0};
			GetLocalTime(&st);
			cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "���ɹ��˳������ң�����ر�!" << endl;
			cout << "------------------------------------------------------------------------------------" << endl;
			isalive = 0;
			closesocket(*Cli);
			exit(0);
		}
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
			 << " ";
		cout << "��Ϣ�Ѿ��ɹ�����:" << buff << endl;
		cout << "------------------------------------------------------------------------------------" << endl;
		memset(buff, 0, MaxBufSize);
		Sleep(300);
	}
	return 0;
}

DWORD WINAPI myrecv(LPVOID Cliso)
{
	SOCKET *Cli = (SOCKET *)Cliso;
	char buff[MaxBufSize];
	char getbuff[MaxBufSize];
	memset(buff, 0, MaxBufSize);
	while (1)
	{
		memset(getbuff, 0, MaxBufSize);
		int state = recv(*Cli, getbuff, MaxBufSize, 0);
		if (isalive == 0)
			return 1;
		if (state == SOCKET_ERROR)
		{
			cout << "��Ŀ�������������Ϣʧ�ܣ��������ӣ�" << endl;
			closesocket(*Cli);
			return 1;
		}
		else if (state == 0)
		{
			exit(0);
		}
		// ������Ϣ
		Message recv(getbuff);
		if (recv.getID() != SOCKID)
		{
			cout << "���ܵ�������Ϣ�������Ͽ�����" << endl;
			closesocket(*Cli);
			return 1;
		}
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "��⵽��δ��Ȩ�û����֣������Ͽ�����" << endl;
			closesocket(*Cli);
			return 1;
		}
		// ��������
		if (buff[0] == 0)
			continue;
		cout << "------------------------------------------------------------------------------------" << endl;
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
			 << " ";
		cout << buff << endl;
		cout << "------------------------------------------------------------------------------------" << endl;
		memset(buff, 0, MaxBufSize);
		Sleep(300);
	}
}

void getalive(char *buff)
{
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "��ӭ��Ŀǰ���ߵ��û�Ϊ��";
	for (int i = 10; i < 30; i++)
		if (buff[i] == '1')
			cout << i - 9 << "��";
	cout << endl;
}

int main()
{
	// ��ʼ��DLL
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "��ӭʹ�ü��������ң����ڳ�ʼ���ͻ��˲�������ȴ���" << endl;
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsdata;
	if (WSAStartup(sockVersion, &wsdata) != 0)
	{
		cout << "��ʼ��DLLʧ�ܣ�" << endl;
		return 1;
	}
	cout << "��ʼ��DLL�ɹ���" << endl;
	// ������ʽ�׽���
	SOCKET clientsock = socket(AF_INET, SOCK_STREAM, 0);
	if (clientsock == INVALID_SOCKET)
	{
		cout << "������ʽ�׽���ʧ�ܣ�" << endl;
		return 1;
	}
	// ������ʽ�׽���
	SOCKADDR_IN clientsockaddr;
	clientsockaddr.sin_family = AF_INET;
	clientsockaddr.sin_port = htons(PORT);
	clientsockaddr.sin_addr.s_addr = inet_addr(IP);
	cout << "��������ʼ����ʽ�׽��ֳɹ���" << endl;
	// ��ʼ����
	if (connect(clientsock, (SOCKADDR *)&clientsockaddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		cout << "���ӵ�Ŀ�������ʧ��!" << endl;
		return 0;
	}
	cout << "���ѳɹ����ӵ�Ŀ�������,������IPΪ:" << IP << "�˿�Ϊ:" << PORT << endl;
	// ��ȡ��������������ǵ�ID
	while (1)
	{
		// ��������Խӣ�20���ַ���С
		char getbuff[MaxBufSize];
		char buff[MaxBufSize];
		memset(getbuff, 0, MaxBufSize);
		memset(buff, 0, MaxBufSize);
		recv(clientsock, getbuff, 100, 0);
		// ������Ϣ
		Message recv(getbuff);
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "��⵽��δ��Ȩ�û����֣������Ͽ�����" << endl;
			closesocket(clientsock);
			WSACleanup();
			return 0;
		}
		// ��������
		if (buff[0])
		{
			getalive(buff);
			buff[10] = 0;
			cout << "���ݷ��������䣬" << buff << endl;
			SOCKID = buff[9] - '0';
			break;
		}
		Sleep(300);
	}
	cout << "����������������������Ϣ��ʼ�����ˣ�" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	isalive = 1;
	// ���̴߳�����պͷ���
	int str = 0;
	while (1)
	{
		if (str == 0)
		{
			HANDLE hThread1 = CreateThread(NULL, 0, mysend, (LPVOID)&clientsock, 0, NULL);
			HANDLE hThread2 = CreateThread(NULL, 0, myrecv, (LPVOID)&clientsock, 0, NULL);
			CloseHandle(hThread1);
			CloseHandle(hThread2);
			str++;
		}
		Sleep(300);
	}
	closesocket(clientsock);
	WSACleanup();
	return 0;
}
