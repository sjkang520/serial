#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <iostream>
#define ERR_COMMUNICATION -1//接收的起始数据出错
#define ERR_READ -2//数据读取出错
#define rsspeed 50
//#define ERR_TIMEOUT -3
class Serial
{
public:
	Serial();
	~Serial();
	//打开串口，默认波特率57600，数据位8位，停止位一位，无奇偶校验位
	bool OpenSeialPort(const TCHAR* port, UINT baud_rate = CBR_57600, UINT databits = DATABITS_8, BYTE stop_bit = ONESTOPBIT, BYTE parity = NOPARITY);
	void ClosePort();
	bool OpenListenThread();
	bool CloseListenTread();
	bool WriteData(unsigned char* pData, unsigned int length);//写串口数据
    int ReadData(unsigned char* data);//读串口数据//, DWORD length, UINT BytesInCom);
	int ReadDataTh(unsigned char* data, UINT bytesInCom, DWORD& length);//线程接收串口数据的函数（没有测试）

	bool send_cmd(unsigned char cmd, unsigned char len, unsigned char *data);//发送命令

	int check_card(unsigned char _Mode, unsigned long* snr);//寻卡并选卡
	int load_key(unsigned char mode, unsigned char sector, unsigned char* bufkey);//下载密码A
	int auth_key(unsigned char mode, unsigned char sector);//验证密码
	int read_block(unsigned char address, unsigned char* databuf);//读一个块，16字节
	int write_block(unsigned char address, unsigned char* databuf);//写一个块，16字节
	int write_card(unsigned char data[][16]);//写卡，一次写4个块，一共六十四字节
	int read_card(unsigned char buffer[][16]);//读卡，一次读4个块，一共六十四字节
	bool ReadChar(char& rChar);//从缓冲区中读取一个字符
	UINT GetBytesInCom();//获取串口输入缓冲区的字节数
	void beep(int msecond,int time);//蜂鸣器
	static UINT WINAPI ListenThread(void* pParam);
private:
    HANDLE m_hCom;
	static unsigned char _TKey[16][8]; 
	static unsigned char key[2][8];
	static unsigned char buffer[256];
	static bool s_bExit;//线程退出标志变量
	//线程句柄
	volatile HANDLE m_hListenThread;
	CRITICAL_SECTION m_csCommunicationSync;//互斥操作串口
};

#endif