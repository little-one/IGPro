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

class HighCapabilityCompositeAlg :
	public virtual IJpegOpeAlg
{
public:
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo);	//ֻ����decompress�ṹ�岢�󶨴�����������Ҫ�ֶ����ļ�Դ
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo);
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file);
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file);
public:
	HighCapabilityCompositeAlg();
	~HighCapabilityCompositeAlg();

	virtual void ExecuteEmbedingAlg();
	virtual void ExecuteExtractingAlg();
	virtual int CalPayLoad(FILE* file);
	virtual int CalPayLoads(FileList* fileList);
	virtual ALGORITHM_NAME GetAlgName();
};

