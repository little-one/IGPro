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
#define COMPOSITE_BLOCK_NUM 4		//�ֿ�Ŀ��
#define COMPOSITE_BLOCK_COUNTE 4	//�ֿ���ܿ���
#define COMPOSITE_MATRIX_K 4		//��������Kֵ
#define COMPOSITE_MATRIX_STRIDE 8		//������벽��
#define COMPOSITE_MATRIX_CONTENT 32		//�������Ƕ������λ��
#define COMPOSITE_TABLE_K 4		//������Ƕ��Kֵ
#define COMPOSITE_TABLE_STRIDE 8		//������Ƕ�벽��
#define COMPOSITE_TABLE_CONTENT 32		//������Ƕ������λ��
#define COMPOSITE_TABLE_LOSE_R (4*12)		//��������ʧֵ��¼
#endif // !COMPOSITE_ALG_HEAD


class HighCapabilityCompositeAlg :
	public virtual IJpegOpeAlg
{
private:
	vector<int> EffectiveBitsCountVec;		//����fileList�е�����vector��˳�򱣴���Ӧ�������Ч������
	short MatrixK;
	short TableK;
	short Table_NBit;
	short Table_DBit;
public:
	virtual void InitDecompressInfo(jpeg_decompress_struct* cinfo);	//ֻ����decompress�ṹ�岢�󶨴�����������Ҫ�ֶ����ļ�Դ
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
	int CalEffectiveBitsCount(FILE* file);		//�Զ�ȥ��ͷ��8λ�㷨����λ
	int GetEffectiveBitsCount(string filePath);		//��ȡfileList��ĳ��ý�Ŀ��޸ı���λ����
	void ShowMessage(CString str);			//����ר��
};

