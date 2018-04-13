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

	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo);	//只创建decompress结构体并绑定错误处理器，需要手动绑定文件源
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo);
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file);
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file);

	virtual void ExecuteEmbedingAlg();
	virtual void ExecuteExtractingAlg();
	virtual int CalPayLoad(string filePath);
	virtual int CalPayLoads(FileList* fileList);
	virtual ALGORITHM_NAME GetAlgName();

	//debug用
	void ShowMessage(int num);

};

