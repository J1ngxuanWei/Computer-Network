/*-----------------------------------------------------
                   Designed by wjx
     This document and program use GBK encoding
       And the powershell must be GBK encoding
   How to compile: g++ .\Sender.cpp -o Sender -lwsock32
------------------------------------------------------*/
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <time.h>
#include <chrono>

using namespace std;

// 定义全局变量
#define Send_IP_Address "127.0.0.1"
#define Recv_IP_Address "127.0.0.1"
#define Send_Port 6666
#define Recv_Port 8888
// 一个包的大小
#define MAX_SIZE 12000
// 全局SOCKET参数
SOCKET Recv_socket;
SOCKADDR_IN Send_addr;
// 全局数据包的ACK值
bool isfile = true;
int seq = 0;
// 日志时间
char mytime[30];
chrono::_V2::steady_clock::time_point mystart;
chrono::_V2::steady_clock::time_point myend;
// 文件
bool is_send = false;
int packet_num = 0;
char *file;
int file_size = 0;
int index;
string files[4] = {"./测试文件/1.jpg", "./测试文件/2.jpg", "./测试文件/3.jpg", "./测试文件/helloworld.txt"};
// 窗口指针
int win_left = 0;
int win_right = 8;
// 线程锁
int is_log1 = 0;
int is_log2 = 0;

// 报文数据包类
class UDP_PACKET
{
private:
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
        if ((sum == 0xffff))
            return true;
        else
            return false;
    }
    void setdata(char *buff, int start, int end)
    {
        int str = 0;
        for (int i = start; i < end; i++)
            data[str++] = file[i];
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
    // 计算校验和
    packet.seteff();
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
         << "请输入文件编号:";
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
    syn_p.setType(0); // SYN包
    getime(mytime);
    while (1)
    {
        // 发送第一次握手
        my_sendto(syn_p);
        isfile = false;
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Send ]"
             << "Seq:" << syn_p.getSeq() << " Len:" << syn_p.getLen() << " Checksum:" << syn_p.getcheck() << endl;
        // 接收第二次握手
        my_recvfrom(ack_p);
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Recv ]"
             << "Ack:" << ack_p.getAck() << " Len:" << ack_p.getLen() << " Checksum:" << ack_p.getcheck() << endl;
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "两次握手成功!" << endl;
        isfile = true;
        return true;
    }
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
    fin_p.setType(2); // FIN包
    getime(mytime);
    while (1)
    {
        // 发送第一次挥手
        my_sendto(fin_p);
        isfile = false;
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Send ]"
             << "Seq:" << fin_p.getSeq() << " Len:" << fin_p.getLen() << " Checksum:" << 50505 << endl;
        // 接收第二次挥手
        my_recvfrom(ack_p);
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Recv ]"
             << "Ack:" << ack_p.getAck() << " Len:" << ack_p.getLen() << " Checksum:" << 49550 << endl;
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "二次挥手成功!" << endl;
        isfile = true;
        return true;
    }
}

void printdetial()
{
    std::chrono::duration<double, std::micro> elapsed = myend - mystart; // std::micro 表示以微秒为时间单位
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "文件大小为: " << file_size << " Byte" << endl;
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "传输文件时间:" << elapsed.count() / 1000000 << "s" << endl;
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "吞吐率:" << ((file_size / 1024)) / (elapsed.count() / 1000000) << "Kb/s" << endl;
    // 关闭socket
    closesocket(Recv_socket);
    WSACleanup();
}

// 读取文件
void readfile()
{
    string file_path;
    ifstream myfile;
    file_path = files[index - 1];
    // 打开文件
    myfile.open(file_path, ifstream::in | ios::binary);
    if (myfile)
    {
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "开始读取文件!" << endl;

        // 获取文件大小
        myfile.seekg(0, myfile.end);
        file_size = myfile.tellg();
        myfile.seekg(0, myfile.beg);
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "文件大小为: " << file_size << " Byte" << endl;
        // 存入file数组
        file = new char[file_size + 1];
        myfile.read(file, file_size);
        file[file_size] = '\0';
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Info ]"
             << "读取文件成功!" << endl;
        // 关闭文件
        myfile.close();
    }
    else
    {
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Erro ]"
             << "读取文件失败!" << endl;
    }
}

