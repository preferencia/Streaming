
// WindowClient.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CWindowClientApp:
// �� Ŭ������ ������ ���ؼ��� WindowClient.cpp�� �����Ͻʽÿ�.
//

class CWindowClientApp : public CWinAppEx
{
public:
	CWindowClientApp();

// �������Դϴ�.
	public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CWindowClientApp theApp;