
// ImagePro.h : ImagePro Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CImageProApp:
// �йش����ʵ�֣������ ImagePro.cpp
//

class CImageProApp : public CWinApp
{
public:
	CImageProApp();


// ��д
public:
	virtual BOOL InitInstance();

// ʵ��
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CImageProApp theApp;
