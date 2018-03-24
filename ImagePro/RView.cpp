// RView.cpp : 实现文件
//

#include "stdafx.h"
#include "ImagePro.h"
#include "RView.h"


// CRView

IMPLEMENT_DYNCREATE(CRView, CFormView)

CRView::CRView()
	: CFormView(IDD_RVIEWFORM)
{

}

CRView::~CRView()
{
}

void CRView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRView, CFormView)
END_MESSAGE_MAP()


// CRView 诊断

#ifdef _DEBUG
void CRView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CRView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// CRView 消息处理程序
