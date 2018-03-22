#pragma once

#include <vector>
#include <string>

#ifndef HIDENFILEPROCESS
#define HIDENFILEPROCESS
#include "BinaryFileSolver.h"
#include "Encrypt_Decrpty.h"
#include "FolderMonitor.h"
#include "StreamConvert.h"
#endif // !HIDENFILEPROCESS

#ifndef FILENAMELIST
#define FILENAMELIST
typedef struct
{
	vector<string> CarrierImagePathList;		//载体图像可以不止有一张
	string HidenFilePath;		//隐藏文件路径
	vector<string> NewFilePathList;			//新生成的图像路径
}FileList;
#endif // !FILENAMELIST


#ifndef ALGORITHMNAME
#define ALGORITHMNAME
typedef enum :char
{
	ALG_UNKNOWN,
	ALG_DCTLSB
}ALGORITHM_NAME;
#endif // !ALGORITHMNAME


#ifndef EXTERNJPEGLIB
#define EXTERNJPEGLIB
extern "C"
{
#include "jpeglib.h" 
}
#endif // !EXTERNJPEGLIB

#ifndef ALGHEADPARAMETER
#define ALGHEADPARAMETER
#define ALGHEAD_TYPE_LENGTH 8	//所有载体的头部的前8bit位用来描述使用的加密算法，不用做加密载体
#define ALGHEAD_CONTENT_LENGTH 32	//载体的第9bit到第40位用来描述该载体内有多少位是被加密过的
#define ALGHEAD_BLOCK_NUM 8		//如果分快的话载体块的标号，从0开始
#endif // !ALGHEAD

#ifndef MEMORYSIZE
#define MEMORYSIZE
#define VIRT_MEMORY_SIZE 10000000
#endif // !MEMORYSIZE



class IJpegOpeAlg
{
private:
	FileList fileList;
public:
	IJpegOpeAlg();
	void SetFileList(FileList fl)
	{
		this->fileList = fl;
	}
	FileList GetFileList()
	{
		return this->fileList;
	}

	virtual ~IJpegOpeAlg();

	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo) = 0;	//只创建decompress结构体并绑定错误处理器，需要手动绑定文件源
	//virtual jpeg_decompress_struct* InitDecompressInfo(char* filePath) = 0;	//创建decompress并以默认的只读方式打开文件绑定文件源
	//virtual jpeg_decompress_struct* InitDecompressInfo(char* filePath, char* openMode) = 0;	//openMode代表打开文件的模式
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file) = 0;

	virtual void InitCompressInfo(jpeg_compress_struct* cinfo) = 0;
	//virtual jpeg_compress_struct* InitCompressInfo(char* filePath) = 0;
	//virtual jpeg_compress_struct* InitCompressInfo(char* filePath, char* openMode) = 0;
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file) = 0;

	virtual void ExecuteEmbedingAlg() = 0;		//此函数不作载荷检查，在调用之前就要做好载荷检查
	virtual void ExecuteExtractingAlg() = 0;
	virtual int CalPayLoad(FILE* file) = 0;		//计算单个载体的有效载荷
	virtual int CalPayLoads(FileList* fileList) = 0;		//计算多个载体的有效载荷
	virtual ALGORITHM_NAME GetAlgName() = 0;	//返回嵌入算法名称

	//void GetRGB(jpeg_decompress_struct* cinfo, char* R, int SizeR, char* G, int SizeG, char* B, int SizeB);

	//在此函数调用之前必须先调用InitDecompressInfo函数对decompress对象初始化
	virtual void GetImageInfo(jpeg_decompress_struct* cinfo, JDIMENSION* ImageWidth, JDIMENSION* ImageHeight, 
		int* ImageComponents, J_COLOR_SPACE* ImageColorSpace);	//图像的宽度，高度，通道数，色彩空间
	virtual void SetImageInfo(jpeg_compress_struct* cinfo, const JDIMENSION ImageWidth, const JDIMENSION ImageHeight, 
		const int ImageComponents, const J_COLOR_SPACE ImageColorSpace);	//此函数默认调用set_default参数去初始化其它参数

	virtual FILE* GetFile(char* filePath, char* fileMode);
	virtual void ClosFile(FILE* file);

	virtual jvirt_barray_ptr* ReadImageByDCTcoefficient(jpeg_decompress_struct* cinfo);	//通过读取量化后的DCT因子的方式来读取JPEG图片
	//只有在色彩空间为JCS_RGB时才会生效，否则直接返回，不改变RGB指针的内容
	virtual void ReadImageByRGB(jpeg_decompress_struct* cinfo, JSAMPLE** R, JSAMPLE** G, JSAMPLE** B, int* RGBLength);	//通过读取RGB方式读取JPEG图，RGBLength返回RGB数组的长度，RGB数组在传入之前不需要分配内存
	
	virtual void WriteImageByDCTcoefficient(jpeg_compress_struct* cinfo, jpeg_decompress_struct* tcinfo, jvirt_barray_ptr* coeff_array);
	//只有色彩空间为JCS_RGB时才会生效
	virtual void WriteImageByRGB(jpeg_compress_struct* cinfo, const JSAMPLE* R, const JSAMPLE* G, const JSAMPLE* B);
	
	virtual void FinishDecompress(jpeg_decompress_struct* cinfo);
	virtual void FinishCompress(jpeg_compress_struct* cinfo);
	
	virtual jvirt_barray_ptr* WriteAlgHead(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo, ALGORITHM_NAME AlgType);		//统一使用DCT因子的LSB算法嵌入前8位用来描述该载体使用的加密算法以供解密时选择正确算法解密
	virtual ALGORITHM_NAME ReadAlgHead(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo);

};

