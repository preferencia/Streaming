#pragma once

#include "Codec.h"
#include "Mux.h"

class CObjectManager
{
public:
	CObjectManager();
	~CObjectManager();

	int 	CreateObject(void** ppOjbect, int nObjectType);
	void	DestroyObject(void** ppOjbect, int nObjectType);
};

