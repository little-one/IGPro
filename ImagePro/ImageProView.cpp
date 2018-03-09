
// ImageProView.cpp : CImageProView 类的实现
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
using namespace std;
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "ImagePro.h"
#endif

#include "ImageProDoc.h"
#include "ImageProView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <setjmp.h>
#include "BinaryFileSolver.h"
#include "Encrypt_Decrpty.h"
#include "FolderMonitor.h"
#include "StreamConvert.h"

extern "C" 
{ 
#include "jpeglib.h" 
}

//#pragma comment(lib,"./libjpeg.lib")
// CImageProView

IMPLEMENT_DYNCREATE(CImageProView, CFormView)

BEGIN_MESSAGE_MAP(CImageProView, CFormView)
	ON_BN_CLICKED(IDC_BUTTON1, &CImageProView::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CImageProView::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CImageProView::OnBnClickedButton3)
END_MESSAGE_MAP()

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};


// CImageProView 构造/析构

CImageProView::CImageProView()
	: CFormView(IDD_IMAGEPRO_FORM)
{
	// TODO: 在此处添加构造代码

}

CImageProView::~CImageProView()
{
}

void CImageProView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BOOL CImageProView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CFormView::PreCreateWindow(cs);
}

void CImageProView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

}


// CImageProView 诊断

#ifdef _DEBUG
void CImageProView::AssertValid() const
{
	CFormView::AssertValid();
}

void CImageProView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CImageProDoc* CImageProView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImageProDoc)));
	return (CImageProDoc*)m_pDocument;
}
#endif //_DEBUG


// CImageProView 消息处理程序

void CImageProView::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	//struct jpeg_decompress_struct cinfo;
	//struct my_error_mgr jerr;
	//FILE* infile;
	//JSAMPARRAY buffer;
	//int row_stride;
	//if ((fopen_s(&infile,"F:\\test\\t1.jpg", "rb")) == NULL)
	//{
	//	return;
	//}
	struct jpeg_compress_struct coutfo;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr injerr;
	struct jpeg_error_mgr outjerr;
	cinfo.err = jpeg_std_error(&injerr);
	coutfo.err = jpeg_std_error(&outjerr);
	jpeg_create_compress(&coutfo);
	jpeg_create_decompress(&cinfo);
	FILE* infile;
	FILE* outfile;
	fopen_s(&infile, "F:\\test\\t1.jpg", "rb");
	fopen_s(&outfile, "F:\\test\\t2.jpg", "wb");
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_stdio_dest(&coutfo, outfile);
	coutfo.image_width = cinfo.image_width;
	coutfo.image_height = cinfo.image_height;
	coutfo.input_components = cinfo.num_components;
	coutfo.in_color_space = cinfo.out_color_space;
	jpeg_set_defaults(&coutfo);
	
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&cinfo);

	
	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = cinfo.comp_info + ci;
		
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					blockptr[bi] = 3;
				}
			}
		}
	}

	jpeg_copy_critical_parameters(&cinfo, &coutfo);
	jpeg_write_coefficients(&coutfo, coeff_arrays);
	//int count = 0;
	//while (buffer)
	//{
	//	JBLOCKARRAY jbarray = (*buffer)->mem_buffer;
	//	JBLOCKROW jbrow = NULL;
	//	for (int i = 0; i < (*buffer)->rows_in_mem; i++)
	//	{
	//		jbrow = *jbarray;
	//		for (int j = 0; j < (*buffer)->blocksperrow; j++)
	//		{
	//			for (int k = 1; k < 64; k++)
	//			{
	//				count++;
	//			}
	//		}
	//	}
	//}

	//jpeg_start_decompress(&cinfo);
	//jpeg_start_compress(&coutfo, TRUE);

	//int row_stride = cinfo.output_width * cinfo.output_components;
	//JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	//JSAMPROW row_pointer[1];
	//int count = 0;
	//while (cinfo.output_scanline < cinfo.output_height)
	//{
	//	jpeg_read_scanlines(&cinfo, buffer, 1);
	//	row_pointer[0] = buffer[0];
	//	if (count < 50)
	//		for (int i = 0; i < row_stride; i++)
	//		{
	//			buffer[0][i] = 255;
	//		}
	//	jpeg_write_scanlines(&coutfo, row_pointer, 1);
	//	count++;
	//}
	
	
	jpeg_finish_compress(&coutfo);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_compress(&coutfo);
	jpeg_destroy_decompress(&cinfo);
	

	fclose(infile);
	fclose(outfile);
}


