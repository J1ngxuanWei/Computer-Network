/*--------------------------------------------------------
				   Designed by wjx
	 This document and program use GBK encoding
		And the powershell must be GBK encoding
   How to compile: g++ .\Server.cpp -o Server -lwsock32
---------------------------------------------------------*/
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
int Clinub = 0;
vector<SOCKET> Clis;
int Client_alive[MaxClient];
char FLAG[7] = {'2', '1', '1', '2', '4', '9', '5'};

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

DWORD WINAPI SeverThread(LPVOID Socketindex)
{
	SOCKET Cli = Clis[(long long)Socketindex];
	if (Cli == INVALID_SOCKET)
	{
		cout << "Ϊ�û������������ʧ�ܣ�" << endl;
		return 1;
	}
	Client_alive[(long long)Socketindex] = 1;
	char getbuff[MaxBufSize];
	char buff[MaxBufSize];
	memset(buff, 0, MaxBufSize);
	// ��Ϊ�ǽ��պ�ŷ��ͣ����Բ���Ҫ���߳�
	while (1)
	{
		// ��ʼ���ܿͻ���������
		recv(Cli, getbuff, MaxBufSize, 0);
		// ������Ϣ
		Message recv(getbuff);
		if (recv.getID() != (long long)Socketindex + 1)
		{
			cout << "���ܵ�������Ϣ" << endl;
			continue;
		}
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "��⵽��δ��Ȩ�û����֣������ر�����û�������" << endl;
			memset(buff, 0, MaxBufSize);
			strcpy(buff, "/quit");
			// �����û��˳����ִ���
		}
		// ���ַ����Ͳ�֪��BUG������
		if (buff[0] == 0)
			continue;
		if (strcmp(buff, "/quit") == 0)
		{
			// �û��˳�
			SYSTEMTIME st = {0};
			GetLocalTime(&st);
			cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "�û� " << (long long)Socketindex + 1 << " �˳������ң�" << endl;
			string buf = "�û� ";
			buf += '1' + (long long)Socketindex;
			buf += " �˳�����";
			strcpy(buff, buf.c_str());
			// ������Ϣ
			char sendbuff[MaxBufSize];
			memset(sendbuff, 0, MaxBufSize);
			Message ssend;
			// ssend.setID(i + 1);
			ssend.set_Encryption(1);
			ssend.setdata(buff);
			// ssend.tobuff(sendbuff);
			//  Ⱥ����Ϣ
			for (int i = 0; i < Clinub; i++)
			{
				if (Cli != Clis[i] && Client_alive[i] == 1)
				{
					ssend.setID(i + 1);
					ssend.tobuff(sendbuff);
					send(Clis[i], sendbuff, MaxBufSize, 0);
				}
			}
			// �ر�SOCKET
			Client_alive[(long long)Socketindex] = 0;
			closesocket(Cli);
			return 0;
		}
		// ����Ⱥ����Ϣ
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
			 << " ";
		cout << "�û� " << (long long)Socketindex + 1 << " :" << buff << endl;
		string buffstr(buff);
		buffstr = " :" + buffstr;
		char id = ('0' + ((long long)Socketindex + 1));
		buffstr = id + buffstr;
		buffstr = "�û� " + buffstr;
		strcpy(buff, buffstr.c_str());
		// ������Ϣ
		char sendbuff[MaxBufSize];
		memset(sendbuff, 0, MaxBufSize);
		Message ssend;
		ssend.set_Encryption(1);
		ssend.setdata(buff);
		for (int i = 0; i < Clinub; i++)
		{
			if (Cli != Clis[i] && Client_alive[i] == 1)
			{
				ssend.setID(i + 1);
				ssend.tobuff(sendbuff);
				send(Clis[i], sendbuff, MaxBufSize, 0);
			}
		}
		memset(buff, 0, MaxBufSize);
		Sleep(300);
	}
	return 0;
}

