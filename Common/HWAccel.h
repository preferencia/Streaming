#pragma once

#include "Common.h"

namespace HWAccel
{
	int 									InitHwCodec(AVCodecContext* pCtx, const enum AVHWDeviceType type, AVBufferRef** ppHwDeviceCtx, AVBufferRef** ppHwFramesCtx);											
	enum AVPixelFormat 	FindFmtByHwType(const enum AVHWDeviceType type);
	enum AVPixelFormat		GetHwFormat(AVCodecContext* pCtx, const enum AVPixelFormat* pPixFmts);
};
