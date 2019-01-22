#include "Serial.h"
unsigned char Serial::_TKey[16][8] ={ { 0xbd, 0xde, 0x6f, 0x37, 0x83, 0x83, 0x00, 0x00 },
										   	{ 0x14, 0x8a, 0xc5, 0xe2, 0x28, 0x28, 0x00, 0x00 },
											{ 0x7d, 0x3e, 0x9f, 0x4f, 0x95, 0x95, 0x00, 0x00 },
											{ 0xad, 0xd6, 0x6b, 0x35, 0xc8, 0xc8, 0x00, 0x00 },
											{ 0xdf, 0xef, 0x77, 0xbb, 0xe4, 0xe4, 0x00, 0x00 },
											{ 0x09, 0x84, 0x42, 0x21, 0xbc, 0xbc, 0x00, 0x00 },
											{ 0x5f, 0xaf, 0xd7, 0xeb, 0xa5, 0xa5, 0x00, 0x00 },
											{ 0x29, 0x14, 0x8a, 0xc5, 0x9f, 0x9f, 0x00, 0x00 },
											{ 0xfa, 0xfd, 0xfe, 0x7f, 0xff, 0xff, 0x00, 0x00 },
											{ 0x73, 0x39, 0x9c, 0xce, 0xbe, 0xbe, 0x00, 0x00 },
											{ 0xfc, 0x7e, 0xbf, 0xdf, 0xbf, 0xbf, 0x00, 0x00 },
											{ 0xcf, 0xe7, 0x73, 0x39, 0x51, 0x51, 0x00, 0x00 },
											{ 0xf7, 0xfb, 0x7d, 0x3e, 0x5a, 0x5a, 0x00, 0x00 },
											{ 0xf2, 0x79, 0x3c, 0x1e, 0x8d, 0x8d, 0x00, 0x00 },
											{ 0xcf, 0xe7, 0x73, 0x39, 0x45, 0x45, 0x00, 0x00 },
											{ 0xb7, 0xdb, 0x6d, 0xb6, 0x7d, 0x7d, 0x00, 0x00 } };
unsigned char Serial::key[2][8] = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 },
								    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00 } };
unsigned char Serial::buffer[256] = { 0 };
bool Serial::s_bExit = true;
Serial::Serial()
{
	m_hCom = INVALID_HANDLE_VALUE;
	m_hListenThread = INVALID_HANDLE_VALUE;
	InitializeCriticalSection(&m_csCommunicationSync);
}

Serial::~Serial()
{
	ClosePort();
	CloseListenTread();
	DeleteCriticalSection(&m_csCommunicationSync);

}

bool Serial::OpenSeialPort(const TCHAR* port, UINT baud_rate , UINT databits , BYTE stop_bit, BYTE parity)
{
	m_hCom = CreateFile(port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_hCom == INVALID_HANDLE_VALUE) 
	{
		return false;
	}
	//���ô��ڶ�дʱ��,��Ϊ0����ʾ�����ó�ʱ����
	COMMTIMEOUTS CommTimeOuts;
	GetCommTimeouts(m_hCom, &CommTimeOuts);
	CommTimeOuts.ReadIntervalTimeout = MAXDWORD;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 0;

	if (!SetCommTimeouts(m_hCom, &CommTimeOuts)) {
		printf("SetCommTimeouts error\n");
		return false;
	}
	SetupComm(m_hCom, 1024, 1024);
	DCB dcb;
	if (!GetCommState(m_hCom, &dcb)) {
		printf("Get current Serial parameter error\n");
	}
	dcb.BaudRate = baud_rate;	//������
	dcb.fBinary = TRUE;			//������ģʽ������ΪTRUE
	dcb.ByteSize = databits;	//����λ����Χ4-8
	dcb.StopBits = stop_bit;	//һλֹͣλ

	if (parity == NOPARITY)
	{
		dcb.fParity = FALSE;	//��żУ�顣����żУ��
		dcb.Parity = parity;	//У��ģʽ������żУ��
	}
	else 
	{
		dcb.fParity = TRUE;		//��żУ�顣
		dcb.Parity = parity;	//У��ģʽ������żУ��
	}
	if (!SetCommState(m_hCom, &dcb)) {
		printf("SetCommState error\n");
		return false;
	}
	PurgeComm(m_hCom, PURGE_RXCLEAR | PURGE_TXCLEAR);//��մ��ڻ�����
	return true;
}

