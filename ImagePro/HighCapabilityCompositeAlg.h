#pragma once

#include <vector>

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

#ifndef COMPOSITE_ALG_HEAD
#define COMPOSITE_ALG_HEAD
#define COMPOSITE_BLOCK_NUM 4		//分块的块号
#define COMPOSITE_BLOCK_COUNTE 4	//分块的总块数
#define COMPOSITE_MATRIX_K 4		//矩阵编码的K值
#define COMPOSITE_MATRIX_STRIDE 8		//矩阵编码步长
#define COMPOSITE_MATRIX_CONTENT 32		//矩阵编码嵌入密文位数
#define COMPOSITE_TABLE_K 4		//量化表嵌入K值
#define COMPOSITE_TABLE_STRIDE 8		//量化表嵌入步长
#define COMPOSITE_TABLE_CONTENT 32		//量化表嵌入密文位数
#define COMPOSITE_TABLE_LOSE_R (4*12)		//量化表损失值记录
#endif // !COMPOSITE_ALG_HEAD


class HighCapabilityCompositeAlg :
	public virtual IJpegOpeAlg
{
private:
	vector<int> EffectiveBitsCountVec;		//按照fileList中的载体vector的顺序保存相应载体的有效比特数
	short MatrixK;
	short TableK;
	short Table_NBit;
	short Table_DBit;
public:
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo);	//只创建decompress结构体并绑定错误处理器，需要手动绑定文件源
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo);
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file);
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file);
public:
	HighCapabilityCompositeAlg();
	HighCapabilityCompositeAlg(FileList* fileList, int MatrixK, int TableK);
	~HighCapabilityCompositeAlg();

	virtual void ExecuteEmbedingAlg();
	virtual void ExecuteExtractingAlg();
	virtual int CalPayLoad(string filePath);
	virtual int CalPayLoads(FileList* fileList);
	virtual ALGORITHM_NAME GetAlgName();
protected:
	int MatrixCalPayLoad(string filePath, int K);
	int TableCalEffectiveBitsCount(string filePath, int K);
	jvirt_barray_ptr* MatrixCodeEmbed(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo);
	jvirt_barray_ptr* TableEmbed(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo);
	int CalEffectiveBitsCount(FILE* file);		//自动去除头部8位算法描述位
	int GetEffectiveBitsCount(string filePath);		//获取fileList中某载媒的可修改比特位总数

	void ShowMessage(CString str);			//调试专用
	void ShowMessage(vector<int> intList);
	void ShowMessage(int num);
	CString CharToCString(char* str, int length);
	CString ShortToCString(short* str, int length);
	CString UCharToCString(unsigned char* str, int length);

	inline bool GetSecretBitGroup(char** SecretStream, int bitCount, const char const * fileStream, int fileBitCount, int position);
	inline int SetSecretBitGroup(const char* const SecretStream, int bitCount, char * fileStream, int fileBitCount, int position);
	inline void GetZOStreamGroup(char** ZOStream, const JCOEFPTR const * SrcDataGroup, int PGroupPayload);
	inline int MCoding(const char* const SecretStream, const char* const ZOStream, int K);		//获取需要修改的位的位置，(已经自动减一，直接用到数据下标即可，返回-1表示不需要修改任何位)
	inline void MDeCoding(char** SecreStream, const char* const ZOStream, int K);
};

