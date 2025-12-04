#include "LoginServer.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

extern char	G_cData50000[50000];
//extern void PutLogList(char* cMsg);
extern char G_cTxt[512];
extern class CGame* G_pGame;

#define WORLDNAMELS   "Apocalypse"
#define WORLDNAMELS2   "WS2"

LoginServer::LoginServer()
{

}

LoginServer::~LoginServer()
{

}

bool IsValidName(char* pStr)
{
	int i, iLen;
	iLen = strlen(pStr);
	for (i = 0; i < iLen; i++)
	{
		//if (pStr[i] < 0)	return false;
		if ((pStr[i] == ',') || (pStr[i] == '=') || (pStr[i] == ' ') || (pStr[i] == '\n') ||
			(pStr[i] == '\t') || (pStr[i] == '.') || (pStr[i] == '\\') || (pStr[i] == '/') ||
			(pStr[i] == ':') || (pStr[i] == '*') || (pStr[i] == '?') || (pStr[i] == '<') ||
			(pStr[i] == '>') || (pStr[i] == '|') || (pStr[i] == '"') || (pStr[i] == '`') ||
			(pStr[i] == ';') || (pStr[i] == '=') || (pStr[i] == '@') || (pStr[i] == '[') ||
			(pStr[i] == ']') || (pStr[i] == '^') || (pStr[i] == '_') || (pStr[i] == '\'')) return false;
		//if ((pStr[i] < '0') || (pStr[i] > 'z')) return false;
	}
	return true;
}

void LoginServer::Activated()
{
	
}

void LoginServer::RequestLogin(int h, char* pData)
{
	if (G_pGame->_lclients[h] == NULL)
		return;

	char cName[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);

	if (string(world_name) != WORLDNAMELS)
		return;

	if (!IsValidName(cName) || !IsValidName(cPassword) || !IsValidName(world_name))
		return;

	wsprintf(G_cTxt, "(!) Account Request Login: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cName, cPassword, chars);
	switch (status)
	{
	case LogIn::Ok:
	{
		char pData[512] = {};
		char* cp2 = pData;
		Push(cp2, (int)chars.size());
		GetCharList(cName, cp2, chars);
		SendLoginMsg(DEF_LOGRESMSGTYPE_CONFIRM, DEF_LOGRESMSGTYPE_CONFIRM, pData, cp2 - pData, h);
		//PutLogList("Ok");
		break;
	}

	case LogIn::NoAcc:
		SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
		PutLogList("No Acc");
		break;

	case LogIn::NoPass:
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, 0, 0, h);
		PutLogList("No Pass");
		break;
	}
}

