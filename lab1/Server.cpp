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

DWORD WINAPI SeverThread(LPVOID Socketindex)
{
	SOCKET Cli = Clis[(long long)Socketindex];
	if (Cli == INVALID_SOCKET)
	{
		cout << "为用户创建处理进程失败！" << endl;
		return 1;
	}
	Client_alive[(long long)Socketindex] = 1;
	char getbuff[MaxBufSize];
	char buff[MaxBufSize];
	memset(buff, 0, MaxBufSize);
	// 因为是接收后才发送，所以不需要多线程
	while (1)
	{
		// 开始接受客户机的数据
		recv(Cli, getbuff, MaxBufSize, 0);
		// 处理消息
		Message recv(getbuff);
		if (recv.getID() != (long long)Socketindex + 1)
		{
			cout << "接受到错误信息" << endl;
			continue;
		}
		recv.getdata(buff);
		if (recv.get_Authorize() == 0)
		{
			cout << "检测到被未授权用户入侵，即将关闭与该用户的连接" << endl;
			memset(buff, 0, MaxBufSize);
			strcpy(buff, "/quit");
			// 进入用户退出部分处理
		}
		// 空字符串和不知名BUG不处理
		if (buff[0] == 0)
			continue;
		if (strcmp(buff, "/quit") == 0)
		{
			// 用户退出
			SYSTEMTIME st = {0};
			GetLocalTime(&st);
			cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "用户 " << (long long)Socketindex + 1 << " 退出聊天室！" << endl;
			string buf = "用户 ";
			buf += '1' + (long long)Socketindex;
			buf += " 退出聊天";
			strcpy(buff, buf.c_str());
			// 编码消息
			char sendbuff[MaxBufSize];
			memset(sendbuff, 0, MaxBufSize);
			Message ssend;
			// ssend.setID(i + 1);
			ssend.set_Encryption(1);
			ssend.setdata(buff);
			// ssend.tobuff(sendbuff);
			//  群发消息
			for (int i = 0; i < Clinub; i++)
			{
				if (Cli != Clis[i] && Client_alive[i] == 1)
				{
					ssend.setID(i + 1);
					ssend.tobuff(sendbuff);
					send(Clis[i], sendbuff, MaxBufSize, 0);
				}
			}
			// 关闭SOCKET
			Client_alive[(long long)Socketindex] = 0;
			closesocket(Cli);
			return 0;
		}
		// 正常群发消息
		SYSTEMTIME st = {0};
		GetLocalTime(&st);
		cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
			 << " ";
		cout << "用户 " << (long long)Socketindex + 1 << " :" << buff << endl;
		string buffstr(buff);
		buffstr = " :" + buffstr;
		char id = ('0' + ((long long)Socketindex + 1));
		buffstr = id + buffstr;
		buffstr = "用户 " + buffstr;
		strcpy(buff, buffstr.c_str());
		// 编码消息
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
	cout << "开始初始化服务器端参数，请稍后。" << endl;
	// 初始DLL
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsdata;
	if (WSAStartup(sockVersion, &wsdata) != 0)
	{
		cout << "初始化DLL失败！" << endl;
		return 1;
	}
	cout << "初始化DLL成功！" << endl;
	// 创建流式套接字
	SOCKET serversock = socket(AF_INET, SOCK_STREAM, 0);
	if (serversock == INVALID_SOCKET)
	{
		cout << "创建流式套接字失败！" << endl;
		return 1;
	}
	// 绑定流式套接字
	SOCKADDR_IN serversockaddr;
	serversockaddr.sin_family = AF_INET;
	serversockaddr.sin_port = htons(PORT);
	serversockaddr.sin_addr.s_addr = inet_addr(IP);
	if (bind(serversock, (LPSOCKADDR)&serversockaddr, sizeof(serversockaddr)) == SOCKET_ERROR)
	{
		cout << "绑定流式套接字失败！" << endl;
		return 1;
	}
	cout << "创建并绑定流式套接字成功！" << endl;
	// 开始监听
	if (listen(serversock, MaxClient) == SOCKET_ERROR)
	{
		cout << "创建监听请求失败！" << endl;
		return 1;
	}
	cout << "成功开始监听客户端！" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	cout << "开始等待客户端连接" << endl;
	cout << "------------------------------------------------------------------------------------" << endl;
	while (1)
	{
		if (Clinub < MaxClient)
		{
			SOCKET ClientSocket = accept(serversock, 0, 0);
			if (ClientSocket == INVALID_SOCKET)
			{
				cout << "接受客户机连接失败！" << endl;
				return 1;
			}
			// 在这++，确保接受到一个可用的SOCKET后执行
			Clinub++;
			Clis.push_back(ClientSocket);
			// 我们需要保证中文输入，要求powershell为GBK编码，一个中文为两个字符
			char buff[MaxBufSize] = "您的id为:";
			buff[9] = '0' + Clinub;
			// 表达哪些用户已经在服务器上
			setalive(buff);
			buff[30] = 0;
			// 编码消息
			char sendbuff[MaxBufSize];
			memset(sendbuff, 0, MaxBufSize);
			Message ssend;
			ssend.setID(Clinub);
			ssend.set_Encryption(1);
			ssend.setdata(buff);
			ssend.tobuff(sendbuff);
			// 发送
			send(ClientSocket, sendbuff, 100, 0);
			// 告知新用户
			memset(sendbuff, 0, MaxBufSize);
			char newbuff[MaxBufSize] = "用户 i 连接到服务器!";
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
			cout << "(" << st.wMonth << "月" << st.wDay << "日" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << ")"
				 << " ";
			cout << "用户 " << Clinub << " 连接到服务器！" << endl;
			// 开始分发线程处理
			HANDLE hThread = CreateThread(NULL, NULL, &SeverThread, LPVOID(Clinub - 1), 0, NULL);
			CloseHandle(hThread);
		}
		else
		{
			cout << "很抱歉，服务器已达负载上限，请等待。" << endl;
		}
	}
	closesocket(serversock);
	WSACleanup();
	return 0;
}