void CImageProView::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	/*FILE* outPut;
	fopen_s(&outPut, "F:\\test\\myout.txt", "wb");

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr injerr;
	cinfo.err = jpeg_std_error(&injerr);
	jpeg_create_decompress(&cinfo);
	FILE* infile;
	fopen_s(&infile, "F:\\test\\t2.jpg", "rb");
	jpeg_stdio_src(&cinfo, infile);
	jpeg_read_header(&cinfo, TRUE);
	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&cinfo);


	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = cinfo.comp_info + ci;

		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (cinfo.mem->access_virt_barray)((j_common_ptr)&cinfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					//fputc(blockptr+bi, outPut);
					CString result;
					result.Format(_T("%d"), blockptr[bi]);
					MessageBox(result);
					fwrite(blockptr + bi, sizeof(blockptr[0]), 1, outPut);
					fputc(' ', outPut);
				}
			}
			fputs("\r\n", outPut);
		}
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	fclose(outPut);*/
	BinaryFileSolver fSolver;
	fSolver.ClearBeforeRead();
	fSolver.setInFilePath("F:\\test\\SchoolBadge.jpg");
	if (fSolver.LoadFile() != 0)
		return;
	uint8_t* FileContent = fSolver.GetBinaryFileContent();
	int FileByteCount = fSolver.GetInFileByteCount();
	int BitCount = FileByteCount * 8;
	char* BitStream = new char[BitCount];
	StreamConvert StrmCnv;
	StrmCnv.byteStreamToBinaryString(FileContent, FileByteCount, BitStream, BitCount, 0);
	delete[] FileContent;

	jpeg_decompress_struct inputInfo;
	jpeg_compress_struct outputInfo;

	jpeg_error_mgr injerr;
	jpeg_error_mgr outjerr;

	inputInfo.err = jpeg_std_error(&injerr);
	outputInfo.err = jpeg_std_error(&outjerr);

	jpeg_create_decompress(&inputInfo);
	jpeg_create_compress(&outputInfo);

	FILE* inFile;
	FILE* outFile;
	fopen_s(&inFile, "F:\\test\\t1.jpg", "rb");
	fopen_s(&outFile, "F:\\test\\t5.jpg", "wb");
	
	jpeg_stdio_src(&inputInfo, inFile);
	jpeg_stdio_dest(&outputInfo, outFile);
	
	jpeg_read_header(&inputInfo, TRUE);

	outputInfo.image_width = inputInfo.image_width;
	outputInfo.image_height = inputInfo.image_height;
	outputInfo.input_components = inputInfo.num_components;
	outputInfo.in_color_space = inputInfo.out_color_space;
	jpeg_set_defaults(&outputInfo);

	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&inputInfo);

	int HideFileCounter = 0;

	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = inputInfo.comp_info + ci;
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (inputInfo.mem->access_virt_barray)((j_common_ptr)&inputInfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					//blockptr[bi] = 3;
					if (HideFileCounter == BitCount)
						break;
					if (blockptr[bi] == 0)
						continue;

					switch (BitStream[HideFileCounter])
					{
					case '0':
						if (blockptr[bi] == 1)
							blockptr[bi] += 1;
						else
						{
							if (blockptr[bi] % 2 == 1)
								blockptr[bi] -= 1;
						}
						break;
					case '1':
						if (blockptr[bi] % 2 == 0)
							blockptr[bi] -= 1;
						break;
					default:
						break;
					}
					HideFileCounter++;

				}
				if (HideFileCounter == BitCount)
					break;
			}
			if (HideFileCounter == BitCount)
				break;
		}
		if (HideFileCounter == BitCount)
			break;
	}

	jpeg_copy_critical_parameters(&inputInfo, &outputInfo);
	jpeg_write_coefficients(&outputInfo, coeff_arrays);

	jpeg_finish_compress(&outputInfo);
	jpeg_finish_decompress(&inputInfo);
	jpeg_destroy_compress(&outputInfo);
	jpeg_destroy_decompress(&inputInfo);


	fclose(inFile);
	fclose(outFile);

}


