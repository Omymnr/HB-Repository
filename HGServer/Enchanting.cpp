#include "Enchanting.h"

CEnchanting::CEnchanting()
{
	ZeroMemory(cName, sizeof(cName));
	ZeroMemory(cDesc, sizeof(cDesc));
	iCount = NULL;
	dwType = NULL;
	dwValue = NULL;
}

CEnchanting::~CEnchanting()
{

}