#include "MobCounter.h"
CMobCounter::CMobCounter()
{
	ZeroMemory(cNpcName, sizeof(cNpcName));
	iKillCount = NULL;
	iLevel = NULL;
	iNextCount = NULL;
}


CMobCounter::~CMobCounter()
{
}