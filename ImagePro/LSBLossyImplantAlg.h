#pragma once

#ifndef MYINTERFACE
#define MYINTERFACE
#include "IJpegOpeAlg.h"
#endif // !MYINTERFACE

#ifndef HIDENFILEPROCESS
#define HIDENFILEPROCESS
#include "BinaryFileSolver.h"
#include "Encrypt_Decrpty.h"
#include "FolderMonitor.h"
#include "StreamConvert.h"
#endif // !HIDENFILEPROCESS

class LSBLossyImplantAlg :
	public virtual IJpegOpeAlg
{
public:
	LSBLossyImplantAlg();
	LSBLossyImplantAlg(FileList* fileList);
	~LSBLossyImplantAlg();
	//void test(jpeg_decompress_struct* cinfo, FILE* file);

	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo);	//ֻ����decompress�ṹ�岢�󶨴�����������Ҫ�ֶ����ļ�Դ
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo);
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file);
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file);

	virtual void ExecuteEmbedingAlg();
	virtual void ExecuteExtractingAlg();
	virtual int CalPayLoad(string filePath);
	virtual int CalPayLoads(FileList* fileList);
	virtual ALGORITHM_NAME GetAlgName();

	//debug��
	void ShowMessage(int num);

};