void LoginServer::GetCharList(string acc, char*& cp2, std::vector<string> chars)
{
	for (int i = 0; i < chars.size(); i++)
	{
		char seps[] = "= \t\n";
		char cFileName[112] = {};
		char cDir[112] = {};
		mkdir("Characters");
		ZeroMemory(cFileName, sizeof(cFileName));
		strcat(cFileName, "Characters");
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		wsprintf(cDir, "AscII%d", chars[i][0]);
		strcat(cFileName, cDir);
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		strcat(cFileName, (char*)chars[i].c_str());
		strcat(cFileName, ".txt");

		DWORD lpNumberOfBytesRead;
		HANDLE  hFile = CreateFile(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
			return;
		}
		auto dwFileSize = GetFileSize(hFile, NULL);
		auto cp = new char[dwFileSize + 1];
		ZeroMemory(cp, dwFileSize + 1);
		ReadFile(hFile, cp, dwFileSize, &lpNumberOfBytesRead, NULL);
		CloseHandle(hFile);
		auto pStrTok = new class CStrTok(cp, seps);
		int cReadModeA = 0;
		char* token = pStrTok->pGet();

		short sAppr1 = 0, sAppr2 = 0, sAppr3 = 0, sAppr4 = 0;
		WORD cSex = 0, cSkin = 0;
		WORD iLevel = 0;
		DWORD iExp = 0;
		DWORD iApprColor = 0; // Antes ten�as u64, pero el cliente espera DWORD (4 bytes)
		char cMapName[11] = {}; // Siempre tama�o 11 para forzar terminador

		while (token != NULL)
		{
			if (cReadModeA != 0)
			{
				switch (cReadModeA)
				{
				case 1: sAppr1 = atoi(token); cReadModeA = 0; break;
				case 2: sAppr2 = atoi(token); cReadModeA = 0; break;
				case 3: sAppr3 = atoi(token); cReadModeA = 0; break;
				case 4: sAppr4 = atoi(token); cReadModeA = 0; break;
				case 5: cSex = atoi(token); cReadModeA = 0; break;
				case 6: cSkin = atoi(token); cReadModeA = 0; break;
				case 7: iLevel = atoi(token); cReadModeA = 0; break;
				case 14: iExp = atoi(token); cReadModeA = 0; break;
				case 15: iApprColor = (DWORD)strtoul(token, nullptr, 10); cReadModeA = 0; break;
				case 17: ZeroMemory(cMapName, sizeof(cMapName)); strncpy(cMapName, token, 10); cMapName[10] = 0; cReadModeA = 0; break;
				}
			}
			else {
				if (memcmp(token, "appr1", 5) == 0) cReadModeA = 1;
				if (memcmp(token, "appr2", 5) == 0) cReadModeA = 2;
				if (memcmp(token, "appr3", 5) == 0) cReadModeA = 3;
				if (memcmp(token, "appr4", 5) == 0) cReadModeA = 4;
				if (memcmp(token, "sex-status", 10) == 0) cReadModeA = 5;
				if (memcmp(token, "skin-status", 11) == 0) cReadModeA = 6;
				if (memcmp(token, "character-LEVEL", 15) == 0) cReadModeA = 7;
				if (memcmp(token, "character-EXP", 13) == 0) cReadModeA = 14;
				if (memcmp(token, "appr-color-new", 14) == 0) cReadModeA = 15;
				if (memcmp(token, "character-loc-map", 17) == 0) cReadModeA = 17;
			}
			token = pStrTok->pGet();
		}

		delete pStrTok;
		delete[] cp;

		// Nombre
		char cName[11] = {};
		ZeroMemory(cName, sizeof(cName));
		strncpy(cName, chars[i].c_str(), 10); // Solo 10 bytes
		cName[10] = 0;

		Push(cp2, cName, 10);          // <- SOLO 10 bytes, sin null extra
		Push(cp2, sAppr1);             // short
		Push(cp2, sAppr2);
		Push(cp2, sAppr3);
		Push(cp2, sAppr4);
		Push(cp2, cSex);               // WORD
		Push(cp2, cSkin);              // WORD
		Push(cp2, iLevel);             // WORD
		Push(cp2, iExp);               // DWORD
		Push(cp2, iApprColor);         // DWORD
		Push(cp2, cMapName, 10);       // SOLO 10 BYTES del mapname, igual que nombre
	}
}


