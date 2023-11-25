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
	// 设计：第一位为用户的id（不大于MaxClient），第二位为加密位（1表示采取异或加密，0表示不加密）
	// 第三到九位为标志位（也就是我们所认证的服务器和客户端的标志码）（这里设计为我的学号2112495）
	// 同时标志位的每一位转换为十进制，7个数字加起来，作为解码的key，如果加密位为1，对后面的数据位执行异或解密操作
	// 后面到MaxBufSize的都为数据位
private:
	char ID; // ID为索引加一（也就是显示的ID）
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
		// 空字符不处理
		if (buff[0] == 0)
			continue;
		cout << "------------------------------------------------------------------------------------" << endl;
		// 编码消息
		char sendbuff[MaxBufSize];
		memset(sendbuff, 0, MaxBufSize);
		Message ssend;
		ssend.setID(SOCKID);
		ssend.set_Encryption(1);
		ssend.setdata(buff);
		ssend.tobuff(sendbuff);
		// 发送
		if (send(*Cli, sendbuff, MaxBufSize, 0) == SOCKET_ERROR)
		{
			cout << "发送失败，请检查您的连接！" << endl;
			return 1;
		}
		if (strcmp(buff, "/quit") == 0)
		{
			// 退出
			SYSTEMTIME st = {0};
			GetLocalTime(&st);
			cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "您成功退出聊天室，程序关闭!" << endl;
			cout << "------------------------------------------------------------------------------------" << endl;
			isalive = 0;
			closesocket(*Cli);
			exit(0);
		}
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
			 << " ";
		cout << "消息已经成功发送:" << buff << endl;
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
			cout << "从目标服务器接受消息失败，请检查连接！" << endl;
			closesocket(*Cli);
			return 1;
		}
		else if (state == 0)
		{
			exit(0);
		}
		// 处理消息
		Message recv(getbuff);
		if (recv.getID() != SOCKID)
		{
			cout << "接受到错误信息，即将断开连接" << endl;
			closesocket(*Cli);
			return 1;
		}
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "检测到被未授权用户入侵，即将断开连接" << endl;
			closesocket(*Cli);
			return 1;
		}
		// 处理数据
		if (buff[0] == 0)
			continue;
		cout << "------------------------------------------------------------------------------------" << endl;
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
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
	cout << "欢迎！目前在线的用户为：";
	for (int i = 10; i < 30; i++)
		if (buff[i] == '1')
			cout << i - 9 << "、";
	cout << endl;
}

int main()
{
	// 初始化DLL
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "欢迎使用简易聊天室！正在初始化客户端参数，请等待。" << endl;
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsdata;
	if (WSAStartup(sockVersion, &wsdata) != 0)
	{
		cout << "初始化DLL失败！" << endl;
		return 1;
	}
	cout << "初始化DLL成功！" << endl;
	// 创建流式套接字
	SOCKET clientsock = socket(AF_INET, SOCK_STREAM, 0);
	if (clientsock == INVALID_SOCKET)
	{
		cout << "创建流式套接字失败！" << endl;
		return 1;
	}
	// 设置流式套接字
	SOCKADDR_IN clientsockaddr;
	clientsockaddr.sin_family = AF_INET;
	clientsockaddr.sin_port = htons(PORT);
	clientsockaddr.sin_addr.s_addr = inet_addr(IP);
	cout << "创建并初始化流式套接字成功！" << endl;
	// 开始连接
	if (connect(clientsock, (SOCKADDR *)&clientsockaddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		cout << "连接到目标服务器失败!" << endl;
		return 0;
	}
	cout << "您已成功连接到目标服务器,服务器IP为:" << IP << "端口为:" << PORT << endl;
	// 获取服务器分配给我们的ID
	while (1)
	{
		// 与服务器对接，20个字符大小
		char getbuff[MaxBufSize];
		char buff[MaxBufSize];
		memset(getbuff, 0, MaxBufSize);
		memset(buff, 0, MaxBufSize);
		recv(clientsock, getbuff, 100, 0);
		// 处理消息
		Message recv(getbuff);
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "检测到被未授权用户入侵，即将断开连接" << endl;
			closesocket(clientsock);
			WSACleanup();
			return 0;
		}
		// 处理数据
		if (buff[0])
		{
			getalive(buff);
			buff[10] = 0;
			cout << "依据服务器分配，" << buff << endl;
			SOCKID = buff[9] - '0';
			break;
		}
		Sleep(300);
	}
	cout << "现在您可以在下面输入消息开始聊天了！" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	isalive = 1;
	// 多线程处理接收和发送
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
