
// ImageProView.h : CImageProView 类的接口
//

#pragma once


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
};

#ifndef _DEBUG  // ImageProView.cpp 中的调试版本
inline CImageProDoc* CImageProView::GetDocument() const
   { return reinterpret_cast<CImageProDoc*>(m_pDocument); }
#endif

