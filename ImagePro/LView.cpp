// LView.cpp : 实现文件
//

#include "stdafx.h"
#include "ImagePro.h"
#include "LView.h"


// CLView

IMPLEMENT_DYNCREATE(CLView, CFormView)

CLView::CLView()
	: CFormView(IDD_LVIEWFORM)
{

}

CLView::~CLView()
{
}

void CLView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLView, CFormView)
END_MESSAGE_MAP()


// CLView 诊断

#ifdef _DEBUG
void CLView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CLView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// CLView 消息处理程序
