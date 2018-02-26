
// ImageProView.cpp : CImageProView 类的实现
//

#include "stdafx.h"
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

extern "C"
{
#include "jpeglib.h"
}
#pragma comment(lib,"libjpeg.lib")

// CImageProView

IMPLEMENT_DYNCREATE(CImageProView, CFormView)

BEGIN_MESSAGE_MAP(CImageProView, CFormView)
	ON_BN_CLICKED(IDC_BUTTON1, &CImageProView::OnBnClickedButton1)
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
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	FILE* infile;
	JSAMPARRAY buffer;
	int row_stride;
	if ((fopen_s(&infile,"F:\\test\\t1.jpg", "rb")) == NULL)
	{
		return;
	}
	cinfo.err = jpeg_std_error(&jerr.pub);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void)jpeg_read_header(&cinfo, TRUE);
	(void)jpeg_start_decompress(&cinfo);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
	}
	(void)jpeg_finish_decompress(&cinfo);
	////
}