LogIn LoginServer::AccountLogIn(string acc, string pass, std::vector<string>& chars)
{
	char file_name[255], cDir[63], cTxt[50];
	char* cp, * token, cReadModeA, cReadModeB;
	char seps[] = "= \t\n";
	class CStrTok* pStrTok;

	cReadModeA = cReadModeB = 0;

	ZeroMemory(cTxt, sizeof(cTxt));
	wsprintf(cTxt, "account-character");

	if (acc.size() == 0)
		return LogIn::NoAcc;

	if (pass.size() == 0)
		return LogIn::NoPass;

	ZeroMemory(file_name, sizeof(file_name));
	ZeroMemory(cDir, sizeof(cDir));
	strcat(file_name, "Accounts");
	strcat(file_name, "\\");
	strcat(file_name, "\\");
	wsprintf(cDir, "AscII%d", acc[0]);
	strcat(file_name, cDir);
	strcat(file_name, "\\");
	strcat(file_name, "\\");
	strcat(file_name, (char*)acc.c_str());
	strcat(file_name, ".txt");


	HANDLE file_handle = CreateFile(file_name, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	DWORD file_sz = 0;
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		CloseHandle(file_handle);
		wsprintf(G_cTxt, "(!) Account Does not Exist [invalid handle] (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	file_sz = GetFileSize(file_handle, NULL);
	CloseHandle(file_handle);

	if (file_sz == 0)
	{
		wsprintf(G_cTxt, "(!) Login File sz = 0 (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	auto file = fopen(file_name, "rt");
	if (file == NULL)
	{
		wsprintf(G_cTxt, "(!) Account Does not Exist (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	struct Acc
	{
		string name;
		string pass;
	} account;

	cp = new char[file_sz + 1];
	ZeroMemory(cp, file_sz + 1);
	fread(cp, file_sz, 1, file);
	pStrTok = new class CStrTok(cp, seps);
	token = pStrTok->pGet();
	while (token != NULL)
	{
		if (cReadModeA != 0)
		{
			switch (cReadModeA)
			{
			case 1:
				if (strlen(token) <= 0)
				{

					delete pStrTok;
					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}

				if (acc != token)
				{
					PutLogList("(!) Wrong Acc name");

					delete pStrTok;
					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}
				cReadModeA = 0;
				cReadModeA = 0;
				break;
			case 2:
				if (strlen(token) <= 0)
				{
					delete pStrTok;
					delete[] cp;
					return LogIn::NoAcc;
				}
				if (pass != token)
				{
					PutLogList("(!) Wrong pass");

					delete pStrTok;
					delete[] cp;

					fclose(file);
					return LogIn::NoPass;

				}
				cReadModeA = 0;
				break;

			case 3:
			{
				string tok = token;
				chars.push_back(tok);
				if (chars.size() > 4)
				{
					PutLogList("(!) Charlist exceeds 4 in acc cfg!");

					delete pStrTok;
					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}
				cReadModeA = 0;
				break;
			}
			}
		}
		else {
			if (memcmp(token, "account-name", 12) == 0)
				cReadModeA = 1;

			if (memcmp(token, "account-password", 16) == 0)
				cReadModeA = 2;

			if (memcmp(token, cTxt, strlen(cTxt)) == 0)
				cReadModeA = 3;
		}
		token = pStrTok->pGet();
	}

	delete pStrTok;
	delete[] cp;

	fclose(file);

	PutLogList("Account Login!");
	return LogIn::Ok;
}

void LoginServer::ResponseCharacter(int h, char* pData)
{

	char cName[11] = {};
	char cAcc[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	char gender, skin, hairstyle, haircolor, under, str, vit, dex, intl, mag, chr;

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);
	Pop(cp, gender);
	Pop(cp, skin);
	Pop(cp, hairstyle);
	Pop(cp, haircolor);
	Pop(cp, under);
	Pop(cp, str);
	Pop(cp, vit);
	Pop(cp, dex);
	Pop(cp, intl);
	Pop(cp, mag);
	Pop(cp, chr);

	if (string(world_name) != WORLDNAMELS)
		return;

	wsprintf(G_cTxt, "(!) Request create new Character: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (chars.size() >= 4)
		return;

	if (!IsValidName(cAcc) || !IsValidName(cPassword) || !IsValidName(cName))
		return;

	char seps[] = "= \t\n";
	char cFileName[112] = {};
	char cDir[112] = {};

	ZeroMemory(cFileName, sizeof(cFileName));
	strcat(cFileName, "Characters");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	wsprintf(cDir, "AscII%d", cName[0]);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cName);
	strcat(cFileName, ".txt");
	//wsprintf(G_cTxt, "(!) Getting character file %s", cFileName);
	//PutLogList(G_cTxt);

	HANDLE  hFile = CreateFile(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	auto dwFileSize = GetFileSize(hFile, NULL);

	CloseHandle(hFile);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, h);
		return;
	}

	_mkdir("Characters");
	_mkdir(string(string("Characters\\") + cDir).c_str());
	auto file = fopen(cFileName, "wt");
	if (!file)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, h);
		return;
	}

	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);

	char cBuffer[112] = {};
	char cFile[2048] = {};

	ZeroMemory(cFile, sizeof(cFile));

	strcat(cFile, "[FILE-DATE]");
	strcat(cFile, "\n\n");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "file-saved-date: %d %d %d %d %d", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n\n");

	strcat(cFile, "[NAME-ACCOUNT]");
	strcat(cFile, "\n\n");

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-name = %s\n", cName);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "account-name = %s\n", cAcc);
	strcat(cFile, cBuffer);

	strcat(cFile, "\n\n");
	strcat(cFile, "[STATUS]\n\n");

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-location = NONE\n");
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-guild-name = NONE\n");
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-guild-rank = -1\n");
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-loc-map = default\n");
	strcat(cFile, cBuffer);
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-loc-x = -1\n");
	strcat(cFile, cBuffer);
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-loc-y = -1\n");
	strcat(cFile, cBuffer);

	int hp = vit * 3 + 1 * 2 + str / 2;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-HP = %d\n", hp);
	strcat(cFile, cBuffer);

	int mp = (mag) * 2 + 1 * 2 + (intl) / 2;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-MP = %d\n", mp);
	strcat(cFile, cBuffer);

	int sp = 1 * 2 + str * 2;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-SP = %d\n", sp);
	strcat(cFile, cBuffer);


	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-DefenseRatio = %d\n", 14);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-LEVEL = 1\n");
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-STR = %d\n", str);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-INT = %d\n", intl);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-VIT = %d\n", vit);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-DEX = %d\n", dex);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-MAG = %d\n", mag);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-CHARISMA = %d\n", chr);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-LUCK = %d\n", 10);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-EXP = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-Lu_Pool = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-EK-Count = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "character-PK-Count = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "sex-status = %d\n", gender);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "skin-status = %d\n", skin);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "hairstyle-status = %d\n", hairstyle);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "haircolor-status = %d\n", haircolor);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "underwear-status = %d\n", under);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "appr1 = %d\n", 1187);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "appr2 = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "appr3 = %d\n", 0);
	strcat(cFile, cBuffer);

	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "appr4 = %d\n", 0);
	strcat(cFile, cBuffer);

	strcat(cFile, "\n\n[ITEMLIST]\n\n");

	strcat(cFile, "character-item = Dagger               1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(cFile, "character-item = BigRedPotion         1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(cFile, "character-item = BigBluePotion        1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(cFile, "character-item = BigGreenPotion       1 0 0 0 0 0 0 0 0 300 0\n");

	int iRand = (rand() % 16);
	char cTxt[120];
	switch (iRand) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 7:
	case 8:
	case 15:
		strcat(cFile, "character-item = WoodShield           1 0 0 0 0 0 0 0 0 300 0\n");
		break;

	default:
		if (gender == 1) {
			wsprintf(cTxt, "character-item = KneeTrousers(M)     1 0 0 0 0 %d 0 0 0 300 0\n", iRand);
		}
		else {
			wsprintf(cTxt, "character-item = Chemise(W)	         1 0 0 0 0 %d 0 0 0 300 0\n", iRand);
		}
		strcat(cFile, cTxt);
		strcat(cFile, "\n");
		break;
	}

	strcat(cFile, "[MAGIC-SKILL-MASTERY]\n\n");
	strcat(cFile, "magic-mastery     = 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
	strcat(cFile, "\n\n");

	strcat(cFile, "skill-mastery     = 0 0 0 3 20 24 0 24 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ");
	//	for (i = 0; i < 60; i++) {
			//ZeroMemory(cTxt, sizeof(cTxt));
			//wsprintf(cTxt, "0 0 0 3 20 24 0 24 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ");
			//strcat(pData, cTxt);
	//	}
	strcat(cFile, "\n");

	strcat(cFile, "skill-SSN     = ");
	for (int i = 0; i < 60; i++) {
		strcat(cFile, "0 ");
	}
	strcat(cFile, "\n\n");

	strcat(cFile, "[ITEM-EQUIP-STATUS]\n\n");
	strcat(cFile, "item-equip-status = 00000110000000000000000000000000000000000000000000");
	strcat(cFile, "\n\n");

	strcat(cFile, "[EOF]");
	strcat(cFile, "\n\n\n\n");

	fwrite(cFile, 1, strlen(cFile), file);
	fclose(file);

	SaveAccountInfo(0, cAcc, nullptr, cName, 1, h);

	char cData[512] = {};
	char* cp2 = cData;
	Push(cp2, cName, 10);
	chars.push_back(cName);
	Push(cp2, (int)chars.size());
	GetCharList(cAcc, cp2, chars);
	SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, cData, cp2 - cData, h);
}

