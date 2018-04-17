
// ImageProView.cpp : CImageProView ���ʵ��
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
using namespace std;
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
