
// ImageProView.h : CImageProView ��Ľӿ�
//

#pragma once

#ifndef EXTERNJPEGLIB
extern "C"
{
#include "jpeglib.h" 
}
#endif // !EXTERNJPEGLIB



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
	//void jpeg_mem_error_exit(j_common_ptr cinfo);
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
public:
	//afx_msg void OnBnClickedButton1();
	//afx_msg void OnBnClickedButton2();
	//afx_msg void OnBnClickedButton3();
	//afx_msg void OnBnClickedButton4();
	////void my_jpeg_error_exit(j_common_ptr cinfo);
	//afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
};

#ifndef _DEBUG  // ImageProView.cpp �еĵ��԰汾
inline CImageProDoc* CImageProView::GetDocument() const
   { return reinterpret_cast<CImageProDoc*>(m_pDocument); }
#endif

//static void my_jpeg_error_exit(j_common_ptr cinfo);