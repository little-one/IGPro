#pragma once



// CLView ������ͼ

class CLView : public CFormView
{
	DECLARE_DYNCREATE(CLView)

protected:
	CLView();           // ��̬������ʹ�õ��ܱ����Ĺ��캯��
	virtual ~CLView();

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LVIEWFORM };
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


