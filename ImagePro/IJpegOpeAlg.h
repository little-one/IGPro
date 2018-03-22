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
	vector<string> CarrierImagePathList;		//����ͼ����Բ�ֹ��һ��
	string HidenFilePath;		//�����ļ�·��
	vector<string> NewFilePathList;			//�����ɵ�ͼ��·��
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
#define ALGHEAD_TYPE_LENGTH 8	//���������ͷ����ǰ8bitλ��������ʹ�õļ����㷨����������������
#define ALGHEAD_CONTENT_LENGTH 32	//����ĵ�9bit����40λ�����������������ж���λ�Ǳ����ܹ���
#define ALGHEAD_BLOCK_NUM 8		//����ֿ�Ļ������ı�ţ���0��ʼ
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

	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo) = 0;	//ֻ����decompress�ṹ�岢�󶨴�����������Ҫ�ֶ����ļ�Դ
	//virtual jpeg_decompress_struct* InitDecompressInfo(char* filePath) = 0;	//����decompress����Ĭ�ϵ�ֻ����ʽ���ļ����ļ�Դ
	//virtual jpeg_decompress_struct* InitDecompressInfo(char* filePath, char* openMode) = 0;	//openMode������ļ���ģʽ
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo, FILE* file) = 0;

	virtual void InitCompressInfo(jpeg_compress_struct* cinfo) = 0;
	//virtual jpeg_compress_struct* InitCompressInfo(char* filePath) = 0;
	//virtual jpeg_compress_struct* InitCompressInfo(char* filePath, char* openMode) = 0;
	virtual void InitCompressInfo(jpeg_compress_struct* cinfo, FILE* file) = 0;

	virtual void ExecuteEmbedingAlg() = 0;		//�˺��������غɼ�飬�ڵ���֮ǰ��Ҫ�����غɼ��
	virtual void ExecuteExtractingAlg() = 0;
	virtual int CalPayLoad(FILE* file) = 0;		//���㵥���������Ч�غ�
	virtual int CalPayLoads(FileList* fileList) = 0;		//�������������Ч�غ�
	virtual ALGORITHM_NAME GetAlgName() = 0;	//����Ƕ���㷨����

	//void GetRGB(jpeg_decompress_struct* cinfo, char* R, int SizeR, char* G, int SizeG, char* B, int SizeB);

	//�ڴ˺�������֮ǰ�����ȵ���InitDecompressInfo������decompress�����ʼ��
	virtual void GetImageInfo(jpeg_decompress_struct* cinfo, JDIMENSION* ImageWidth, JDIMENSION* ImageHeight, 
		int* ImageComponents, J_COLOR_SPACE* ImageColorSpace);	//ͼ��Ŀ�ȣ��߶ȣ�ͨ������ɫ�ʿռ�
	virtual void SetImageInfo(jpeg_compress_struct* cinfo, const JDIMENSION ImageWidth, const JDIMENSION ImageHeight, 
		const int ImageComponents, const J_COLOR_SPACE ImageColorSpace);	//�˺���Ĭ�ϵ���set_default����ȥ��ʼ����������

	virtual FILE* GetFile(char* filePath, char* fileMode);
	virtual void ClosFile(FILE* file);

	virtual jvirt_barray_ptr* ReadImageByDCTcoefficient(jpeg_decompress_struct* cinfo);	//ͨ����ȡ�������DCT���ӵķ�ʽ����ȡJPEGͼƬ
	//ֻ����ɫ�ʿռ�ΪJCS_RGBʱ�Ż���Ч������ֱ�ӷ��أ����ı�RGBָ�������
	virtual void ReadImageByRGB(jpeg_decompress_struct* cinfo, JSAMPLE** R, JSAMPLE** G, JSAMPLE** B, int* RGBLength);	//ͨ����ȡRGB��ʽ��ȡJPEGͼ��RGBLength����RGB����ĳ��ȣ�RGB�����ڴ���֮ǰ����Ҫ�����ڴ�
	
	virtual void WriteImageByDCTcoefficient(jpeg_compress_struct* cinfo, jpeg_decompress_struct* tcinfo, jvirt_barray_ptr* coeff_array);
	//ֻ��ɫ�ʿռ�ΪJCS_RGBʱ�Ż���Ч
	virtual void WriteImageByRGB(jpeg_compress_struct* cinfo, const JSAMPLE* R, const JSAMPLE* G, const JSAMPLE* B);
	
	virtual void FinishDecompress(jpeg_decompress_struct* cinfo);
	virtual void FinishCompress(jpeg_compress_struct* cinfo);
	
	virtual jvirt_barray_ptr* WriteAlgHead(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo, ALGORITHM_NAME AlgType);		//ͳһʹ��DCT���ӵ�LSB�㷨Ƕ��ǰ8λ��������������ʹ�õļ����㷨�Թ�����ʱѡ����ȷ�㷨����
	virtual ALGORITHM_NAME ReadAlgHead(jvirt_barray_ptr* coeff_arrays, jpeg_decompress_struct* cinfo);

};

