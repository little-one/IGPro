
// ImageProView.h : CImageProView 类的接口
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
protected: // 仅从序列化创建
	CImageProView();
	DECLARE_DYNCREATE(CImageProView)

public:
#ifdef AFX_DESIGN_TIME
	enum{ IDD = IDD_IMAGEPRO_FORM };
#endif

// 特性
public:
	CImageProDoc* GetDocument() const;

// 操作
public:
	//void jpeg_mem_error_exit(j_common_ptr cinfo);
// 重写
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual void OnInitialUpdate(); // 构造后第一次调用

// 实现
public:
	virtual ~CImageProView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
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
	afx_msg void OnBnClickedButton3();
};

#ifndef _DEBUG  // ImageProView.cpp 中的调试版本
inline CImageProDoc* CImageProView::GetDocument() const
   { return reinterpret_cast<CImageProDoc*>(m_pDocument); }
#endif

//static void my_jpeg_error_exit(j_common_ptr cinfo);