#pragma once



// CRView ������ͼ

class CRView : public CFormView
{
	DECLARE_DYNCREATE(CRView)

protected:
	CRView();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��
	virtual ~CRView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_RVIEWFORM };
#endif
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	DECLARE_MESSAGE_MAP()
};