#include <sstream>
void LoginServer::DeleteCharacter(int h, char* pData)
{
	//PutLogList("DeleteCharacter()");

	char cName[11] = {};
	char cAcc[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);

	wsprintf(G_cTxt, "(!) Request delete Character: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (chars.size() == 0)
		return;

	char cDir[112] = {};
	char cTxt[112] = {};
	char cFileName[112] = {};
	strcat(cFileName, "Characters");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	wsprintf(cTxt, "AscII%d", *cName);
	strcat(cFileName, cTxt);
	strcpy(cDir, cFileName);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cName);
	strcat(cFileName, ".txt");

	DeleteFile(cFileName);


	ZeroMemory(cFileName, sizeof(cFileName));
	ZeroMemory(cDir, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	wsprintf(cDir, "AscII%d", *cAcc);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, cAcc);
	strcat(cFileName, ".txt");

	ifstream in(cFileName, ios::in);
	if (!in.is_open())
	{
		PutLogList("in is_opem = false");
		return;
	}
	string wordToReplace("account-character = ");
	wordToReplace.append(cName);

	string wordToReplace2("account-character = ");
	wordToReplace2.append(cName);
	string wordToReplaceWith("");

	stringstream ss;
	bool done = false;
	string line;
	size_t len = wordToReplace.length();
	while (getline(in, line))
	{
		string repl = wordToReplace;
		size_t pos = line.find(repl);
		len = repl.length();
		if (pos == string::npos)
		{
			repl = wordToReplace2;
			pos = line.find(repl);
			len = repl.length();
		}

		if (pos != string::npos)
		{
			line.replace(pos, len, wordToReplaceWith);
			done = true;
		}

		ss << line << '\n';
	}
	in.close();

	ofstream out(cFileName);
	out << ss.str();


	if (done)
	{
		char cData[512] = {};
		char* cp2 = cData;
		for (auto it = chars.begin(); it != chars.end();)
		{
			if (cName == *it)
			{
				it = chars.erase(it);
				continue;
			}
			else ++it;
		}
		Push(cp2, (int)chars.size());
		GetCharList(cAcc, cp2, chars);
		SendLoginMsg(DEF_LOGRESMSGTYPE_CHARACTERDELETED, DEF_LOGRESMSGTYPE_CHARACTERDELETED, cData, cp2 - cData, h);
	}
	/*

		if (SaveAccountInfo(0, cAcc, nullptr, cName, 3, h) == true)
		{

		}*/
}