void setalive(char *buff)
{
	for (int i = 0; i < MaxClient; i++)
		if (Client_alive[i] == 0)
			buff[10 + i] = '0';
		else
			buff[10 + i] = '1';
}

int main()
{
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "��ʼ��ʼ���������˲��������Ժ�" << endl;
	// ��ʼDLL
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsdata;
	if (WSAStartup(sockVersion, &wsdata) != 0)
	{
		cout << "��ʼ��DLLʧ�ܣ�" << endl;
		return 1;
	}
	cout << "��ʼ��DLL�ɹ���" << endl;
	// ������ʽ�׽���
	SOCKET serversock = socket(AF_INET, SOCK_STREAM, 0);
	if (serversock == INVALID_SOCKET)
	{
		cout << "������ʽ�׽���ʧ�ܣ�" << endl;
		return 1;
	}
	// ����ʽ�׽���
	SOCKADDR_IN serversockaddr;
	serversockaddr.sin_family = AF_INET;
	serversockaddr.sin_port = htons(PORT);
	serversockaddr.sin_addr.s_addr = inet_addr(IP);
	if (bind(serversock, (LPSOCKADDR)&serversockaddr, sizeof(serversockaddr)) == SOCKET_ERROR)
	{
		cout << "����ʽ�׽���ʧ�ܣ�" << endl;
		return 1;
	}
	cout << "����������ʽ�׽��ֳɹ���" << endl;
	// ��ʼ����
	if (listen(serversock, MaxClient) == SOCKET_ERROR)
	{
		cout << "������������ʧ�ܣ�" << endl;
		return 1;
	}
	cout << "�ɹ���ʼ�����ͻ��ˣ�" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "��ʼ�ȴ��ͻ�������" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	while (1)
	{
		if (Clinub < MaxClient)
		{
			SOCKET ClientSocket = accept(serversock, 0, 0);
			if (ClientSocket == INVALID_SOCKET)
			{
				cout << "���ܿͻ�������ʧ�ܣ�" << endl;
				return 1;
			}
			// ����++��ȷ�����ܵ�һ�����õ�SOCKET��ִ��
			Clinub++;
			Clis.push_back(ClientSocket);
			// ������Ҫ��֤�������룬Ҫ��powershellΪGBK���룬һ������Ϊ�����ַ�
			char buff[MaxBufSize] = "����idΪ:";
			buff[9] = '0' + Clinub;
			// �����Щ�û��Ѿ��ڷ�������
			setalive(buff);
			buff[30] = 0;
			// ������Ϣ
			char sendbuff[MaxBufSize];
			memset(sendbuff, 0, MaxBufSize);
			Message ssend;
			ssend.setID(Clinub);
			ssend.set_Encryption(1);
			ssend.setdata(buff);
			ssend.tobuff(sendbuff);
			// ����
			send(ClientSocket, sendbuff, 100, 0);
			// ��֪���û�
			memset(sendbuff, 0, MaxBufSize);
			char newbuff[MaxBufSize] = "�û� i ���ӵ�������!";
			newbuff[5] = '0' + Clinub;
			ssend.setdata(newbuff);
			for (int i = 0; i < Clinub; i++)
			{
				if (ClientSocket != Clis[i] && Client_alive[i] == 1)
				{
					ssend.setID(i + 1);
					ssend.tobuff(sendbuff);
					send(Clis[i], sendbuff, MaxBufSize, 0);
				}
			}
			//
			SYSTEMTIME st = {0};
			GetLocalTime(&st);
			cout << "(" << st.wMonth << "��" << st.wDay << "��" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "�û� " << Clinub << " ���ӵ���������" << endl;
			// ��ʼ�ַ��̴߳���
			HANDLE hThread = CreateThread(NULL, NULL, &SeverThread, LPVOID(Clinub - 1), 0, NULL);
			CloseHandle(hThread);
		}
		else
		{
			cout << "�ܱ�Ǹ���������Ѵ︺�����ޣ���ȴ���" << endl;
		}
	}
	closesocket(serversock);
	WSACleanup();
	return 0;
}