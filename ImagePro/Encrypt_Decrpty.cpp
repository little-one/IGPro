#include "stdafx.h"
#include "Encrypt_Decrpty.h"


int Encrypt_Decrpty::byteStream2BinaryString(uint8_t byteBuf[], int byteLen, char charBuf[], int charLen, int type)
{
	switch (type)
	{
	case 0:
		if (charLen < 8 * byteLen)
			return -1;
		else
		{
			int charBufIndex = 0;
			for (int i = 0; i < byteLen; i++)
				for (int j = 7; j >= 0; j--)
				{
					if (byteBuf[i] >> j & 0x1)
						charBuf[charBufIndex++] = '1';
					else
						charBuf[charBufIndex++] = '0';
				}
		}
		break;
	case 1:
		memset(byteBuf, 0, byteLen);
		if (8 * byteLen < charLen)
			return -1;
		char tmpStr[9] = "";
		int byteBufIndex = 0;
		for (int i = 0; i < charLen; i += 8)
		{
			strncpy_s(tmpStr, charBuf + i, 8);
			for (int j = 0; j < 8; j++)
				byteBuf[byteBufIndex] += tmpStr[j] - '0' << 7 - j;
			++byteBufIndex;
		}
		break;
	}
	return 0;
}

int Encrypt_Decrpty::Encrypt(int byteLen)		//byteBuf中存着字符串
{
	uint8_t* byteBuf = new uint8_t[byteLen * 8];
	char* charBuf = new char[byteLen * 8];
	ifs.seekg(0, ios::beg);
	ifs.read((char*)byteBuf, byteLen);
	int ret = byteStream2BinaryString(byteBuf, byteLen, charBuf, byteLen * 8, 0);
	if (ret != 0)
		return -1; 
	ofs.write(charBuf, byteLen * 8);
	delete[] byteBuf;
	delete[] charBuf;
	return 0;
}

int Encrypt_Decrpty::Decrypt()		//二进制转char数组
{
	ifs.seekg(0, ios::end);
	int charLen = ifs.tellg();
	char* charBuf = new char[charLen];
	int byteLen = charLen % 8 == 0 ? charLen / 8 : charLen / 8 + 1;
	uint8_t* byteBuf = new uint8_t[byteLen];
	ifs.seekg(0, ios::beg);
	ifs.read(charBuf, charLen);
	int ret = byteStream2BinaryString(byteBuf, byteLen, charBuf, charLen, 1);
	if (ret != 0)
		return -1;
	ofs.write((char*)byteBuf, byteLen);
	delete[] byteBuf;
	delete[] charBuf;
	return 0;
}

Encrypt_Decrpty::Encrypt_Decrpty()
{
}


Encrypt_Decrpty::~Encrypt_Decrpty()
{
}