void LoginServer::ChangePassword(int h, char* pData)
{
	char cAcc[11] = {};
	char cPassword[11] = {};
	char cNewPw[11] = {};
	char cNewPwConf[11] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, cNewPw, 10);
	Pop(cp, cNewPwConf, 10);

	wsprintf(G_cTxt, "(!) Request change password: %s", cAcc);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (string(cNewPw) != cNewPwConf)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
		return;
	}


	char cBuffer[1024] = {};
	char cBuffer2[1024] = {};
	char cTmp[1024] = {};
	char cTxt[1024] = {};
	char cTxt2[1024] = {};
	int iTest = -1;
	wsprintf(cTmp, "Accounts\\AscII%d\\%s.txt", cAcc[0], cAcc);
	HANDLE  hFile = CreateFile(cTmp, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
		CloseHandle(hFile);
		return;
	}
	auto iSize = GetFileSize(hFile, NULL);
	CloseHandle(hFile);

	char cFileName[512] = {};
	char cDir[112] = {};
	ZeroMemory(cFileName, sizeof(cFileName));
	ZeroMemory(cDir, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	wsprintf(cDir, "AscII%d", *cAcc);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, cAcc);
	strcat(cFileName, ".txt");

	ifstream in(cFileName, ios::in);
	if (!in.is_open())
	{
		PutLogList("in is_opem = false");
		return;
	}
	string wordToReplace("account-password = ");
	wordToReplace.append(cPassword);

	string wordToReplaceWith("account-password = ");
	wordToReplaceWith.append(cNewPw);

	stringstream ss;
	bool done = false;
	string line;
	size_t len = wordToReplace.length();
	while (getline(in, line))
	{
		string repl = wordToReplace;
		size_t pos = line.find(repl);
		len = repl.length();

		if (pos != string::npos)
		{
			line.replace(pos, len, wordToReplaceWith);
			done = true;
		}

		ss << line << '\n';
	}
	in.close();

	ofstream out(cFileName);
	out << ss.str();
	if (done)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, 0, 0, h);
	}
	/*

		fpos_t pos;
		fgetpos(pFile, &pos);
		iSize = fread(cBuffer, 1, iSize, pFile);

		wsprintf(cTxt, "account-password = %s", cPassword);

		for (int i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cBuffer + i, cTxt, strlen(cTxt)) == 0)
			{
				iTest = i;
				break;
			}
		}

		if (iTest != -1)
		{
			memcpy_secure(cBuffer2, cBuffer, iTest);
			wsprintf(cTxt2, "account-password = %s", cNewPw);
			strcat(cBuffer2, cTxt2);
			memcpy_secure(cBuffer2 + iTest + strlen(cTxt2), cBuffer + iTest + strlen(cTxt), iSize - strlen(cTxt) - iTest);
			fsetpos(pFile, &pos);
			fwrite(cBuffer2, 1, iSize - strlen(cTxt) + strlen(cTxt2), pFile);
			fclose(pFile);
		}
		else
		{
			SendLoginMsg(LOGRESMSGTYPE_PASSWORDCHANGEFAIL, LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
			fclose(pFile);
			return;
		}
	*/



}

