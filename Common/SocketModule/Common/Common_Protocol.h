#pragma once

enum 
{
    SVC_CODE_START  = 1000,

    // 인증
    SVC_AUTH        = 1001,

    // 조회
    SVC_FSQ01       = 1100,                     // 계좌 예탁자산 조회
    SVC_FSQ02,                                  // 미결제 잔고내역 조회
    SVC_FSQ03,                                  // 주문 체결내역 조회

    // 주문
    SVC_FSO01       = 1200,                     // 신규주문
    SVC_FSO02,                                  // 정정주문
    SVC_FSO03,                                  // 취소주문

    // 실시간 통보
    SVC_FSR01       = 1300,                     // 주문접수
    SVC_FSR02,                                  // 주문체결
    SVC_FSR_ERR,                                // 실시간 메시지 에러(주문거부 등)

    // FreeCap과 연결 종료
    SVC_DISCONNECT  = 1400,

    SVC_CODE_END,
};

#pragma pack( push, 1 )

typedef struct _FS_HEADER
{
    UINT    uiSvcCode;                          // 서비스 코드
    UINT    uiErrCode;                          // 에러 코드(SM은 0으로 Setting)
    UINT    uiDataLen;                          // 데이터 길이
    UINT    uiChecksum;                         // 헤더 검증용 체크섬
} FS_HEADER, *PFS_HEADER;

#define MAKE_FS_HEADER(header, svc, err, len) \
{\
    (header)->uiSvcCode     = svc;\
    (header)->uiErrCode     = err;\
    (header)->uiDataLen     = len;\
    (header)->uiChecksum    = svc ^ err ^ len;\
}

// 인증
typedef struct _FS_AUTH_SM2FS
{
    char pAuthData[0];
} FS_AUTH_SM2FS, *PFS_AUTH_SM2FS;

typedef struct _FS_AUTH_FS2SM
{
    char pNewAuthData[0];
} FS_AUTH_FS2SM, *PFS_AUTH_FS2SM;

typedef struct _FS2SM_CNT_BLOCK
{
    UINT uiCount;                           // 조회 데이터 레코드 개수
} FS2SM_CNT_BLOCK, *PFS2SM_CNT_BLOCK;

// 에러 데이터
typedef struct _FS_ERR_FS2SM
{
    char szErrCode              [5];        // 에러 코드
    char pErrData               [0];        // 에러 데이터 
} FS_ERR_FS2SM, *PFS_ERR_FS2SM;

// 일반 에러 메시지 데이터
typedef struct _FS_ERR_FS2SM_MSG_ONLY
{
    char szErrMsg               [0];        // 에러 메시지
} FS_ERR_FS2SM_MSG_ONLY, *PFS_ERR_FS2SM_MSG_ONLY;

// 주문 거부 에러 데이터
typedef struct _FS_ERR_FS2SM_REJECT
{
    int  nOrdKeyNum;                        // 주문키
    char szOrdNo                [20];       // 주문번호
    char szOrdOrgNo             [20];       // 원주문번호
    char szErrMsg               [0];        // 에러 메시지
} FS_ERR_FS2SM_REJECT, *PFS_ERR_FS2SM_REJECT;

