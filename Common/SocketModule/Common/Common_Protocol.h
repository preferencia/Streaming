#pragma once

enum 
{
    SVC_CODE_START  = 1000,

    // ����
    SVC_AUTH        = 1001,

    // ��ȸ
    SVC_FSQ01       = 1100,                     // ���� ��Ź�ڻ� ��ȸ
    SVC_FSQ02,                                  // �̰��� �ܰ��� ��ȸ
    SVC_FSQ03,                                  // �ֹ� ü�᳻�� ��ȸ

    // �ֹ�
    SVC_FSO01       = 1200,                     // �ű��ֹ�
    SVC_FSO02,                                  // �����ֹ�
    SVC_FSO03,                                  // ����ֹ�

    // �ǽð� �뺸
    SVC_FSR01       = 1300,                     // �ֹ�����
    SVC_FSR02,                                  // �ֹ�ü��
    SVC_FSR_ERR,                                // �ǽð� �޽��� ����(�ֹ��ź� ��)

    // FreeCap�� ���� ����
    SVC_DISCONNECT  = 1400,

    SVC_CODE_END,
};

#pragma pack( push, 1 )

typedef struct _FS_HEADER
{
    UINT    uiSvcCode;                          // ���� �ڵ�
    UINT    uiErrCode;                          // ���� �ڵ�(SM�� 0���� Setting)
    UINT    uiDataLen;                          // ������ ����
    UINT    uiChecksum;                         // ��� ������ üũ��
} FS_HEADER, *PFS_HEADER;

#define MAKE_FS_HEADER(header, svc, err, len) \
{\
    (header)->uiSvcCode     = svc;\
    (header)->uiErrCode     = err;\
    (header)->uiDataLen     = len;\
    (header)->uiChecksum    = svc ^ err ^ len;\
}

// ����
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
    UINT uiCount;                           // ��ȸ ������ ���ڵ� ����
} FS2SM_CNT_BLOCK, *PFS2SM_CNT_BLOCK;

// ���� ������
typedef struct _FS_ERR_FS2SM
{
    char szErrCode              [5];        // ���� �ڵ�
    char pErrData               [0];        // ���� ������ 
} FS_ERR_FS2SM, *PFS_ERR_FS2SM;

// �Ϲ� ���� �޽��� ������
typedef struct _FS_ERR_FS2SM_MSG_ONLY
{
    char szErrMsg               [0];        // ���� �޽���
} FS_ERR_FS2SM_MSG_ONLY, *PFS_ERR_FS2SM_MSG_ONLY;

// �ֹ� �ź� ���� ������
typedef struct _FS_ERR_FS2SM_REJECT
{
    int  nOrdKeyNum;                        // �ֹ�Ű
    char szOrdNo                [20];       // �ֹ���ȣ
    char szOrdOrgNo             [20];       // ���ֹ���ȣ
    char szErrMsg               [0];        // ���� �޽���
} FS_ERR_FS2SM_REJECT, *PFS_ERR_FS2SM_REJECT;

