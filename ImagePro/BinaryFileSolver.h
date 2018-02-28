#pragma once
#include <string>
#include <vector>
using namespace std;
class BinaryFileSolver
{
private:
	string inFilePath = "";
	string outFilePath = "";
	int inFileByteCount = 0;
	int outFileByteCount = 0;
	uint8_t* BinaryFileContent = nullptr;
public:
	int GetInFileByteCount();
	int GetOutFileByteCount();
	uint8_t* GetBinaryFileContent();
	void setInFilePath(string filePath);
	void setOutFilePath(string filePath);
	void ClearBeforeRead();
	BinaryFileSolver();
	~BinaryFileSolver();
	int LoadFile();
	int AppendFile(uint8_t Content[], int ContentByte);
};