// 계좌 예탁 자산 조회
typedef struct _FSQ01_SM2FS
{
    char szAccNo                [20];       // 계좌번호
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSQ01_SM2FS, *PFSQ01_SM2FS;

typedef struct _FSQ01_FS2SM
{
    char szCrcyCode             [10];       // 통화코드
    char szFutsLqdtPnlAmt       [20];       // 청산손익
    char szFutsEvalPnlAmt       [20];       // 평가손익
    char szFutsCmsnAmt          [20];       // 수수료
    char szFutsWthdwAbleAmt     [20];       // 인출가능금액
    char szFutsCsgnMgn          [20];       // 위탁증거금
    char szOvrsFutsSplmMgn      [20];       // 추가증거금
    char szFutsOrdAbleAmt       [20];       // 주문가능금액
    char szTotalFutsDps         [20];       // 예탁금 총액
    char szFutsEvalDpstgTotAmt  [20];       // 예탁금 잔액
} FSQ01_FS2SM, *PFSQ01_FS2SM;

// 잔고 내역 조회
typedef struct _FSQ02_SM2FS
{
    char szAccNo                [20];       // 계좌번호
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSQ02_SM2FS, *PFSQ02_SM2FS;

typedef struct _FSQ02_FS2SM
{
    char szCodeVal              [32];       // 종목코드
    char szBuySellTp            [1 ];       // 매매구분 (1.매수 2.매도)
    char szBalQty               [10];       // 미결제(잔고)수량
    char szPchsPrc              [20];       // 평균가
    char szNowPrc               [20];       // 현재가
    char szFutsEvalPnlAmt       [20];       // 평가손익
} FSQ02_FS2SM, *PFSQ02_FS2SM;

// 체결/미체결 내역 조회
typedef struct _FSQ03_SM2FS
{
    char szAccNo                [20];       // 계좌번호
    char szReqDate              [8 ];       // 조회 날짜
    char szReqTp                [1 ];       // 조회 내역 종류 (0 : 전체, 1 : 체결, 2 : 미체결)
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSQ03_SM2FS, *PFSQ03_SM2FS;

typedef struct _FSQ03_FS2SM
{
    char szRepTp                [1 ];       // 조회 내역 응답 종류 (0 : 전체, 1 : 체결, 2 : 미체결)
    char szFutsOrdNo            [20];       // 주문번호
    char szOrgOrdNo             [20];       // 원주문번호
    char szCodeVal              [32];       // 종목코드
    char szBuySellTp            [1 ];       // 매매구분         1 : 매수,   2 : 매도
    char szOrdTp                [1 ];       // 주문구분         1 : 신규,   2 : 정정,   3 : 취소
    char szPriceTp              [1 ];       // 가격조건         1 : 시장가, 2 : 지정가
    char szOrdPrice             [15];       // 주문가격
    char szOrdQty               [10];       // 주문수량
    char szExecPrice            [15];       // 체결가격
    char szExecQty              [10];       // 체결수량
    char szNonExecQty           [10];       // 미체결수량
    char szOrdTime              [8 ];       // 주문시간
} FSQ03_FS2SM, *PFSQ03_FS2SM;

// 주문 공통 데이터
typedef struct _FS_ORD_COMMON
{
    int  nOrdKeyNum;                        // 주문키 
    char szAccNo                [20];       // 계좌번호
    char szCodeVal              [18];       // 종목코드
    char szOrdTp                [1 ];       // 주문구분코드     0 : 신규,   1 : 정정,   2 : 취소
} FS_ORD_COMMON, *PFS_ORD_COMMON;

// 신규 주문
typedef struct _FSO01_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szStrategyTp           [1 ];       // SignalMaker 주문구분코드   0 : 알고리즘 실행 주문, 1 : 매뉴얼 주문
    char szBuySellTp            [1 ];       // 매매구분         1 : 매수,   2 : 매도
    char szPriceTp              [1 ];       // 가격조건         1 : 시장가, 2 : 지정가
    char szOrdPrice             [15];       // 주문가격
    char szOrdQty               [10];       // 주문수량
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSO01_SM2FS, *PFSO01_SM2FS;

typedef struct _FSO01_FS2SM
{
    int  nOrdKeyNum;                        // 주문키 
    char szOrdNo                [20];       // 주문번호
    char szOrdQty               [10];       // 주문수량
	char szResult               [1];		// 0 : 성공, 9 : 오류
} FSO01_FS2SM, *PFSO01_FS2SM;

// 정정 주문
typedef struct _FSO02_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szOrgOrdNo             [20];       // 원주문번호
    char szStrategyTp           [1 ];       // SignalMaker 주문구분코드   0 : 알고리즘 실행 주문, 1 : 매뉴얼 주문
    char szPriceTp              [1 ];       // 가격조건         1 : 시장가, 2 : 지정가
    char szOrdPrice             [15];       // 주문가격
    char szOrdQty               [10];       // 주문수량
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSO02_SM2FS, *PFSO02_SM2FS;

typedef struct _FSO02_FS2SM
{
    int  nOrdKeyNum;                        // 주문키 
    char szOrdNo                [20];       // 주문번호
} FSO02_FS2SM, *PFSO02_FS2SM;

// 취소 주문
typedef struct _FSO03_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szOrgOrdNo             [20];       // 원주문번호
    char pAuthData              [0 ];       // 인증용 암호화 데이터
} FSO03_SM2FS, *PFSO03_SM2FS;

typedef struct _FSO03_FS2SM
{
    int  nOrdKeyNum;                        // 주문키 
    char szOrdNo                [20];       // 주문번호
} FSO03_FS2SM, *PFSO03_FS2SM;

// 실시간 통보 - 주문접수
typedef struct _FSR01_FS2SM
{
    char szAccNo                [20];       // 계좌번호
    char szOrdNo                [20];       // 주문번호
    char szOrgOrdNo             [20];       // 원주문번호
    char szCodeVal              [32];       // 종목코드
    char szOrdTp                [1 ];       // 주문구분코드     1 : 신규,   2 : 정정,   3 : 취소
    char szBuySellTp            [1 ];       // 매매구분         1 : 매수,   2 : 매도
    char szPriceTp              [1 ];       // 가격조건         1 : 시장가  2  :지정가
    char szOrdPrice             [15];       // 주문가격
    char szOrdQty               [10];       // 주문수량
    char szBalQty               [10];       // 미결제(잔고)수량
    char szOrdDate              [8 ];       // 주문일자
    char szOrdTime              [8 ];       // 주문시간
} FSR01_FS2SM, *PFSR01_FS2SM;

// 실시간 통보 - 주문 체결
typedef struct _FSR02_FS2SM
{
    char szAccNo                [20];       // 계좌번호
    char szOrdNo                [20];       // 주문번호
    char szCodeVal              [32];       // 종목코드
    char szBuySellTp            [1 ];       // 매매구분         1 : 매수,   2 : 매도
    char szOrdPrice             [15];       // 주문가격
    char szOrdQty               [10];       // 주문수량
    char szExecPrice            [15];       // 체결가격
    char szExecQty              [10];       // 체결수량
    char szRemainQty            [10];       // 미체결수량
    char szTrdAmt               [20];       // 약정금액
    char szCmsnAmt              [15];       // 수수료
    char szOrdDate              [8 ];       // 주문일자
    char szOrdTime              [8 ];       // 주문시간
    char szExecDate             [8 ];       // 체결일자
    char szExecTime             [8 ];       // 체결시간
} FSR02_FS2SM, *PFSR02_FS2SM;

#pragma pack( pop ) 