void LoginServer::CreateNewAccount(int h, char* pData)
{
	char* cp;
	FILE* pFile;
	HANDLE hFile;
	DWORD  dwFileSize;
	SYSTEMTIME SysTime;
	char cFile[20000], cBuffer[1024], cFn[1024];
	char cName[12] = {};
	char cPassword[12] = {};
	char cEmailAddr[52] = {};
	char cQuiz[47] = {};
	char cAnswer[27] = {};

	if (G_pGame->_lclients[h] == NULL)
		return;

	GetLocalTime(&SysTime);

	cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, cEmailAddr, 50);
	Pop(cp, cQuiz, 45);
	Pop(cp, cAnswer, 25);

	if ((strlen(cName) == 0) || (strlen(cPassword) == 0) ||
		(strlen(cEmailAddr) == 0) || (strlen(cQuiz) == 0) ||
		(strlen(cAnswer) == 0))
		return;

	wsprintf(G_cTxt, "(!) Request create new Account: %s", cName);
	PutLogList(G_cTxt);

	if (!IsValidName(cName) || !IsValidName(cPassword))
		return;

	ZeroMemory(cFn, sizeof(cFn));
	wsprintf(cFn, "Accounts\\AscII%d\\%s.txt", cName[0], cName);
	hFile = CreateFile(cFn, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	dwFileSize = GetFileSize(hFile, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}
	else
	{
		CloseHandle(hFile);
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, 0, 0, h);
		return;
	}

	//mkdir("DataBase");
	mkdir("Accounts");
	char Aux = 0;
	Aux = cName[0];
	ZeroMemory(cFn, sizeof(cFn));
	wsprintf(cFn, "Accounts\\AscII%d", Aux);
	_mkdir(cFn);

	ZeroMemory(cFn, sizeof(cFn));
	wsprintf(cFn, "Accounts\\AscII%d\\%s.txt", Aux, cName);
	pFile = fopen(cFn, "wt");
	if (pFile == NULL)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, 0, 0, h);
		return;
	}

	SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED, DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED, 0, 0, h);

	ZeroMemory(cFile, sizeof(cFile));
	strcat(cFile, "Account-generated: ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "Time(%d:%d/%d/%d/%d) IP(%s)", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, G_pGame->_lclients[h]->_ip);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "[Account Info]");
	strcat(cFile, "\n");

	strcat(cFile, "account-name = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	memcpy(cBuffer, cName, 10);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-password = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	memcpy(cBuffer, cPassword, 10);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Email = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "%s", cEmailAddr);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Quiz = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "%s", cQuiz);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Answer = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "%s", cAnswer);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-change-password = ");
	ZeroMemory(cBuffer, sizeof(cBuffer));
	wsprintf(cBuffer, "%d %d %d", SysTime.wYear, SysTime.wMonth, SysTime.wDay);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");
	strcat(cFile, "\n");

	strcat(cFile, "[CHARACTERS]");
	strcat(cFile, "\n");

	fwrite(cFile, 1, strlen(cFile), pFile);
	fclose(pFile);
}


