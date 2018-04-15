#include "stdafx.h"
#include "HWAccel.h"

namespace HWAccel
{
	int 	InitHwCodec(AVCodecContext* pCtx, const enum AVHWDeviceType type, AVBufferRef** ppHwDeviceCtx, AVBufferRef** ppHwFramesCtx)
	{
		if ((NULL == pCtx) || (AV_PIX_FMT_NONE == type) || (NULL == ppHwDeviceCtx))
		{
			TraceLog("Input AVCodecContext is NULL");
			return -1;
		}
		
		if (NULL != *ppHwDeviceCtx) 
		{
			av_buffer_unref(ppHwDeviceCtx);
		}
		
		int nRet = 0;

		if ((nRet = av_hwdevice_ctx_create(ppHwDeviceCtx, type, NULL, NULL, 0)) < 0) 
		{
			TraceLog("Failed to create specified HW device.\n");
			return nRet;
		}
		
		pCtx->hw_device_ctx = av_buffer_ref(*ppHwDeviceCtx);

        if (NULL != ppHwFramesCtx)
        {
            av_buffer_unref(ppHwFramesCtx);

            *ppHwFramesCtx = av_hwframe_ctx_alloc(*ppHwDeviceCtx);
            
            if (NULL == *ppHwFramesCtx)
            {
                TraceLog("Failed to create HW frames.\n");
                return -2;
            }

            AVHWFramesContext* pFramesCtx   = (AVHWFramesContext*)(*ppHwFramesCtx)->data;
            pFramesCtx->format              = (AVPixelFormat)g_nHwPixFmt;
            pFramesCtx->sw_format           = AV_PIX_FMT_NV12;
            pFramesCtx->width               = pCtx->width;
            pFramesCtx->height              = pCtx->height;
            pFramesCtx->initial_pool_size   = 20;

            if ((nRet = av_hwframe_ctx_init(*ppHwFramesCtx)) < 0)
            {
                TraceLog("Failed to init HW frames.\n");
    			return nRet;
            }

            pCtx->hw_frames_ctx             = av_buffer_ref(*ppHwFramesCtx);
        }

		return nRet;
	}
												
	enum AVPixelFormat FindFmtByHwType(const enum AVHWDeviceType type)
	{
		enum AVPixelFormat fmt;

		switch (type) 
		{
        case AV_HWDEVICE_TYPE_VDPAU:
			fmt = AV_PIX_FMT_VDPAU;
			break;

        case AV_HWDEVICE_TYPE_CUDA:
            fmt = AV_PIX_FMT_CUDA;
            break;

        case AV_HWDEVICE_TYPE_VAAPI:
			fmt = AV_PIX_FMT_VAAPI;
			break;
	
#ifdef _WIN32
		case AV_HWDEVICE_TYPE_DXVA2:
			fmt = AV_PIX_FMT_DXVA2_VLD;
			break;
#endif

        case AV_HWDEVICE_TYPE_QSV:
            fmt = AV_PIX_FMT_QSV;
            break;

		case AV_HWDEVICE_TYPE_VIDEOTOOLBOX:
			fmt = AV_PIX_FMT_VIDEOTOOLBOX;
			break;

#ifdef _WIN32		
		case AV_HWDEVICE_TYPE_D3D11VA:
			fmt = AV_PIX_FMT_D3D11;
			break;
#endif
		
		default:
			fmt = AV_PIX_FMT_NONE;
			break;
		}

#ifdef _WIN32
        if ( (AV_PIX_FMT_NONE != fmt) && (AV_PIX_FMT_CUDA != fmt) )
        {
            g_bCheckHwPixFmt = true;
        }

        if ( (AV_PIX_FMT_DXVA2_VLD == fmt) || (AV_PIX_FMT_D3D11 == fmt) )
        {
            g_bDecodeOnlyHwPixFmt = true;
        }
#endif

		return fmt;
	}
	
	enum AVPixelFormat GetHwFormat(AVCodecContext* pCtx, const enum AVPixelFormat* pPixFmts)
	{
		if (NULL == pCtx) 
		{
			fprintf(stderr, "Input AVCodecContext is NULL.\n");
			return AV_PIX_FMT_NONE;
		}
		
		const enum AVPixelFormat *p = NULL;

		for (p = pPixFmts; *p != -1; p++) 
		{
			if (*p == (AVPixelFormat)g_nHwPixFmt)
			{
				return *p;
			}
		}

		fprintf(stderr, "Failed to get HW surface format.\n");
		return AV_PIX_FMT_NONE;
	}
};
