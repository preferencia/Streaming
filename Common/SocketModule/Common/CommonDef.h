#pragma once

#ifdef _UNICODE
#define _DEC_SERVER_NAME        ("127.0.0.1")
#else
#define _DEC_SERVER_NAME        _T("127.0.0.1")
#endif
#define _DEC_LISTEN_PORT        (48000)

enum 
{
    SOCK_INDEX_QUERY = 0,
    SOCK_INDEX_ORDER,
    SOCK_INDEX_RECV_REALTIME,
    SOCK_INDEX_END,
};

enum
{
    RQ_ORD_NONSIGN_LIST = 400,      // ��ü�᳻�� ��ȸ
    RQ_ORD_SIGN_LIST,               // ü�᳻�� ��ȸ
    RQ_BALANCE_LIST,                // �̰����ܰ��� ��ȸ
    RQ_ORD_LIST_ALL,                // ��ü�ֹ����� ��ȸ
    RQ_ACC_DEPOSIT,                 // ���¿�Ź�ڻ� ��ȸ

    RQ_FS_ORD_NEW       = 1000,     // FreeCap �������� �ű��ֹ�
    RQ_FS_ORD_MODIFY    = 1300,     // FreeCap �������� �����ֹ�
    RQ_FS_ORD_CANCEL    = 1600,     // FreeCap �������� ����ֹ�

    RQ_HANA_ORD_NEW     = 2000,     // �ϳ��������� �ű��ֹ�
    RQ_HANA_ORD_MODIFY  = 2300,     // �ϳ��������� �������� �����ֹ�
    RQ_HANA_ORD_CANCEL  = 2600,     // �ϳ��������� �������� ����ֹ�

    RQ_KR_ORD_NEW       = 4000,     // KR �ű��ֹ�
    RQ_KR_ORD_MODIFY    = 4300,     // KR �����ֹ�
    RQ_KR_ORD_CANCEL    = 4600,     // KR ����ֹ�

    RQ_EBEST_ORD_NEW    = 5000,     // EBest �ű��ֹ�
    RQ_EBEST_ORD_MODIFY = 5300,     // EBest �����ֹ�
    RQ_EBEST_ORD_CANCEL = 5600,     // EBest ����ֹ�

    RT_ORD_ACCEPT       = 10000,    // �ֹ����� �뺸
    RT_ORD_SIGN,                    // �ֹ�ü�� �뺸
};

enum
{
    WPARAM_SETUP_COMPLETE   = 0,
    WPARAM_SETUP_FAILED,
    WPARAM_AUTH_SUCCESS,
    WPARAM_CONNECTION_CLOSE,
};

enum
{
    LPARAM_ALREADY_EXSIT_LISTEN_SOCKET   = 0,
    LPARAM_COMPLETE_CREATE_LISTEN_SOCKET,
    LPARAM_COMPLETE_SET_CALLBACK_FUNCTION,
};

enum
{
    enumLogTypeStr = 0,
    enumLogTypeData,
};

#define _DEC_CONNECT_CNT        (SOCK_INDEX_END)

#define _DEC_KEY_SRC_VALUE_LEN  (12)
#define _DEC_IP_ADDR_LEN        (16)
#define _DEC_MAX_BUFFER_SIZE    (4096)
#define _DEC_MAX_ORD_KEY_NUM    (300)

#define UM_SYS_TRADING_PROC     (WM_USER + 2000)

#define SAFE_DELETE(p)          {   if (NULL != p) delete p; p = NULL;      }
#define SAFE_DELETE_ARRAY(p)    {   if (NULL != p) delete [] p; p = NULL;   }