bool LoginServer::SaveAccountInfo(int iAccount, char* cAccountName, char* cTemp, char* cCharName, char cMode, int h)
{
	char* g_txt = &G_cTxt[0];

	char cFileName[255], cDir[63], cTxt[50], cTxt2[2000], cData[2000];
	int iLine, i;
	int    iSize;
	short iMinus;
	int    iCharPos = -1;
	int    iTest = -1;
	bool bDeleteLine;
	HANDLE hFile;
	fpos_t pos;
	DWORD  dwSize = 0;
	DWORD dwFileSize;
	FILE* pFile;

	memset(cData, 0, 2000);
	memset(cTxt2, 0, 2000);
	dwFileSize = 0;
	iLine = 0;
	bDeleteLine = false;
	iMinus = 0;
	ZeroMemory(cTxt, sizeof(cTxt));
	ZeroMemory(cTxt2, sizeof(cTxt2));
	ZeroMemory(cData, sizeof(cData));

	ZeroMemory(cFileName, sizeof(cFileName));
	ZeroMemory(cDir, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	wsprintf(cDir, "AscII%d", *cAccountName);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cAccountName);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	iSize = GetFileSize(hFile, NULL);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	//char cTxt3[112] = {};

	switch (cMode) {
	case 1: //save new character
		pFile = fopen(cFileName, "at");
		if (pFile == NULL) {
			wsprintf(g_txt, "(X) Account none exist: Name(%s)", cAccountName);
			PutLogList(g_txt);
			return false;
		}
		wsprintf(cTxt, "\naccount-character = %s", cCharName);
		fwrite(cTxt, 1, strlen(cTxt), pFile);
		fclose(pFile);
		break;

	case 2: //password change
		wsprintf(g_txt, "(X) PasswordChange(%s)", cTemp);
		PutLogList(g_txt);
		wsprintf(cTxt, "account-password = %s", cTemp);
		pFile = fopen(cFileName, "rt");
		if (pFile == NULL) {
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
			return false;
		}
		fgetpos(pFile, &pos);
		iSize = fread(cData, 1, iSize, pFile);
		fread(cData, dwFileSize, 1, pFile);
		for (i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cData + i, "[CHARACTERS]", 12) == 0)
			{
				iCharPos = i;
			}
			if (memcmp((char*)cData + i, "account-password = ", 19) == 0)
			{
				iTest = i;
			}
		}
		if (iTest == -1)
		{
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
		}
		else
		{
			memcpy(cTxt2, cData, iTest + 19);
			memcpy(cTxt2 + iTest + 19, cTxt, strlen(cTxt));
			memcpy(cTxt2 + iTest + 19 + strlen(cTxt), cData + iTest + 19 + strlen(cTemp), iSize - 19 - iTest - strlen(cTemp));
			SaveInfo(cFileName, cTxt2, 1);
		}
		fclose(pFile);
		break;

	case 3: //delete character
		wsprintf(g_txt, "(X) Character Delete(%s)", cCharName);
		PutLogList(g_txt);
		wsprintf(cTxt, "account-character = %s", cCharName);
		//wsprintf(cTxt3, "account-character = %s", cCharName);
		pFile = fopen(cFileName, "rt");
		if (pFile == NULL) {
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
			return false;
		}
		fgetpos(pFile, &pos);
		iSize = fread(cData, 1, iSize, pFile);
		fread(cData, dwFileSize, 1, pFile);
		for (int i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cData + i, "[CHARACTERS]", 12) == 0)
			{
				iCharPos = i;
			}
			if (memcmp((char*)cData + i, cTxt, strlen(cTxt)) == 0)
			{
				iTest = i;
			}
			/*if (memcmp((char*)cData + i, cTxt3, strlen(cTxt3)) == 0)
			{
				iTest = i;
			}*/
		}
		if (iTest == -1)
		{
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, h);
		}
		else
		{
			memcpy(cTxt2, cData, iTest - 1);
			memcpy(cTxt2 + iTest - 1, cData + iTest + strlen(cTxt), iSize - strlen(cTxt) - iTest);
			SaveInfo(cFileName, cTxt2, 1);
		}
		fclose(pFile);
		break;
	}
	return false;
}
void LoginServer::SaveInfo(char cFileName[255], char* pData, DWORD dwStartSize)
{
	FILE* pFile;

	pFile = fopen(cFileName, "wt");
	if (pFile != NULL)
	{
		if (sizeof(pData) > 0) fwrite(pData, dwStartSize, strlen(pData), pFile);
		fclose(pFile);
	}
}