void CImageProView::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码
	jpeg_decompress_struct inputInfo;
	jpeg_compress_struct outputInfo;

	jpeg_error_mgr injerr;
	jpeg_error_mgr outjerr;

	inputInfo.err = jpeg_std_error(&injerr);
	outputInfo.err = jpeg_std_error(&outjerr);

	jpeg_create_decompress(&inputInfo);
	jpeg_create_compress(&outputInfo);

	FILE* inputFile;
	FILE* outputFile;

	fopen_s(&inputFile, "F:\\test\\t5.jpg", "rb");
	fopen_s(&outputFile, "F:\\test\\t6.jpg", "wb");

	jpeg_stdio_src(&inputInfo, inputFile);
	jpeg_stdio_dest(&outputInfo, outputFile);

	jpeg_read_header(&inputInfo, TRUE);

	outputInfo.image_width = inputInfo.image_width;
	outputInfo.image_height = inputInfo.image_height;
	outputInfo.input_components = inputInfo.num_components;
	outputInfo.in_color_space = inputInfo.out_color_space;
	jpeg_set_defaults(&outputInfo);

	jvirt_barray_ptr* coeff_arrays;
	coeff_arrays = jpeg_read_coefficients(&inputInfo);

	char* HideBitStream = new char[97480];
	int HideBitCountThreshold = 97480;
	int HideFileCounter = 0;

	for (int ci = 0; ci < 3; ci++)
	{
		JBLOCKARRAY buffer;
		JCOEFPTR blockptr;
		jpeg_component_info* compptr;
		compptr = inputInfo.comp_info + ci;
		for (int by = 0; by < compptr->height_in_blocks; by++)
		{
			buffer = (inputInfo.mem->access_virt_barray)((j_common_ptr)&inputInfo, coeff_arrays[ci], 0, (JDIMENSION)1, FALSE);
			for (int bx = 0; bx < compptr->width_in_blocks; bx++)
			{
				blockptr = buffer[by][bx];
				for (int bi = 0; bi < 64; bi++)
				{
					//blockptr[bi] = 3;
					if (HideFileCounter == HideBitCountThreshold)
						break;
					if (blockptr[bi] == 0)
						continue;

					short bitFlg = blockptr[bi] % 2;
					switch (bitFlg)
					{
					case 0:
						HideBitStream[HideFileCounter] = '0';
						break;
					case 1:
						HideBitStream[HideFileCounter] = '1';
						break;
					default:
						break;
					}
					HideFileCounter++;
				}
				if (HideFileCounter == HideBitCountThreshold)
					break;
			}
			if (HideFileCounter == HideBitCountThreshold)
				break;
		}
		if (HideFileCounter == HideBitCountThreshold)
			break;
	}

	jpeg_copy_critical_parameters(&inputInfo, &outputInfo);
	jpeg_write_coefficients(&outputInfo, coeff_arrays);

	jpeg_finish_compress(&outputInfo);
	jpeg_finish_decompress(&inputInfo);
	jpeg_destroy_compress(&outputInfo);
	jpeg_destroy_decompress(&inputInfo);


	fclose(inputFile);
	fclose(outputFile);

	BinaryFileSolver outSolver;
	outSolver.setOutFilePath("F:\\test\\SchoolBadgeCopy.jpg");
	uint8_t* newFile = new uint8_t[HideBitCountThreshold / 8];
	StreamConvert StrmCon;
	StrmCon.byteStreamToBinaryString(newFile, HideBitCountThreshold / 8, HideBitStream, HideBitCountThreshold, 1);
	outSolver.AppendFile(newFile, HideBitCountThreshold / 8);


}
