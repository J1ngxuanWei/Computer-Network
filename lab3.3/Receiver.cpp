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

// ����ȫ�ֱ���
#define Send_IP_Address "127.0.0.1"
#define Recv_IP_Address "127.0.0.1"
#define Send_Port 6666
#define Recv_Port 7777
// һ�����Ĵ�С
#define MAX_SIZE 12000
// ȫ��SOCKET����
SOCKET Recv_socket;
SOCKADDR_IN Send_addr;
// ȫ�����ݰ���ACKֵ
int ack = 1;
bool is_add = true;
int *ackpa;
int packnub = 0;
// ��־ʱ��
char mytime[30];
int index;
string files[4] = {"./1.jpg", "./2.jpg", "./3.jpg", "./helloworld.txt"};
// ����ָ��
int win_left = 1;
int win_right = 1;
int window = 0;

class UDP_PACKET
{
    // α�ײ�
    u_char Source_ip[4] = {127, 0, 0, 1};
    u_char Destination_ip[4] = {127, 0, 0, 1};
    u_char zero[3] = {0, 0, 0};
    u_char Type;
    u_char File_flag;
    u_char effective;
    // �ײ�
    u_short Source_port = Recv_Port;
    u_short Destination_port = Send_Port;
    u_short checksum;
    u_short Seq;
    u_short Ack;
    u_short Len;
    // ����
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
            // ���
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

class UDP_BUFF
{
private:
    UDP_PACKET *buff;
    // ��¼������
    int base;

public:
    UDP_BUFF()
    {
        base = 1;
    }
    void setinit()
    {
        buff = new UDP_PACKET[window];
    }
    void setbuff(UDP_PACKET &a, int b)
    {
        memcpy(buff + (b - base), &a, sizeof(UDP_PACKET));
    }
    void getbuff(UDP_PACKET &a, int b)
    {
        memcpy(&a, buff + (b - base), sizeof(UDP_PACKET));
    }
    void slide()
    {
        base++;
        int i = 0;
        for (; i < window - 1; i++)
            memcpy(buff + i, buff + i + 1, sizeof(UDP_PACKET));
        memset(buff + i, 0, sizeof(UDP_PACKET));
    }
};

UDP_BUFF mybuff;

// ��ȡʱ��
void getime(char *mtime)
{
    time_t now = time(0);
    struct tm stime;
    localtime_s(&stime, &now);
    strftime(mtime, 30, "%Y-%m-%d %H:%M:%S", &stime);
    return;
}

// ���ͺ���
bool my_sendto(UDP_PACKET &packet, int myack = -1)
{
    // ����У��͡������ACK����Ack��Ų���++
    if (myack == -1)
        packet.setAck(ack);
    else
        packet.setAck(myack);
    packet.setType(1);
    packet.setcheck();
    if (sendto(Recv_socket, (char *)&packet, sizeof(UDP_PACKET), 0, (struct sockaddr *)&Send_addr, sizeof(sockaddr)) == SOCKET_ERROR)
        return false;
    else
        return true;
}

// ���պ���
bool my_recvfrom(UDP_PACKET &packet)
{
    int addr_len = sizeof(sockaddr_in);
    if (recvfrom(Recv_socket, (char *)&packet, sizeof(UDP_PACKET), 0, (struct sockaddr *)&Send_addr, &addr_len) == SOCKET_ERROR)
        return false;
    else
        return true;
}

void my_slide()
{
    win_left++;
    win_right++;
    if (win_right >= packnub + 1)
        win_right = packnub + 1;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "���ڻ���Ϊ�� [ " << win_left << " , " << win_right << " ]" << endl;
}

void initfile()
{
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "�����봰�ڴ�С:";
    cin >> window;
    win_right = window + 1;
    mybuff.setinit();
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "������Ҫ������ļ���ţ�";
    cin >> index;
}

void initwindow()
{
    int istr = 0;
    while (1)
    {
        UDP_PACKET packet1, packet2;
        my_recvfrom(packet1);
        if (packet1.geteff() && packet1.verifycheck())
        {
            if (packet1.getSeq() != 0)
                break;
            ;
            getime(mytime);
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Seq:" << packet1.getSeq() << " Len:" << packet1.getLen() << " Checksum:" << packet1.getcheck() << endl;
            // ���ͻذ�
            packet2.setLen(21);
            my_sendto(packet2, 0);
            getime(mytime);
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Send ]"
                 << "Ack:" << packet2.getAck() << " Len:" << packet2.getLen() << " Checksum:" << packet2.getcheck() << endl;
            // �����ز�������ʼ������
            packnub = packet1.getLen();
            ackpa = new int[packnub + 1];
            for (int i = 0; i < packnub + 1; i++)
                ackpa[i] = 0;
            istr = 1;
        }
    }
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "�ɹ����յ��ļ��ܰ�����" << packnub << endl;
}

