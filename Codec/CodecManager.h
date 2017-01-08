#pragma once

#include "Codec.h"

class CCodecManager
{
public:
	CCodecManager();
	~CCodecManager();

	int 	CreateCodec(CCodec** ppCodec, int nCodecType);
	void	DestroyCodec(CCodec* pCodec);
};

