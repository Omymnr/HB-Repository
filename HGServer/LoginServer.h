#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
using namespace std;

#include "Game.h"

enum class LogIn
{
	Ok,
	NoAcc,
	NoPass,
};

class LoginServer
{
public:
	LoginServer();
	~LoginServer();

	void RequestLogin(int h, char* pData);
	void GetCharList(string acc, char*& cp2, std::vector<string> chars);
	LogIn AccountLogIn(string name, string pass, std::vector<string>& chars);
	void ResponseCharacter(int h, char* pData);
	void DeleteCharacter(int h, char* pData);
	void ChangePassword(int h, char* pData);
	void RequestEnterGame(int h, char* pData);
	void CreateNewAccount(int h, char* pData);
	bool SaveAccountInfo(int iAccount, char* cAccountName, char* cTemp, char* cCharName, char cMode, int h);
	void SaveInfo(char cFileName[255], char* pData, DWORD dwStartSize);
	void SendLoginMsg(DWORD msgid, WORD msgtype, char* data, int sz, int h);
	void LocalSavePlayerData(int h);
	void Activated();
};

extern LoginServer* g_login;
