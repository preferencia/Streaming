#include "stdafx.h"
#include "ObjectManager.h"
#include "Encoder.h"
#include "Decoder.h"
#include "Muxer.h"
#include "Demuxer.h"

CObjectManager::CObjectManager()
{
}

CObjectManager::~CObjectManager()
{
}

int CObjectManager::CreateObject(void** ppOjbect, int nObjectType)
{
	if (NULL == ppOjbect)
	{
		return -1;
	}

	if (NULL != *ppOjbect)
	{
		return -2;
	}

	switch (nObjectType)
	{
	case OBJECT_TYPE_ENCODER:
		{
			*ppOjbect   = new CEncoder;
		}
		break;

	case OBJECT_TYPE_DECODER:
		{
			*ppOjbect   = new CDecoder;
		}
		break;

	case OBJECT_TYPE_MUXER:
		{
			*ppOjbect   = new CMuxer;
		}
		break;

    case OBJECT_TYPE_DEMUXER:
        {
            *ppOjbect   = new CDemuxer;
        }
        break;

	default:
		break;
	}

	return 0;
}

void CObjectManager::DestroyObject(void** ppOjbect, int nObjectType)
{
    if (NULL != ppOjbect)
    {
		switch (nObjectType)
		{
		case OBJECT_TYPE_ENCODER:
		case OBJECT_TYPE_DECODER:
		{
			CCodec* pCodec	= (CCodec*)*ppOjbect;
			SAFE_DELETE(pCodec);
		}
		break;

		case OBJECT_TYPE_MUXER:
		case OBJECT_TYPE_DEMUXER:
		{
			CMux* pMux		= (CMux*)*ppOjbect;
			SAFE_DELETE(pMux);
		}
		break;

		default:
			break;
		}
		
    }	
}