bool Handshake()
{
    UDP_PACKET syn_p, ack_p;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "��ʼִ�����ֹ���!" << endl;
    // ���յ�һ������
    while (1)
    {
        if (my_recvfrom(syn_p) && syn_p.geteff() && syn_p.verifycheck())
        {
            getime(mytime);
            ack = syn_p.getSeq();
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Seq:" << syn_p.getSeq() << " Len:" << syn_p.getLen() << " Checksum:" << syn_p.getcheck() << endl;
            // ���͵ڶ�������
            getime(mytime);
            if (my_sendto(ack_p))
            {
                cout << "[ Log ]"
                     << "[ " << mytime << " ] "
                     << "[ Send ]"
                     << "Ack:" << ack_p.getAck() << " Len:" << ack_p.getLen() << " Checksum:" << ack_p.getcheck() << endl;
            }
            break;
        }
    }
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "�������ֳɹ�!" << endl;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "��ʼ�ȴ������ļ�" << endl;
}

// ��������
bool wave()
{
    UDP_PACKET fin_p, ack_p;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "��ʼִ�л��ֹ���!" << endl;
    // ���յ�һ�λ���
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
    // ���͵ڶ��λ���
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
             << "���λ��ֳɹ�!" << endl;
        // �ر�socket
        closesocket(Recv_socket);
        WSACleanup();
        return true;
    }
}

// �����ļ���д�뵱ǰĿ¼
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
        getime(mytime);
        if (packet1.geteff() && packet1.verifycheck() && packet1.getType() == 2 && ack != 0)
        {
            // ���ļ������п��ܱ���������˵����յ����ְ�ʱ�����˳�����ʱ�������ط����ƣ���˲����������⣩
            my_sendto(packet2, ack);
            break;
        }
        if (packet1.geteff() && packet1.verifycheck() && packet1.getSeq() != 0)
        {
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Recv ]"
                 << "Seq:" << packet1.getSeq() << " Len:" << packet1.getLen() << " Checksum:" << packet1.getcheck() << endl;
            ack = packet1.getSeq();
            my_sendto(packet2, ack);
            getime(mytime);
            cout << "[ Log ]"
                 << "[ " << mytime << " ] "
                 << "[ Send ]"
                 << "Ack:" << packet2.getAck() << " Len:" << packet2.getLen() << " Checksum:" << packet2.getcheck() << endl;
            // �����
            if (ackpa[ack] == 0)
            {
                // û�б������
                ackpa[ack] = 1;
                mybuff.setbuff(packet1, ack);
            }
            // ��������
            while (ackpa[win_left] != 0)
            {
                // ��ʱ���ڵ�����˵ĳɹ����յ��ˣ���˻���
                if (win_left > packnub)
                    break;
                UDP_PACKET buffpack;
                mybuff.getbuff(buffpack, win_left);
                mybuff.slide(); // �������洰��
                // д���ļ�
                if (buffpack.getFile_flag())
                {
                    // ���һ�����ˣ����Ȳ�һ����MAX
                    char file[buffpack.getLen()];
                    buffpack.getdata(file, buffpack.getLen());
                    file_storage.write(file, buffpack.getLen());
                }
                else
                {
                    char file[MAX_SIZE];
                    buffpack.getdata(file, MAX_SIZE);
                    file_storage.write(file, MAX_SIZE);
                }
                my_slide();
            }
        }
    }
    file_storage.close();
    delete[] ackpa;
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Info ]"
         << "�ļ��������!" << endl;
}

void initdll()
{
    // ��ʼ��DLL
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsdata;
    if (WSAStartup(sockVersion, &wsdata) != 0)
    {
        getime(mytime);
        cout << "[ Log ]"
             << "[ " << mytime << " ] "
             << "[ Init ]"
             << "��ʼ��DLLʧ�ܣ�" << endl;
        return;
    }
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "��ʼ��DLL�ɹ���" << endl;
}

void initsocket()
{
    // ����Socket���趨��ز���ֵ
    u_long mm = 1;
    Recv_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ioctlsocket(Recv_socket, FIONBIO, &mm);

    // SOCKADDR_IN�ṹ�屣�淢�Ͷ˵�ַ
    Send_addr.sin_family = AF_INET;
    Send_addr.sin_addr.s_addr = inet_addr(Send_IP_Address);
    Send_addr.sin_port = htons(Send_Port);

    // SOCKADDR_IN�ṹ�屣����ն˵�ַ��������Socket
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
             << "Recv�˰�ʧ��" << endl;
        return;
    }
    getime(mytime);
    cout << "[ Log ]"
         << "[ " << mytime << " ] "
         << "[ Init ]"
         << "Recv�˰󶨳ɹ���" << endl;
}

int main()
{
    // ��ʼ��dll
    initdll();

    // ��ʼ��SOCKET
    initsocket();

    // ��ʼ���ļ����
    initfile();

    // ִ������
    Handshake();

    is_add = false;

    // ��ʼ������
    initwindow();

    // �����ļ�
    recvfile();

    is_add = true;

    // ִ�л���
    wave();

    return 0;
}