#pragma once

#ifndef MYINTERFACE
#include "IJpegOpeAlg.h"
#endif // !MYINTERFACE
#ifndef HIDENFILEPROCESS
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

	virtual jpeg_decompress_struct* InitDecompressInfo();	//ֻ����decompress�ṹ�岢�󶨴�����������Ҫ�ֶ����ļ�Դ

	virtual jpeg_compress_struct* InitCompressInfo();

	virtual jpeg_decompress_struct* InitDecompressInfo(FILE* file);

	virtual jpeg_compress_struct* InitCompressInfo(FILE* file);

	virtual void ExecuteEmbedingAlg();
	virtual void ExecuteExtractingAlg();
	virtual int CalPayLoad(FILE* file);
	virtual int CalPayLoads(FileList* fileList);
	virtual ALGORITHM_NAME GetAlgName();

};

