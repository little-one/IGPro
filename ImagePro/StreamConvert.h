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
	int K;				    //系数K，相当于隐写数据流的进制数
	int BinaryBit = -1;			//二进制每BinaryBit化为一组转为N进制
	int N_DecimalBit = -1;		//每组由二进制转过来的N进制数的位数
public:
	map<int, pair<int, int> > BitRepresentationMap;				//进制转换过程中位数的对应关系, map的索引对应的数字代表是二进制与N进制的互转，键值的意义是pair.first位的二进制数转换成pair.second位的N进制
private:
	inline void InitializeBitMap();
	pair<int, int> GetBitRepresentation(int N);		//获取进制转换时相应的位数
public:
	int byteStreamToBinaryString(uint8_t byteBuf[], int byteLen, char charBuf[], int charLen, int type);
	int bitConvert_BinaryToNDecimal(char Binary[], int BinaryBitCount, char NDecimal[], int NDecimalBitCount, int type);
	int Cal_BinaryBitCount(int NDecimalBitCount);		//根据N进制数的位数求得转换后的二进制位数(剔除补零位数后,所以必定是8的倍数)
	int Cal_NDecimalBitCount(int BinaryBitCount);		//根据二进制的位数求得转换后的N进制位数
public:
	StreamConvert();
	StreamConvert(int N);
	~StreamConvert();
public:
	//调试专用
	void ShowMessage(CString str);			
	void ShowMessage(vector<int> intList);

	void Convert_IntToBinaryStream(int num, char** BinaryStream);			
	int Convert_BinaryStreamToInt(char* BStream);		//传入的BStream的大小必须是BITCOUNT_INT+1的

	void Convert_CharToBinaryStream(char num, char** BinaryStream);			//将一个char字符转为二进制字符流
	char Convert_BinaryStreamToChar(char* BStream);

	void Convert_HalfCharToBinaryStream(char num, char** BinaryStream);		//将num转为4bit长度的字符数组流
	char Convert_BinaryStreamToHalfChar(char* BStream);		//将4bit长度的字符数组流转为char， 传入的BStream必须为长度为4的，如果大于4则截取前4位

	int Convert_CharToInt(char cnum);		//
	int Convert_KStreamToInt(const char* BStream, int K);		//将K位比特流转为int，K必须小于BStream数组的长度
};

