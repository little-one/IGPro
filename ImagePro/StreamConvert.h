#pragma once
#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <map>
#include <stdint.h>

using namespace std;

class StreamConvert
{
private:
	int K;				    //ϵ��K���൱����д�������Ľ�����
	int BinaryBit = -1;			//������ÿBinaryBit��Ϊһ��תΪN����
	int N_DecimalBit = -1;		//ÿ���ɶ�����ת������N��������λ��
	map<int, pair<int, int> > BitRepresentationMap;				//����ת��������λ���Ķ�Ӧ��ϵ, map��������Ӧ�����ִ����Ƕ�������N���ƵĻ�ת����ֵ��������pair.firstλ�Ķ�������ת����pair.secondλ��N����
private:
	void InitializeBitMap();
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
};
