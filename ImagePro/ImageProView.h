
// ImageProView.h : CImageProView ��Ľӿ�
//

#pragma once


class CImageProView : public CFormView
{
protected: // �������л�����
	CImageProView();
	DECLARE_DYNCREATE(CImageProView)

public:
#ifdef AFX_DESIGN_TIME
	enum{ IDD = IDD_IMAGEPRO_FORM };
#endif

// ����
public:
	CImageProDoc* GetDocument() const;

// ����
public:

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��
	virtual void OnInitialUpdate(); // ������һ�ε���

// ʵ��
public:
	virtual ~CImageProView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// ���ɵ���Ϣӳ�亯��
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // ImageProView.cpp �еĵ��԰汾
inline CImageProDoc* CImageProView::GetDocument() const
   { return reinterpret_cast<CImageProDoc*>(m_pDocument); }
#endif

