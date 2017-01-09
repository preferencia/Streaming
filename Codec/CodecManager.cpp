#include "stdafx.h"
#include "CodecManager.h"
#include "Encoder.h"
#include "Decoder.h"


CCodecManager::CCodecManager()
{
}


CCodecManager::~CCodecManager()
{
}


int CCodecManager::CreateCodec(CCodec** ppCodec, int nCodecType)
{
	if (NULL == ppCodec)
	{
		return -1;
	}

	if (NULL != *ppCodec)
	{
		return -2;
	}

	switch (nCodecType)
	{
	case CODEC_TYPE_ENCODER:
		{
			*ppCodec = new CEncoder;
		}
		break;

	case CODEC_TYPE_DECODER:
		{
			*ppCodec = new CDecoder;
		}
		break;

	default:
		break;
	}

	return 0;
}

void CCodecManager::DestroyCodec(CCodec** ppCodec)
{
    if (NULL != ppCodec)
    {
        SAFE_DELETE(*ppCodec);
    }	
}