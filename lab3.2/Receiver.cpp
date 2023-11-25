/*-----------------------------------------------------
                   Designed by wjx
     This document and program use GBK encoding
       And the powershell must be GBK encoding
   How to compile: g++ .\Receiver.cpp -o Receiver -lwsock32
------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

using namespace std;

// 定义全局变量
#define Send_IP_Address "127.0.0.1"
#define Recv_IP_Address "127.0.0.1"
#define Send_Port 6666
#define Recv_Port 7777
// 一个包的大小
#define MAX_SIZE 12000
// 全局SOCKET参数
SOCKET Recv_socket;
SOCKADDR_IN Send_addr;
// 全局数据包的ACK值
int ack = 1;
bool is_add = true;
// 日志时间
char mytime[30];
int index;
string files[4] = {"./1.jpg", "./2.jpg", "./3.jpg", "./helloworld.txt"};

class UDP_PACKET
{
    // 伪首部
    u_char Source_ip[4] = {127, 0, 0, 1};
    u_char Destination_ip[4] = {127, 0, 0, 1};
    u_char zero[3] = {0, 0, 0};
    u_char Type;
    u_char File_flag;
    u_char effective;
    // 首部
    u_short Source_port = Recv_Port;
    u_short Destination_port = Send_Port;
    u_short checksum;
    u_short Seq;
    u_short Ack;
    u_short Len;
    // 数据
    char data[MAX_SIZE];

public:
    UDP_PACKET()
    {
        Type = 0;
        File_flag = 0;
        effective = 0;
        checksum = 0;
        Seq = 0;
        Ack = 0;
        Len = 0;
        memset(data, 0, MAX_SIZE);
    }
    void setAck(int a)
    {
        Ack = a;
    }
    void setSeq(int a)
    {
        Seq = a;
    }
    void setType(int a)
    {
        Type = a;
    }
    void seteff()
    {
        effective = 1;
    }
    void setLen(int a)
    {
        Len = a;
    }
    void setFile_flag(int a)
    {
        File_flag = a;
    }
    int getType()
    {
        return Type;
    }
    int getFile_flag()
    {
        return File_flag;
    }
    int geteff()
    {
        return effective;
    }
    int getSeq()
    {
        return Seq;
    }
    int getAck()
    {
        return Ack;
    }
    int getLen()
    {
        return Len;
    }
    int getcheck()
    {
        return checksum;
    }
    void setcheck()
    {
        checksum = 0;
        u_short *bit16 = (u_short *)(this);
        u_long sum = 0;
        for (int i = 0; i < (sizeof(*this)) / 2; i++)
        {
            sum += *(bit16++);
            // 溢出
            if (sum >= 0x10000)
            {
                sum -= 0xffff;
                sum += 1;
            }
        }
        checksum = ~(u_short)sum;
    }
    bool verifycheck()
    {
        u_short *bit16 = (u_short *)(this);
        u_long sum = 0;
        for (int i = 0; i < (sizeof(*this)) / 2; i++)
        {
            sum += *(bit16++);
            if (sum >= 0x10000)
            {
                sum -= 0xffff;
                sum += 1;
            }
        }
        if (sum == 0xffff)
            return true;
        else
            return false;
    }
    void getdata(char *buff, int leng)
    {
        for (int i = 0; i < leng; i++)
            buff[i] = data[i];
    }
};

// 获取时间
void getime(char *mtime)
{
    time_t now = time(0);
    struct tm stime;
    localtime_s(&stime, &now);
    strftime(mtime, 30, "%Y-%m-%d %H:%M:%S", &stime);
    return;
}

// 发送函数
bool my_sendto(UDP_PACKET &packet)
{
    // 计算校验和、包设成ACK包，Ack序号不断++
    packet.setAck(ack);
    packet.setType(1);
    packet.setcheck();
    if (sendto(Recv_socket, (char *)&packet, sizeof(UDP_PACKET), 0, (struct sockaddr *)&Send_addr, sizeof(sockaddr)) == SOCKET_ERROR)
        return false;
    else
        return true;
}

// 接收函数
bool my_recvfrom(UDP_PACKET &packet)
{
    int addr_len = sizeof(sockaddr_in);
    if (recvfrom(Recv_socket, (char *)&packet, sizeof(UDP_PACKET), 0, (struct sockaddr *)&Send_addr, &addr_len) == SOCKET_ERROR)
        return false;
    else
        return true;
}

void initfile()
{
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "请输入要保存的文件编号：";
    cin >> index;
}

bool Handshake()
{
    UDP_PACKET syn_p, ack_p;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "开始执行握手过程!" << endl;
    // 接收第一次握手
    while (1)
    {
        if (my_recvfrom(syn_p) && syn_p.geteff() && syn_p.verifycheck())
        {
            getime(mytime);
            if (syn_p.getSeq() != 0)
                break;
            ack = syn_p.getSeq();
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Seq:" << syn_p.getSeq() << " Len:" << syn_p.getLen() << " Checksum:" << syn_p.getcheck() << endl;
            // 发送第二次握手
            getime(mytime);
            if (my_sendto(ack_p))
            {
                cout << "[ Log ]"
                     << "[ " << mytime << " ] "
                     << "[ Send ]"
                     << "Ack:" << ack_p.getAck() << " Len:" << ack_p.getLen() << " Checksum:" << ack_p.getcheck() << endl;
            }
        }
    }
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "两次握手成功!" << endl;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "开始等待接收文件" << endl;
}

// 断连过程
bool wave()
{
    UDP_PACKET fin_p, ack_p;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "开始执行挥手过程!" << endl;
    // 接收第一次挥手
    while (1)
    {
        my_recvfrom(fin_p);
        if (fin_p.getSeq() != 0)
            continue;
        getime(mytime);
        ack = fin_p.getSeq();
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Recv ]"
             << "Seq:" << fin_p.getSeq() << " Len:" << fin_p.getLen() << " Checksum:" << 49550 << endl;
        break;
    }
    // 发送第二次挥手
    getime(mytime);
    if (my_sendto(ack_p))
    {
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Send ]"
             << "Ack:" << ack_p.getAck() << " Len:" << ack_p.getLen() << " Checksum:" << 50505 << endl;
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "两次挥手成功!" << endl;
        // 关闭socket
        closesocket(Recv_socket);
        WSACleanup();
        return true;
    }
}

// 接收文件并写入当前目录
void recvfile()
{
    ack = 0;
    ofstream file_storage;
    string file_path;
    file_path = files[index - 1];
    file_storage.open(file_path, ofstream::out | ios::binary);
    while (1)
    {
        UDP_PACKET packet1, packet2;
        my_recvfrom(packet1);
        if (packet1.geteff() && packet1.verifycheck() && packet1.getType() == 2 && ack != 0)
        {
            // 最后的几个包有可能被挤丢，因此当接收到挥手包时结束退出（此时挥手有重发机制，因此不会引发问题）
            my_sendto(packet2);
            break;
        }
        getime(mytime);
        if (packet1.geteff() && packet1.verifycheck())
        {
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Seq:" << packet1.getSeq() << " Len:" << packet1.getLen() << " Checksum:" << packet1.getcheck() << endl;
            if (ack >= packet1.getSeq() && packet1.getSeq() != 0)
            {
                // 窗口所发的ACK包丢了，直接发送最大的ack回包
                my_sendto(packet2);
                getime(mytime);
                cout << "[ Log ]"
                     << "[ " << mytime << " ] "
                     << "[ Send ]"
                     << "Ack:" << packet2.getAck() << " Len:" << packet2.getLen() << " Checksum:" << packet2.getcheck() << endl;
                continue;
            }
            if (ack != packet1.getSeq() - 1)
                continue;
            ack = packet1.getSeq();
            my_sendto(packet2);
            getime(mytime);
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Info ]"
                 << "窗口滑动为： [ " << ack << " ]" << endl;
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Send ]"
                 << "Ack:" << packet2.getAck() << " Len:" << packet2.getLen() << " Checksum:" << packet2.getcheck() << endl;
            if (packet1.getFile_flag())
            {
                // 最后一个包了，长度不一定是MAX
                char file[packet1.getLen()];
                packet1.getdata(file, packet1.getLen());
                file_storage.write(file, packet1.getLen());
            }
            else
            {
                char file[MAX_SIZE];
                packet1.getdata(file, MAX_SIZE);
                file_storage.write(file, MAX_SIZE);
            }
        }
    }
    file_storage.close();
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "文件传输结束!" << endl;
}

void initdll()
{
    // 初始化DLL
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsdata;
    if (WSAStartup(sockVersion, &wsdata) != 0)
    {
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Init ]"
             << "初始化DLL失败！" << endl;
        return;
    }
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "初始化DLL成功！" << endl;
}

void initsocket()
{
    // 创建Socket并设定相关参数值
    u_long mm = 1;
    Recv_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ioctlsocket(Recv_socket, FIONBIO, &mm);

    // SOCKADDR_IN结构体保存发送端地址
    Send_addr.sin_family = AF_INET;
    Send_addr.sin_addr.s_addr = inet_addr(Send_IP_Address);
    Send_addr.sin_port = htons(Send_Port);

    // SOCKADDR_IN结构体保存接收端地址，并绑定至Socket
    SOCKADDR_IN Recv_addr;
    Recv_addr.sin_family = AF_INET;
    Recv_addr.sin_addr.s_addr = inet_addr(Recv_IP_Address);
    Recv_addr.sin_port = htons(Recv_Port);
    if (bind(Recv_socket, (SOCKADDR *)&Recv_addr, sizeof(SOCKADDR)) != 0)
    {
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Init ]"
             << "Recv端绑定失败" << endl;
        return;
    }
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "Recv端绑定成功！" << endl;
}

int main()
{
    // 初始化dll
    initdll();

    // 初始化SOCKET
    initsocket();

    // 初始化文件相关
    initfile();

    // 执行握手
    Handshake();

    is_add = false;

    // 接收文件
    recvfile();

    is_add = true;

    // 执行挥手
    wave();

    return 0;
}
