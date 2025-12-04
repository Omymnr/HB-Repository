#pragma once
#include <windows.h>

class CEnchanting
{
public:
	CEnchanting();
	virtual ~CEnchanting();

	char cName[21], cDesc[11];
	int iCount;
	DWORD dwType, dwValue;
};

