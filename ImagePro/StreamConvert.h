#pragma once
#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <map>
#include "MainFrm.h"
#include <stdint.h>

#include <vector>
#ifndef BITCOUNT
#define BITCOUNT
#define BITCOUNT_INT (8*sizeof(int))
#define BITCOUNT_HALFCHAR 4
#endif // !BITCOUNT_INT


using namespace std;

class StreamConvert
{
private:
	int K;				    //ϵ��K���൱����д�������Ľ�����
	int BinaryBit = -1;			//������ÿBinaryBit��Ϊһ��תΪN����
	int N_DecimalBit = -1;		//ÿ���ɶ�����ת������N��������λ��
public:
	map<int, pair<int, int> > BitRepresentationMap;				//����ת��������λ���Ķ�Ӧ��ϵ, map��������Ӧ�����ִ����Ƕ�������N���ƵĻ�ת����ֵ��������pair.firstλ�Ķ�������ת����pair.secondλ��N����
private:
	inline void InitializeBitMap();
	pair<int, int> GetBitRepresentation(int N);		//��ȡ����ת��ʱ��Ӧ��λ��
public:
	int byteStreamToBinaryString(uint8_t byteBuf[], int byteLen, char charBuf[], int charLen, int type);
	int bitConvert_BinaryToNDecimal(char Binary[], int BinaryBitCount, char NDecimal[], int NDecimalBitCount, int type);
	int Cal_BinaryBitCount(int NDecimalBitCount);		//����N��������λ�����ת����Ķ�����λ��(�޳�����λ����,���Աض���8�ı���)
	int Cal_NDecimalBitCount(int BinaryBitCount);		//���ݶ����Ƶ�λ�����ת�����N����λ��
public:
	StreamConvert();
	StreamConvert(int N);
	~StreamConvert();
public:
	//����ר��
	void ShowMessage(CString str);			
	void ShowMessage(vector<int> intList);

	void Convert_IntToBinaryStream(int num, char** BinaryStream);			
	int Convert_BinaryStreamToInt(char* BStream);		//�����BStream�Ĵ�С������BITCOUNT_INT+1��

	void Convert_CharToBinaryStream(char num, char** BinaryStream);			//��һ��char�ַ�תΪ�������ַ���
	char Convert_BinaryStreamToChar(char* BStream);

	void Convert_HalfCharToBinaryStream(char num, char** BinaryStream);		//��numתΪ4bit���ȵ��ַ�������
	char Convert_BinaryStreamToHalfChar(char* BStream);		//��4bit���ȵ��ַ�������תΪchar�� �����BStream����Ϊ����Ϊ4�ģ��������4���ȡǰ4λ

	int Convert_CharToInt(char cnum);		//
	int Convert_KStreamToInt(const char* BStream, int K);		//��Kλ������תΪint��K����С��BStream����ĳ���
};