void Serial::ClosePort()
{
	if (m_hCom != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hCom);
		m_hCom = INVALID_HANDLE_VALUE;
	}
}

bool Serial::OpenListenThread()
{
	if (m_hListenThread != INVALID_HANDLE_VALUE)//�߳��ѿ���
		return false;
	s_bExit = false;
	UINT threadId;
	m_hListenThread = (HANDLE)_beginthreadex(NULL, 0, ListenThread, this, 0, &threadId);
	if (!m_hListenThread)
	{
		printf("openListenThread error\n");
		return false;
	}
	//�����߳����ȼ���������ͨ�߳�
	if (!SetThreadPriority(m_hListenThread, THREAD_PRIORITY_ABOVE_NORMAL))
	{
		return false;
	}

	return true;

}
bool Serial::CloseListenTread()
{
	if (m_hListenThread != INVALID_HANDLE_VALUE)
	{
		s_bExit = true;
		Sleep(10);
		CloseHandle(m_hListenThread);
		m_hListenThread = INVALID_HANDLE_VALUE;
	}
	return true;
}

bool Serial::WriteData(unsigned char* pData, unsigned int length) 
{
	DWORD BytesToSend;//ʵ��д�����ݵĳ���
	BOOL bResult ;
	if (m_hCom == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	EnterCriticalSection(&m_csCommunicationSync);//�ٽ�������
	bResult = WriteFile(m_hCom, pData, length, &BytesToSend, NULL);//д����
	if (!bResult)
	{
		PurgeComm(m_hCom, PURGE_TXCLEAR | PURGE_RXCLEAR);
		LeaveCriticalSection(&m_csCommunicationSync);//�뿪�ٽ�����
		return false;
	}
	LeaveCriticalSection(&m_csCommunicationSync);
	return BytesToSend==length;
}
int Serial::ReadData(unsigned char* data)//, DWORD length, UINT BytesInCom)
{
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;
	unsigned char databuf[256];
	unsigned int bcc = 0;
	int i = 0;
	int index;
	DWORD nread=0; 

	int status;
	BOOL  bResult = false;

	while (1)
	{
		UINT BytesInQue = GetBytesInCom();
		if (BytesInQue == 0)
		{
			Sleep(50);
			continue;
		}
		bResult = ReadFile(m_hCom, databuf, BytesInQue, &nread, NULL);
		if (bResult)
		{
			if (nread>0)//���յ�����
			{
				if (databuf[0] != 0x02)//ͨѶ��ʼλSTX
				{
					return ERR_COMMUNICATION;
				}
					
				if (databuf[1] != 0x10)//�ж��������SeqNr�Ƿ�Ϊ0x10
					status = databuf[2];
				else
					status = databuf[3];
				if (status != 0)//status�������ý��
					return status;
				index = 1;
				for (i = 1; i<4; i++)//�ҵ�������ʼ��λ��
				{
					if (databuf[i] == 0x10)
						index += 2;
					else
						index++;
				}
				int datalen = databuf[index - 1];//���ݳ���
				for (i = 0; i<datalen; i++)
				{
					data[i] = databuf[index];
					if (databuf[index] == 0x10)//0X10���������Σ������DLE(10H)�źŻ���
						index += 2;
					else
						index++;
				}
				bcc = bcc^databuf[1] ^ databuf[2] ^ databuf[3];
				for (i = 0; i<datalen; i++)
					bcc ^= data[i];//bcc���У����
				if (bcc != databuf[index])//bcc���У���벻��
					return ERR_READ;
			/*	for (i = 0; i<nread; i++)
					printf("%x ", databuf[i]);
				printf("\n");
				for (i = 0; i<datalen; i++)
					printf("%x ", data[i]);
				printf("\n");*/
				break;
			}
		}	
	
	}
	return 0;
}
int Serial::ReadDataTh(unsigned char* data, UINT bytesInCom, DWORD& length)
{
	BOOL bResult;
	EnterCriticalSection(&m_csCommunicationSync);
	bResult = ReadFile(m_hCom, buffer, bytesInCom, &length, NULL);
	if (!bResult)
	{
		printf("readfile error\n");
		PurgeComm(m_hCom, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);
		return -1;
	}
	LeaveCriticalSection(&m_csCommunicationSync);
	return 0;
}
bool Serial::send_cmd(unsigned char cmd, unsigned char len, unsigned char *data)
{
	if (m_hCom == INVALID_HANDLE_VALUE) {
		printf("don't open the serial");
		return false;
	}
	unsigned char info[256], bcc;
	int i, j, index;
	bcc = 0;
	info[0] = 0x02;//��ʼλ
	info[1] = 0x00;//������ţ�ÿ���һ��ͨѶ�����һ
	info[2] = cmd;//command
	info[3] = len;//len
	if (len == 0x10)
	{
		info[4] = 0x10;
		index = 5;
	}
	else
		index = 4;
	for (i = 0; i<len; i++)
	{
		info[index] = *(data + i);
		if (info[index] == 0x10)//0X10���������Σ������DLE(10H)�źŻ���
			info[++index] = 0x10;
		index++;
	}
	bcc = bcc^info[1] ^ info[2] ^ info[3];
	for (i = 0; i<len; i++)
	{
		bcc ^= data[i];
	}
	info[index] = bcc;//���У���� 
	info[++index] = 0x10;//DLE�����߾���
	info[++index] = 0x03;//ETXͨѶ����
	bool flag=WriteData(info, index+1);
	if (flag)
		return true;
	else
		return false;
}
int Serial::check_card(unsigned char _Mode, unsigned long* snr)
{
	unsigned char senddata[64];
	unsigned char buffer[64];
	senddata[0] = _Mode;
	bool send_flag;
	send_flag = send_cmd(0x60, 1, senddata);
	if (!send_flag)
	{
		printf("check card sendcmd error\n");
		return send_flag;
	}
	Sleep(rsspeed);
	int receive_falg = ReadData(buffer);
	*snr = (buffer[3] << 8 | buffer[2]) << 16 | (buffer[1] << 8 | buffer[0]);
	return receive_falg;
	return 0;
}
int Serial::load_key(unsigned char mode, unsigned char sector, unsigned char* bufkey)
{
	unsigned char senddata[64];
	unsigned char buffer[64];
	int i;
	senddata[0] = mode << 2;//mode=0��������A,1��������B
	senddata[1] = sector;
	for (i = 0; i<8; i++)
		senddata[2 + i] = _TKey[sector][i];
	for (i = 0; i < 8; i++)
		senddata[10 + i] = bufkey[i];
	bool send_flag=send_cmd(0x4C, 18, senddata);
	if (!send_flag)
	{
		printf("load_key sendcmd error\n");
		return send_flag;
	}
	Sleep(rsspeed);
	int receive_falg = ReadData(buffer);
	return receive_falg;
}
int Serial::auth_key(unsigned char mode, unsigned char sector)
{
	unsigned char senddata[64];
	unsigned char buffer[64];
	senddata[0] = mode << 2;
	senddata[1] = sector;
	bool send_flag=send_cmd(0x44, 2, senddata);
	if (!send_flag)
	{
		printf("auth_key sendcmd error\n");
		return send_flag;
	}
	Sleep(rsspeed);
	int receive_falg = ReadData(buffer);
	return receive_falg;
}
int Serial::read_block(unsigned char address, unsigned char* databuf)
{
	unsigned char senddata[64];
	senddata[0] = address;
	bool send_flag=send_cmd(0x46, 1, senddata);
	if (!send_flag)
	{
		printf("read_card sendcmd error\n");
		return send_flag;
	}
	Sleep(rsspeed);
	int receive_falg = ReadData(databuf);
	return receive_falg;
}
int Serial::write_block(unsigned char address, unsigned char* databuf)
{
	unsigned char senddata[64];
	unsigned char buffer[64];
	senddata[0] = address;
	memcpy(&senddata[1], databuf, 16);
	bool send_flag=send_cmd(0x47, 17, senddata);
	if (!send_flag)
	{
		printf("write_card sendcmd error\n");
		return send_flag;
	}
	Sleep(rsspeed);
	int receive_falg = ReadData(buffer);
	return receive_falg;
}
int Serial::write_card(unsigned char data[][16])
{
	unsigned long Snr;
	int ret;
	ret = check_card(0, &Snr);//ѡ��
	if (ret != 0)
	{
		printf("�޿�\n");
		return ret;
	}
	//����Ƶģ���RAM��װ������
	ret = load_key(0, 0, key[0]);
	if (ret != 0)
	{
		printf("load_key error\n");
		return ret;
	}
	ret = auth_key(0, 0);
	if (ret != 0)
	{
		printf("auth error\n");
		return ret;
	}
	//������0д����
	ret = write_block(1, data[0]);
	if (ret != 0)
	{
		printf("write_block error\n");
		return ret;
	}
	ret = write_block(2, data[1]);
	if (ret != 0)
	{
		printf("write_block error\n");
		return ret;
	}
	//��ʼ������1д����
	ret = load_key(0, 1, key[0]);//����Ƶģ��RAM��װ������
	if (ret != 0)
	{
		printf("load_key error\n");
		return ret;
	}
	ret = auth_key(0, 1);
	if (ret != 0)
	{
		printf("auth error\n");
		return ret;
	}
	//������1д����
	ret = write_block(4, data[2]);
	if (ret != 0)
	{
		printf("write_block error\n");
		return ret;
	}
	ret = write_block(5, data[3]);
	if (ret != 0)
	{
		printf("write_block error\n");
		return ret;
	}
	beep(10,2);
	return 0;
}
int Serial::read_card(unsigned char buffer[][16])
{
	unsigned long Snr;
	int ret;
	ret = check_card(0, &Snr);//ѡ��
	if (ret != 0)
	{
		printf("�޿�\n");
		return ret;
	}
	//������0��д������
	ret = load_key(0, 0, key[0]);//����Ƶģ��RAM��װ������
	if (ret != 0)
	{
		printf("load_key error\n");
		return ret;
	}
	ret = auth_key(0, 0);
	if (ret != 0)
	{
		printf("auth error\n");
		return ret;
	}
	//������0������
	ret = read_block(1, buffer[0]);
	if (ret != 0)
	{
		printf("read error\n");
		return ret;
	}
	ret = read_block(2, buffer[1]);
	if (ret != 0)
	{
		printf("read error\n");
		return ret;
	}
	//����1
	ret = load_key(0, 1, key[0]);//����Ƶģ��RAM��װ������
	if (ret != 0)
	{
		printf("load_key error\n");
		return ret;
	}
	ret = auth_key(0, 1);
	if (ret != 0)
	{
		printf("auth error\n");
		return ret;
	}

	//������1����
	ret = read_block(4, buffer[2]);
	if (ret != 0)
	{
		printf("read_block error\n");
		return ret;
	}
	ret = read_block(5, buffer[3]);
	if (ret != 0)
	{
		printf("read_block error\n");
		return ret;
	}
	beep(10,2);
	return 0;
}

bool Serial::ReadChar(char& rChar)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hCom == INVALID_HANDLE_VALUE)
		return false;
	EnterCriticalSection(&m_csCommunicationSync);
	bResult = ReadFile(m_hCom, &rChar, 1, &BytesRead, NULL);
	if (!bResult)
	{
		PurgeComm(m_hCom, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}
	return BytesRead == 1;
}
UINT Serial::GetBytesInCom()
{
	DWORD dwError = 0;  //������ 
	COMSTAT  comstat;   //COMSTAT�ṹ��,��¼ͨ���豸��״̬��Ϣ 
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	//�ڵ���ReadFile��WriteFile֮ǰ,ͨ�������������ǰ�����Ĵ����־ 
	if (ClearCommError(m_hCom, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; // ��ȡ�����뻺�����е��ֽ��� 
	}

	return BytesInQue;

}
void Serial::beep(int msecond,int time)
{
	unsigned char data[64];
	data[0] = msecond;
	for (int i = 0; i < time; i++)
	{
		send_cmd(0x36, 2, data);
		Sleep(500);
	}
		
}

UINT WINAPI Serial::ListenThread(void* pParam)
{
	//char rChar;
	Serial *pSerial = reinterpret_cast<Serial*>(pParam);
	DWORD length=0;//ʵ�ʶ������ַ�����
	int result;
	while (!pSerial->s_bExit)
	{
		UINT BytesInCom = pSerial->GetBytesInCom();
		if (BytesInCom == 0)
		{
			Sleep(50);
			continue;
		}
		result=pSerial->ReadDataTh(buffer, BytesInCom, length);
		if (!result)
			return result;
	}
	return 0;
}