DWORD WINAPI sendfile(LPVOID Socketindex)
{
    // 发送文件
    packet_num = (file_size % MAX_SIZE != 0) ? (file_size / MAX_SIZE + 1) : (file_size / MAX_SIZE);
    mystart = std::chrono::steady_clock::now();
    // 文件传输
    // 定义窗口的扫描指针
    int current = 0;
    int windownub = 1;
    while (win_left != packet_num)
    {
        if (current == win_right)
        {
            // 一次窗口扫描完成，到达了右指针
            // 重置到左指针重新扫描
            current = win_left;
            // 定时50ms，时间到之后重新发送窗口内的所有包
            Sleep(50);
        }
        UDP_PACKET send_packet;
        if (current != packet_num - 1)
        {
            // 不是最后一个包
            send_packet.setSeq(current + 1);
            send_packet.setLen(MAX_SIZE);
            send_packet.setdata(file, current * MAX_SIZE, (current + 1) * MAX_SIZE);
            my_sendto(send_packet);
            getime(mytime);
            while (is_log2)
                ;
            is_log1 = 1;
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Send ]"
                 << "Seq:" << send_packet.getSeq() << " Len:" << send_packet.getLen() << " Checksum:" << send_packet.getcheck() << endl;
            is_log1 = 0;
        }
        else
        {
            // 最后一个文件包
            send_packet.setSeq(current + 1);
            send_packet.setFile_flag(1);
            send_packet.setLen(file_size - current * MAX_SIZE);
            send_packet.setdata(file, current * MAX_SIZE, (current)*MAX_SIZE + send_packet.getLen());
            my_sendto(send_packet);
            getime(mytime);
            while (is_log2)
                ;
            is_log1 = 1;
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Send ]"
                 << "Seq:" << send_packet.getSeq() << " Len:" << send_packet.getLen() << " Checksum:" << send_packet.getcheck() << endl;
            is_log1 = 0;
        }
        current++;
    }
    // 结束
    myend = std::chrono::steady_clock::now();
    is_send = false;
}

DWORD WINAPI recvAck(LPVOID Socketindex)
{
    while (is_send)
    {
        UDP_PACKET recv_packet;
        my_recvfrom(recv_packet);
        if (recv_packet.getType() == 1)
        {
            getime(mytime);
            while (is_log1)
                ;
            is_log2 = 1;
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Ack:" << recv_packet.getAck() << " Len:" << recv_packet.getLen() << " Checksum:" << recv_packet.getcheck() << endl;
            is_log2 = 0;
            // 滑动窗口
            if (recv_packet.getAck() > win_left)
            {
                if (recv_packet.getAck() == packet_num)
                    win_left = packet_num;
                else
                    win_left = recv_packet.getAck();
                if (win_left + 8 >= packet_num)
                    win_right = packet_num;
                else
                    win_right = win_left + 8;
            }
        }
    }
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
             << "Send端绑定失败" << endl;
        return;
    }
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "Send端绑定成功！" << endl;
}

int main()
{
    // 初始化dll
    initdll();

    // 初始化SOCKET
    initsocket();

    // 初始化文件相关
    initfile();

    // 两次握手
    Handshake();

    // 读取文件
    readfile();

    // 创建线程完成传输
    is_send = true;
    HANDLE hThread1 = CreateThread(NULL, NULL, &sendfile, LPVOID(1), 0, NULL);
    CloseHandle(hThread1);
    HANDLE hThread2 = CreateThread(NULL, NULL, &recvAck, LPVOID(1), 0, NULL);
    CloseHandle(hThread2);

    // 等待文件传输
    while (is_send)
    {
    }

    // 执行挥手
    wave();

    // 输出文件传输过程信息
    printdetial();

    return 0;
}
