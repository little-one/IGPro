
// ImageProView.cpp : CImageProView ���ʵ��
//

#include "stdafx.h"
// SHARED_HANDLERS ������ʵ��Ԥ��������ͼ������ɸѡ�������
// ATL ��Ŀ�н��ж��壬�����������Ŀ�����ĵ����롣
#ifndef SHARED_HANDLERS
#include "ImagePro.h"
#endif

#include "ImageProDoc.h"
#include "ImageProView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <setjmp.h>



extern "C" 
{ 
#include "jpeglib.h" 
}

//#pragma comment(lib,"./libjpeg.lib")
// CImageProView

IMPLEMENT_DYNCREATE(CImageProView, CFormView)

BEGIN_MESSAGE_MAP(CImageProView, CFormView)
	ON_BN_CLICKED(IDC_BUTTON1, &CImageProView::OnBnClickedButton1)
END_MESSAGE_MAP()

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
};

// CImageProView ����/����

CImageProView::CImageProView()
	: CFormView(IDD_IMAGEPRO_FORM)
{
	// TODO: �ڴ˴���ӹ������

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
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

	return CFormView::PreCreateWindow(cs);
}

void CImageProView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

}


// CImageProView ���

#ifdef _DEBUG
void CImageProView::AssertValid() const
{
	CFormView::AssertValid();
}

void CImageProView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CImageProDoc* CImageProView::GetDocument() const // �ǵ��԰汾��������
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CImageProDoc)));
	return (CImageProDoc*)m_pDocument;
}
#endif //_DEBUG


// CImageProView ��Ϣ�������

void CImageProView::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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

	jpeg_start_decompress(&cinfo);
	jpeg_start_compress(&coutfo, TRUE);

	int row_stride = cinfo.output_width * cinfo.output_components;
	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
	JSAMPROW row_pointer[1];
	int count = 0;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);
		row_pointer[0] = buffer[0];
		if (count < 50)
			for (int i = 0; i < row_stride; i++)
			{
				buffer[0][i] = 255;
			}
		jpeg_write_scanlines(&coutfo, row_pointer, 1);
		count++;
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_finish_compress(&coutfo);

	jpeg_destroy_decompress(&cinfo);
	jpeg_destroy_compress(&coutfo);

	fclose(infile);
	fclose(outfile);
}