void LoginServer::SendLoginMsg(DWORD msg_id, WORD msg_type, char* data, int sz, int h)
{

	int iRet;
	DWORD* dwp;
	char* cp;
	WORD* wp;
	int index = h;

	if (!G_pGame->_lclients[h])
		return;

	ZeroMemory(G_cData50000, sizeof(G_cData50000));

	dwp = (DWORD*)(G_cData50000 + DEF_INDEX4_MSGID);
	*dwp = msg_id;
	wp = (WORD*)(G_cData50000 + DEF_INDEX2_MSGTYPE);
	*wp = msg_type;

	cp = (char*)(G_cData50000 + DEF_INDEX2_MSGTYPE + 2);

	memcpy((char*)cp, data, sz);

	//if is registered
	if (true)
	{
		switch (msg_id) {
		default:
			iRet = G_pGame->_lclients[index]->_sock->iSendMsg(G_cData50000, sz + 6);
			break;
		}

		switch (iRet)
		{
		case DEF_XSOCKEVENT_QUENEFULL:
		case DEF_XSOCKEVENT_SOCKETERROR:
		case DEF_XSOCKEVENT_CRITICALERROR:
		case DEF_XSOCKEVENT_SOCKETCLOSED:
			wsprintf(G_cTxt, "(!) Login Connection Lost on Send (%d)", index);
			PutLogList(G_cTxt);
			delete G_pGame->_lclients[index];
			G_pGame->_lclients[index] = NULL;
			return;
		}
	}
}

void LoginServer::RequestEnterGame(int h, char* pData)
{
	//	PutLogList("RequestEnterGame()");

	char cName[11] = {};
	char cMapName[11] = {};
	char cAcc[11] = {};
	char cPass[11] = {};
	int lvl;
	char ws_name[31] = {};
	char cmd_line[121] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cMapName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPass, 10);
	Pop(cp, lvl);
	Pop(cp, ws_name, 10);
	Pop(cp, cmd_line, 120);

	char cData[112] = {};
	char* cp2 = cData;
	if (string(ws_name) != WORLDNAMELS)
	{
		//SendEventToWLS(ENTERGAMERESTYPE_REJECT, ENTERGAMERESTYPE_REJECT, 0, 0, h);
		return;
	}

	wsprintf(G_cTxt, "(!) Request enter Game: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPass, chars);
	if (status != LogIn::Ok)
		return;

	for (int i = 0; i < DEF_MAXCLIENTS; i++)
	{
		if (!G_pGame->m_pClientList[i])
			continue;

		if (string(G_pGame->m_pClientList[i]->m_cAccountName) == cAcc)
		{
			G_pGame->DeleteClient(i, true, true);
			break;
		}
	}

	cp2 = (char*)cData; //outgoing messag - to Client

	memcpy(cp2, (char*)G_pGame->m_cGameServerAddr, 16);
	cp2 += 16;

	auto wp = (WORD*)cp2;
	*wp = (WORD)G_pGame->m_iLogServerPort;
	cp2 += 2;

	char sv_name[21] = {};
	wsprintf(sv_name, "%s", WORLDNAMELS2);
	memcpy(cp2, sv_name, 20);
	cp2 += 20;

	PutLogList("Send DEF_ENTERGAMERESTYPE_CONFIRM");
	SendLoginMsg(DEF_ENTERGAMERESTYPE_CONFIRM, DEF_ENTERGAMERESTYPE_CONFIRM, cData, 38, h);
}


void LoginServer::LocalSavePlayerData(int h)
{
	char* pData, * cp, cFn[256], cDir[256], cTxt[256], cCharDir[256];
	int    iSize;
	FILE* pFile;
	SYSTEMTIME SysTime;


	if (G_pGame->m_pClientList[h] == NULL) return;

	pData = new char[30000];
	ZeroMemory(pData, 30000);

	cp = (char*)(pData);
	iSize = G_pGame->_iComposePlayerDataFileContents(h, cp);

	GetLocalTime(&SysTime);
	ZeroMemory(cCharDir, sizeof(cDir));
	wsprintf(cCharDir, "Characters");
	ZeroMemory(cDir, sizeof(cDir));
	ZeroMemory(cFn, sizeof(cFn));
	strcat(cFn, cCharDir);
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	wsprintf(cTxt, "AscII%d", (unsigned char)G_pGame->m_pClientList[h]->m_cCharName[0]);
	strcat(cFn, cTxt);
	strcpy(cDir, cFn);
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	strcat(cFn, G_pGame->m_pClientList[h]->m_cCharName);
	strcat(cFn, ".txt");


	_mkdir(cCharDir);
	_mkdir(cDir);


	if (iSize == 0) {
		//	PutLogList("(!) Character data body empty: Cannot create & save temporal player data file.");
		delete[]pData;
		return;
	}

	pFile = fopen(cFn, "wt");
	if (pFile != NULL) {
		fwrite(cp, iSize, 1, pFile);
		fclose(pFile);
	}

	delete[]pData;
}