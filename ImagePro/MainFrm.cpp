
// MainFrm.cpp : CMainFrame ���ʵ��
//

#include "stdafx.h"
#include "ImagePro.h"

#include "MainFrm.h"
/////
#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // ״̬��ָʾ��
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame ����/����

CMainFrame::CMainFrame()
{
	// TODO: �ڴ���ӳ�Ա��ʼ������
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("δ�ܴ���״̬��\n");
		return -1;      // δ�ܴ���
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));
	//SetMenu(NULL);

	//SetWindowLong(this->m_hWnd, GWL_STYLE, 0);
	//SetWindowLong(this->m_hWnd, GWL_EXSTYLE, 0);

	if (!m_wndSplitter.CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE))
	{
		TRACE("Failed to Create StaticSplitter\n");
		return NULL;
	}

	//pContext->m_pNewViewClass = RUNTIME_CLASS(CLView);

	//CRect rect;
	//this->GetClientRect(&rect);
	//SIZE size;
	//size.cx = rect.Width() / 4;
	//size.cy = rect.Height();

	//m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, size, pContext);

	//pContext->m_pNewViewClass = RUNTIME_CLASS(CRView);
	//size.cx = rect.Width() * 3 / 4;
	//size.cy = rect.Height();
	//m_wndSplitter.CreateView(0, 1, pContext->m_pNewViewClass, size, pContext);

	//clv = (CLView*)m_wndSplitter.GetPane(0, 0);
	//crv = (CRView*)m_wndSplitter.GetPane(0, 1);

	return 0;
}

//BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext * pContext)
//{
//	/*if (!m_wndSplitter.CreateStatic(this, 1, 2, WS_CHILD | WS_VISIBLE))
//	{
//		TRACE("Failed to Create StaticSplitter\n");
//		return NULL;
//	}
//	pContext->m_pNewViewClass = RUNTIME_CLASS(CLView);
//
//	CRect rect;
//	this->GetClientRect(&rect);
//	SIZE size;
//	size.cx = rect.Width() / 4;
//	size.cy = rect.Height();
//
//	m_wndSplitter.CreateView(0, 0, pContext->m_pNewViewClass, size, pContext);
//
//	pContext->m_pNewViewClass = RUNTIME_CLASS(CRView);
//	size.cx = rect.Width() * 3 / 4;
//	size.cy = rect.Height();
//	m_wndSplitter.CreateView(0, 1, pContext->m_pNewViewClass, size, pContext);
//
//	clv = (CLView*)m_wndSplitter.GetPane(0, 0);
//	crv = (CRView*)m_wndSplitter.GetPane(0, 1);*/
//
//	return 0;
//}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

	return TRUE;
}

// CMainFrame ���

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
	int a = 0;
}
#endif //_DEBUG


// CMainFrame ��Ϣ�������



BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	// TODO: �ڴ����ר�ô����/����û���

	return CFrameWnd::OnCreateClient(lpcs, pContext);
}