// ���� ��Ź �ڻ� ��ȸ
typedef struct _FSQ01_SM2FS
{
    char szAccNo                [20];       // ���¹�ȣ
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSQ01_SM2FS, *PFSQ01_SM2FS;

typedef struct _FSQ01_FS2SM
{
    char szCrcyCode             [10];       // ��ȭ�ڵ�
    char szFutsLqdtPnlAmt       [20];       // û�����
    char szFutsEvalPnlAmt       [20];       // �򰡼���
    char szFutsCmsnAmt          [20];       // ������
    char szFutsWthdwAbleAmt     [20];       // ���Ⱑ�ɱݾ�
    char szFutsCsgnMgn          [20];       // ��Ź���ű�
    char szOvrsFutsSplmMgn      [20];       // �߰����ű�
    char szFutsOrdAbleAmt       [20];       // �ֹ����ɱݾ�
    char szTotalFutsDps         [20];       // ��Ź�� �Ѿ�
    char szFutsEvalDpstgTotAmt  [20];       // ��Ź�� �ܾ�
} FSQ01_FS2SM, *PFSQ01_FS2SM;

// �ܰ� ���� ��ȸ
typedef struct _FSQ02_SM2FS
{
    char szAccNo                [20];       // ���¹�ȣ
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSQ02_SM2FS, *PFSQ02_SM2FS;

typedef struct _FSQ02_FS2SM
{
    char szCodeVal              [32];       // �����ڵ�
    char szBuySellTp            [1 ];       // �Ÿű��� (1.�ż� 2.�ŵ�)
    char szBalQty               [10];       // �̰���(�ܰ�)����
    char szPchsPrc              [20];       // ��հ�
    char szNowPrc               [20];       // ���簡
    char szFutsEvalPnlAmt       [20];       // �򰡼���
} FSQ02_FS2SM, *PFSQ02_FS2SM;

// ü��/��ü�� ���� ��ȸ
typedef struct _FSQ03_SM2FS
{
    char szAccNo                [20];       // ���¹�ȣ
    char szReqDate              [8 ];       // ��ȸ ��¥
    char szReqTp                [1 ];       // ��ȸ ���� ���� (0 : ��ü, 1 : ü��, 2 : ��ü��)
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSQ03_SM2FS, *PFSQ03_SM2FS;

typedef struct _FSQ03_FS2SM
{
    char szRepTp                [1 ];       // ��ȸ ���� ���� ���� (0 : ��ü, 1 : ü��, 2 : ��ü��)
    char szFutsOrdNo            [20];       // �ֹ���ȣ
    char szOrgOrdNo             [20];       // ���ֹ���ȣ
    char szCodeVal              [32];       // �����ڵ�
    char szBuySellTp            [1 ];       // �Ÿű���         1 : �ż�,   2 : �ŵ�
    char szOrdTp                [1 ];       // �ֹ�����         1 : �ű�,   2 : ����,   3 : ���
    char szPriceTp              [1 ];       // ��������         1 : ���尡, 2 : ������
    char szOrdPrice             [15];       // �ֹ�����
    char szOrdQty               [10];       // �ֹ�����
    char szExecPrice            [15];       // ü�ᰡ��
    char szExecQty              [10];       // ü�����
    char szNonExecQty           [10];       // ��ü�����
    char szOrdTime              [8 ];       // �ֹ��ð�
} FSQ03_FS2SM, *PFSQ03_FS2SM;

// �ֹ� ���� ������
typedef struct _FS_ORD_COMMON
{
    int  nOrdKeyNum;                        // �ֹ�Ű 
    char szAccNo                [20];       // ���¹�ȣ
    char szCodeVal              [18];       // �����ڵ�
    char szOrdTp                [1 ];       // �ֹ������ڵ�     0 : �ű�,   1 : ����,   2 : ���
} FS_ORD_COMMON, *PFS_ORD_COMMON;

// �ű� �ֹ�
typedef struct _FSO01_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szStrategyTp           [1 ];       // SignalMaker �ֹ������ڵ�   0 : �˰��� ���� �ֹ�, 1 : �Ŵ��� �ֹ�
    char szBuySellTp            [1 ];       // �Ÿű���         1 : �ż�,   2 : �ŵ�
    char szPriceTp              [1 ];       // ��������         1 : ���尡, 2 : ������
    char szOrdPrice             [15];       // �ֹ�����
    char szOrdQty               [10];       // �ֹ�����
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSO01_SM2FS, *PFSO01_SM2FS;

typedef struct _FSO01_FS2SM
{
    int  nOrdKeyNum;                        // �ֹ�Ű 
    char szOrdNo                [20];       // �ֹ���ȣ
    char szOrdQty               [10];       // �ֹ�����
	char szResult               [1];		// 0 : ����, 9 : ����
} FSO01_FS2SM, *PFSO01_FS2SM;

// ���� �ֹ�
typedef struct _FSO02_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szOrgOrdNo             [20];       // ���ֹ���ȣ
    char szStrategyTp           [1 ];       // SignalMaker �ֹ������ڵ�   0 : �˰��� ���� �ֹ�, 1 : �Ŵ��� �ֹ�
    char szPriceTp              [1 ];       // ��������         1 : ���尡, 2 : ������
    char szOrdPrice             [15];       // �ֹ�����
    char szOrdQty               [10];       // �ֹ�����
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSO02_SM2FS, *PFSO02_SM2FS;

typedef struct _FSO02_FS2SM
{
    int  nOrdKeyNum;                        // �ֹ�Ű 
    char szOrdNo                [20];       // �ֹ���ȣ
} FSO02_FS2SM, *PFSO02_FS2SM;

// ��� �ֹ�
typedef struct _FSO03_SM2FS
{
    FS_ORD_COMMON          OrdCommon;
    char szOrgOrdNo             [20];       // ���ֹ���ȣ
    char pAuthData              [0 ];       // ������ ��ȣȭ ������
} FSO03_SM2FS, *PFSO03_SM2FS;

typedef struct _FSO03_FS2SM
{
    int  nOrdKeyNum;                        // �ֹ�Ű 
    char szOrdNo                [20];       // �ֹ���ȣ
} FSO03_FS2SM, *PFSO03_FS2SM;

// �ǽð� �뺸 - �ֹ�����
typedef struct _FSR01_FS2SM
{
    char szAccNo                [20];       // ���¹�ȣ
    char szOrdNo                [20];       // �ֹ���ȣ
    char szOrgOrdNo             [20];       // ���ֹ���ȣ
    char szCodeVal              [32];       // �����ڵ�
    char szOrdTp                [1 ];       // �ֹ������ڵ�     1 : �ű�,   2 : ����,   3 : ���
    char szBuySellTp            [1 ];       // �Ÿű���         1 : �ż�,   2 : �ŵ�
    char szPriceTp              [1 ];       // ��������         1 : ���尡  2  :������
    char szOrdPrice             [15];       // �ֹ�����
    char szOrdQty               [10];       // �ֹ�����
    char szBalQty               [10];       // �̰���(�ܰ�)����
    char szOrdDate              [8 ];       // �ֹ�����
    char szOrdTime              [8 ];       // �ֹ��ð�
} FSR01_FS2SM, *PFSR01_FS2SM;

// �ǽð� �뺸 - �ֹ� ü��
typedef struct _FSR02_FS2SM
{
    char szAccNo                [20];       // ���¹�ȣ
    char szOrdNo                [20];       // �ֹ���ȣ
    char szCodeVal              [32];       // �����ڵ�
    char szBuySellTp            [1 ];       // �Ÿű���         1 : �ż�,   2 : �ŵ�
    char szOrdPrice             [15];       // �ֹ�����
    char szOrdQty               [10];       // �ֹ�����
    char szExecPrice            [15];       // ü�ᰡ��
    char szExecQty              [10];       // ü�����
    char szRemainQty            [10];       // ��ü�����
    char szTrdAmt               [20];       // �����ݾ�
    char szCmsnAmt              [15];       // ������
    char szOrdDate              [8 ];       // �ֹ�����
    char szOrdTime              [8 ];       // �ֹ��ð�
    char szExecDate             [8 ];       // ü������
    char szExecTime             [8 ];       // ü��ð�
} FSR02_FS2SM, *PFSR02_FS2SM;

#pragma pack( pop ) 