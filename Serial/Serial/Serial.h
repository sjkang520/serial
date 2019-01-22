#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <iostream>
#define ERR_COMMUNICATION -1//���յ���ʼ���ݳ���
#define ERR_READ -2//���ݶ�ȡ����
#define rsspeed 50
//#define ERR_TIMEOUT -3
class Serial
{
public:
	Serial();
	~Serial();
	//�򿪴��ڣ�Ĭ�ϲ�����57600������λ8λ��ֹͣλһλ������żУ��λ
	bool OpenSeialPort(const TCHAR* port, UINT baud_rate = CBR_57600, UINT databits = DATABITS_8, BYTE stop_bit = ONESTOPBIT, BYTE parity = NOPARITY);
	void ClosePort();
	bool OpenListenThread();
	bool CloseListenTread();
	bool WriteData(unsigned char* pData, unsigned int length);//д��������
    int ReadData(unsigned char* data);//����������//, DWORD length, UINT BytesInCom);
	int ReadDataTh(unsigned char* data, UINT bytesInCom, DWORD& length);//�߳̽��մ������ݵĺ�����û�в��ԣ�

	bool send_cmd(unsigned char cmd, unsigned char len, unsigned char *data);//��������

	int check_card(unsigned char _Mode, unsigned long* snr);//Ѱ����ѡ��
	int load_key(unsigned char mode, unsigned char sector, unsigned char* bufkey);//��������A
	int auth_key(unsigned char mode, unsigned char sector);//��֤����
	int read_block(unsigned char address, unsigned char* databuf);//��һ���飬16�ֽ�
	int write_block(unsigned char address, unsigned char* databuf);//дһ���飬16�ֽ�
	int write_card(unsigned char data[][16]);//д����һ��д4���飬һ����ʮ���ֽ�
	int read_card(unsigned char buffer[][16]);//������һ�ζ�4���飬һ����ʮ���ֽ�
	bool ReadChar(char& rChar);//�ӻ������ж�ȡһ���ַ�
	UINT GetBytesInCom();//��ȡ�������뻺�������ֽ���
	void beep(int msecond,int time);//������
	static UINT WINAPI ListenThread(void* pParam);
private:
    HANDLE m_hCom;
	static unsigned char _TKey[16][8]; 
	static unsigned char key[2][8];
	static unsigned char buffer[256];
	static bool s_bExit;//�߳��˳���־����
	//�߳̾��
	volatile HANDLE m_hListenThread;
	CRITICAL_SECTION m_csCommunicationSync;//�����������
};

#endif