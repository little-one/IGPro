#pragma once
#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <stdint.h>

using namespace std;

class Encrypt_Decrpty
{
private:
	ifstream ifs;
	ofstream ofs;
	string outFilePath;
	string currentDir;
public:
	int byteStream2BinaryString(uint8_t byteBuf[], int byteLen, char charBuf[], int charLen, int type);
	int Encrypt(int byteLen);		//char����ת������
	int Decrypt();		//������תchar����
	
	Encrypt_Decrpty();
	~Encrypt_Decrpty();
};

