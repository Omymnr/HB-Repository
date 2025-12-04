#include "MobCounter.h"

CMobCounter::CMobCounter(char* cName)
{
	strcpy(cNpcName, cName);
	iKillCount = 0;
	iLevel = 0;
	iNextCount = 100;
}

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