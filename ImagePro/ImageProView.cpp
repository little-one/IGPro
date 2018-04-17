
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

#ifndef HIDENFILEPROCESS
#define HIDENFILEPROCESS
#include "BinaryFileSolver.h"
#include "Encrypt_Decrpty.h"
#include "FolderMonitor.h"
#include "StreamConvert.h"
#endif // !HIDENFILEPROCESS

#ifndef MYDATAHIDINGALGORITHM
#define MYDATAHIDINGALGORITHM
#include "LSBLossyImplantAlg.h"
#include "IJpegOpeAlg.h"
#include "HighCapabilityCompositeAlg.h"
#endif // !MYDATAHIDINGALGORITHM






//#pragma comment(lib,"./libjpeg.lib")
// CImageProView

IMPLEMENT_DYNCREATE(CImageProView, CFormView)

BEGIN_MESSAGE_MAP(CImageProView, CFormView)
	/*ON_BN_CLICKED(IDC_BUTTON1, &CImageProView::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CImageProView::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CImageProView::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CImageProView::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CImageProView::OnBnClickedButton5)*/
	ON_BN_CLICKED(IDC_BUTTON1, &CImageProView::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CImageProView::OnBnClickedButton2)
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
	//LSBLossyImplantAlg lsb;
	FileList flist;
	flist.CarrierImagePathList.push_back("F:\\test\\tt1.jpg");
	//flist.CarrierImagePathList.push_back("F:\\test\\tt1.jpg");
	flist.HidenFilePath = "F:\\test\\seimg.jpg";
	flist.NewFilePathList.push_back("F:\\test\\t2.jpg");
	//flist.NewFilePathList.push_back("F:\\test\\tt2.jpg");
	IJpegOpeAlg* JpegOpeAlg = new HighCapabilityCompositeAlg(&flist, 3, 2);
	//IJpegOpeAlg* JpegOpeAlg = new LSBLossyImplantAlg(&flist);
	//JpegOpeAlg->CalPayLoads(&flist);
	JpegOpeAlg->ExecuteEmbedingAlg();
}

void CImageProView::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	FileList flist;
	flist.CarrierImagePathList.push_back("F:\\test\\t2.jpg");
	//flist.CarrierImagePathList.push_back("F:\\test\\tt2.jpg");
	flist.HidenFilePath = "F:\\test\\CpySeimg";
	//flist.NewFilePathList.push_back("F:\\test\\t3.jpg");
	flist.NewFilePathList.push_back("F:\\test\\t3.jpg");
	IJpegOpeAlg* JpegOpeAlg = new HighCapabilityCompositeAlg(&flist, 3, 2);
	//IJpegOpeAlg* JpegOpeAlg = new LSBLossyImplantAlg(&flist);
	JpegOpeAlg->ExecuteExtractingAlg();
}
