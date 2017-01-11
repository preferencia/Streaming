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
    RQ_ORD_NONSIGN_LIST = 400,      // 미체결내역 조회
    RQ_ORD_SIGN_LIST,               // 체결내역 조회
    RQ_BALANCE_LIST,                // 미결제잔고내역 조회
    RQ_ORD_LIST_ALL,                // 전체주문내역 조회
    RQ_ACC_DEPOSIT,                 // 계좌예탁자산 조회

    RQ_FS_ORD_NEW       = 1000,     // FreeCap 모의투자 신규주문
    RQ_FS_ORD_MODIFY    = 1300,     // FreeCap 모의투자 정정주문
    RQ_FS_ORD_CANCEL    = 1600,     // FreeCap 모의투자 취소주문

    RQ_HANA_ORD_NEW     = 2000,     // 하나투자증권 신규주문
    RQ_HANA_ORD_MODIFY  = 2300,     // 하나투자증권 모의투자 정정주문
    RQ_HANA_ORD_CANCEL  = 2600,     // 하나투자증권 모의투자 취소주문

    RQ_KR_ORD_NEW       = 4000,     // KR 신규주문
    RQ_KR_ORD_MODIFY    = 4300,     // KR 정정주문
    RQ_KR_ORD_CANCEL    = 4600,     // KR 취소주문

    RQ_EBEST_ORD_NEW    = 5000,     // EBest 신규주문
    RQ_EBEST_ORD_MODIFY = 5300,     // EBest 정정주문
    RQ_EBEST_ORD_CANCEL = 5600,     // EBest 취소주문

    RT_ORD_ACCEPT       = 10000,    // 주문접수 통보
    RT_ORD_SIGN,                    // 주문체결 통보
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