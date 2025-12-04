// -------------------------------------------------------------- 
//                      New Game Server  						  
//
//                      1998.11 by Soph
//
// --------------------------------------------------------------







// --------------------------------------------------------------


#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <mmsystem.h>
#include <time.h>
#include <richedit.h>
#include <commctrl.h>
#include <direct.h>
#include "winmain.h"
#include "Game.h"
#include "UserMessages.h"
#include "NetMessages.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

// Declaracion externa del ListBox de mapas
extern HWND G_hListMaps;

void PutAdminLogFileList(char * cStr);
void PutHackLogFileList(char * cStr);
void PutPvPLogFileList(char * cStr);

// Prototipos para funciones de logs con colores
void AppendColoredText(HWND hRichEdit, const char* text, COLORREF color);
COLORREF GetLogColor(const char* cMsg);
BOOL IsErrorOrWarning(const char* cMsg);
BOOL CALLBACK AccountCreatorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500

char			szAppClass[32];
HWND			G_hWnd = NULL;
char			G_cMsgList[120*500];
BOOL            G_cMsgUpdated =	FALSE;
char            G_cTxt[512];
char			G_cData50000[50000];
MMRESULT        G_mmTimer = NULL;

// Variables para la consola de comandos GM
char			G_cConsoleInput[256];
int				G_iConsoleInputPos = 0;
HWND			G_hEditConsole = NULL;
HWND			G_hBtnShutdown = NULL;  // Botón de Shutdown
HWND			G_hBtnWarn2 = NULL;     // Botón de aviso shutdown (con timer)
DWORD			G_dwShutdownTimer = 0;  // Tiempo de inicio del shutdown timer
BOOL			G_bShutdownPending = FALSE; // Si hay shutdown pendiente
int				G_iLastMinuteWarned = 6;    // Último minuto en que se envió aviso (empieza en 6 para avisar en 5)
HWND			G_hEditLogs = NULL;     // Control de logs (RichEdit con colores)
HWND			G_hBtnSaveLogs = NULL;  // Botón para guardar logs
HWND			G_hChkErrorsOnly = NULL; // Checkbox para filtrar solo errores
HWND			G_hRichEditLogs = NULL; // RichEdit para logs con colores
HWND			G_hBtnAccounts = NULL;  // Botón para abrir carpeta Accounts
HWND			G_hBtnCharacters = NULL; // Botón para abrir carpeta Characters
HWND			G_hBtnAccountCreator = NULL; // Botón para crear cuentas
HWND			G_hListMaps = NULL;     // ListBox para mapas con scroll
WNDPROC			G_lpfnEditProc = NULL;  // Procedimiento original del EDIT
HINSTANCE		G_hRichEditLib = NULL;  // Handle para RichEdit DLL

// Critical section for thread-safe logging
CRITICAL_SECTION G_csLogLock;
BOOL			G_bLogLockInitialized = FALSE;

// Historial de comandos de la consola
#define MAX_CMD_HISTORY 20
char			G_cCmdHistory[MAX_CMD_HISTORY][256];
int				G_iCmdHistoryCount = 0;
int				G_iCmdHistoryIndex = -1;

// Lista de comandos para autocompletado
const char* G_cConsoleCommands[] = {
	"help", "online", "kick", "ban", "teleport", "heal", "kill",
	"setlevel", "setgold", "setskill", "setstat", "giveitem",
	"spawn", "npcs", "maps", "items", "finditem", "announce",
	"weather", "time", "save", "shutdown", NULL
};

// Variables para estadísticas
DWORD			G_dwLastMsgCount = 0;
DWORD			G_dwMsgsPerSecond = 0;
DWORD			G_dwLastStatTime = 0;

// Filtro de logs
BOOL			G_bErrorsOnlyFilter = FALSE;

// Historial de conexiones (últimas 5) con más info
#define MAX_LAST_CONNECTIONS 5
typedef struct {
	char cPlayerName[20];
	char cIP[20];
	char cTime[12];
	char cAction[12];  // "Connected" o "Disconnected"
} ConnectionInfo;
ConnectionInfo G_LastConnections[MAX_LAST_CONNECTIONS];
int				G_iLastConnIndex = 0;

// Para compatibilidad con código existente
char			G_cLastConnections[5][80];


class XSocket * G_pListenSock = NULL;
class XSocket * G_pLogSock    = NULL;
class CGame *   G_pGame       = NULL;
class XSocket* G_pLoginSock = NULL;
class LoginServer* g_login;

int             G_iQuitProgramCount = 0;
BOOL			G_bIsThread = TRUE;

FILE * pLogFile;

//char			G_cCrashTxt[50000];
// --------------------------------------------------------------

unsigned __stdcall ThreadProc(void* ch)
{
	class CTile* pTile;
	while (G_bIsThread)
	{
		Sleep(1000); 

		for (int a = 0; a < DEF_MAXMAPS; a++)
		{
			if (G_pGame->m_pMapList[a] != NULL)
			{
				for (int iy = 0; iy < G_pGame->m_pMapList[a]->m_sSizeY; iy++)
				{
					for (int ix = 0; ix < G_pGame->m_pMapList[a]->m_sSizeX; ix++)
					{
						pTile = (class CTile*)(G_pGame->m_pMapList[a]->m_pTile + ix + iy * G_pGame->m_pMapList[a]->m_sSizeY);
						if ((pTile != NULL) && (pTile->m_sOwner != NULL) && (pTile->m_cOwnerClass == NULL))
						{
							pTile->m_sOwner = NULL;
						}
					}
				}
			}
		}
	}

	_endthread();
	return NULL;
}

// Subclass procedure para el control EDIT - captura Enter, flechas y TAB
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			char cCmd[256];
			GetWindowText(hWnd, cCmd, 256);
			if (strlen(cCmd) > 0 && G_pGame != NULL) {
				// Guardar comando en el historial
				if (G_iCmdHistoryCount < MAX_CMD_HISTORY) {
					// Mover todos los comandos hacia abajo
					for (int i = G_iCmdHistoryCount; i > 0; i--) {
						strcpy(G_cCmdHistory[i], G_cCmdHistory[i-1]);
					}
					G_iCmdHistoryCount++;
				} else {
					// Historial lleno, descartar el más antiguo
					for (int i = MAX_CMD_HISTORY - 1; i > 0; i--) {
						strcpy(G_cCmdHistory[i], G_cCmdHistory[i-1]);
					}
				}
				strcpy(G_cCmdHistory[0], cCmd);
				G_iCmdHistoryIndex = -1; // Reset del índice
				
				G_pGame->ProcessConsoleCommand(cCmd);
				SetWindowText(hWnd, "");
			}
			return 0;
		}
		// TAB - Autocompletado de comandos
		else if (wParam == VK_TAB) {
			char cCurrentText[256];
			GetWindowText(hWnd, cCurrentText, 256);
			int iLen = strlen(cCurrentText);
			
			if (iLen > 0) {
				// Buscar comando que coincida
				for (int i = 0; G_cConsoleCommands[i] != NULL; i++) {
					if (_strnicmp(cCurrentText, G_cConsoleCommands[i], iLen) == 0) {
						// Encontrado - autocompletar
						SetWindowText(hWnd, G_cConsoleCommands[i]);
						// Mover cursor al final
						int newLen = strlen(G_cConsoleCommands[i]);
						SendMessage(hWnd, EM_SETSEL, newLen, newLen);
						break;
					}
				}
			}
			return 0;
		}
		// Flecha Arriba - Comando anterior en historial
		else if (wParam == VK_UP) {
			if (G_iCmdHistoryCount > 0) {
				if (G_iCmdHistoryIndex < G_iCmdHistoryCount - 1) {
					G_iCmdHistoryIndex++;
				}
				SetWindowText(hWnd, G_cCmdHistory[G_iCmdHistoryIndex]);
				// Mover cursor al final del texto
				SendMessage(hWnd, EM_SETSEL, (WPARAM)strlen(G_cCmdHistory[G_iCmdHistoryIndex]), (LPARAM)strlen(G_cCmdHistory[G_iCmdHistoryIndex]));
			}
			return 0;
		}
		// Flecha Abajo - Comando más reciente en historial
		else if (wParam == VK_DOWN) {
			if (G_iCmdHistoryIndex > 0) {
				G_iCmdHistoryIndex--;
				SetWindowText(hWnd, G_cCmdHistory[G_iCmdHistoryIndex]);
				SendMessage(hWnd, EM_SETSEL, (WPARAM)strlen(G_cCmdHistory[G_iCmdHistoryIndex]), (LPARAM)strlen(G_cCmdHistory[G_iCmdHistoryIndex]));
			} else if (G_iCmdHistoryIndex == 0) {
				G_iCmdHistoryIndex = -1;
				SetWindowText(hWnd, "");
			}
			return 0;
		}
		break;
	case WM_CHAR:
		// Evitar el beep al presionar Enter o TAB
		if (wParam == VK_RETURN || wParam == VK_TAB) {
			return 0;
		}
		break;
	}
	// Llamar al procedimiento original del EDIT
	return CallWindowProc(G_lpfnEditProc, hWnd, message, wParam, lParam);
}

LRESULT CALLBACK WndProc( HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam )
{ 
	switch (message) {
	case WM_CREATE:
		break;
	
	case WM_ERASEBKGND:
		// Evitar el borrado del fondo para eliminar parpadeo
		// El fondo se dibuja en OnPaint con doble buffer
		return 1;
	
	case WM_SIZE:
		// Reposicionar los controles cuando cambia el tamaño de la ventana
		{
			RECT rc;
			GetClientRect(hWnd, &rc);
			int winWidth = rc.right;
			int winHeight = rc.bottom;
			int panelWidth = winWidth - 10;
			
			// Calcular posiciones dinámicas
			int logsTop = 145;
			int gmPanelHeight = 35;
			int gmPanelTop = winHeight - gmPanelHeight - 5;
			int availableHeight = gmPanelTop - logsTop - 10;
			int logsHeight = availableHeight * 55 / 100;  // 55% para logs
			int mapsTop = logsTop + logsHeight + 5;
			
			// Asegurar mínimos
			if (logsHeight < 80) logsHeight = 80;
			if (gmPanelTop < 250) gmPanelTop = winHeight - 40;
			
			// Posicionar RichEdit de logs (dentro del panel, debajo del título y botones)
			// Reducir ancho para dejar espacio al panel PLAYERS ONLINE (320px)
			int logPanelWidth = panelWidth - 330;  // Dejar 330px para PLAYERS ONLINE
			if (logPanelWidth < 300) logPanelWidth = 300;  // Mínimo 300px
			if (G_hRichEditLogs != NULL) {
				int logEditTop = logsTop + 22;  // Espacio para título y botones
				int logEditHeight = logsHeight - 27;
				if (logEditHeight < 50) logEditHeight = 50;
				MoveWindow(G_hRichEditLogs, 10, logEditTop, logPanelWidth, logEditHeight, TRUE);
			}
			
			// Botón Save Logs (arriba a la derecha del panel de logs, antes de PLAYERS ONLINE)
			if (G_hBtnSaveLogs != NULL) {
				MoveWindow(G_hBtnSaveLogs, logPanelWidth - 80, logsTop + 1, 85, 18, TRUE);
			}
			
			// Checkbox Errors Only (al lado del botón Save)
			if (G_hChkErrorsOnly != NULL) {
				MoveWindow(G_hChkErrorsOnly, logPanelWidth - 190, logsTop + 2, 100, 16, TRUE);
			}
			
			// Posicionar campo de comando GM (ahora ocupa todo el ancho menos margen)
			if (G_hEditConsole != NULL) {
				int editWidth = winWidth - 100; 
				if (editWidth < 200) editWidth = 200;
				MoveWindow(G_hEditConsole, 85, gmPanelTop + 6, editWidth, 22, TRUE);
			}
			
			// === POSICIONAR BOTONES EN EL HUECO DEBAJO DE PLAYERS ONLINE ===
			// El panel de Players Online termina aprox en Y=450
			int buttonsY = 460;
			int rightColX = winWidth - 315; // Inicio columna derecha
			
			// Fila 1: Accounts y Characters
			if (G_hBtnAccounts != NULL) {
				MoveWindow(G_hBtnAccounts, rightColX + 60, buttonsY, 80, 26, TRUE);
			}
			if (G_hBtnCharacters != NULL) {
				MoveWindow(G_hBtnCharacters, rightColX + 150, buttonsY, 90, 26, TRUE);
			}
			
			// Fila 2: Warn y Shutdown
			if (G_hBtnWarn2 != NULL) {
				MoveWindow(G_hBtnWarn2, rightColX + 50, buttonsY + 30, 100, 26, TRUE);
			}
			if (G_hBtnShutdown != NULL) {
				MoveWindow(G_hBtnShutdown, rightColX + 160, buttonsY + 30, 95, 26, TRUE);
			}

			// Fila 3: Account Creator
			if (G_hBtnAccountCreator != NULL) {
				MoveWindow(G_hBtnAccountCreator, rightColX + 100, buttonsY + 60, 110, 26, TRUE);
			}
			
			// Actualizar ancho de mapas para DisplayInfo
			if (G_pGame != NULL) {
				G_pGame->m_iDisplayMapsWidth = logPanelWidth;
			}
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	
	case WM_COMMAND:
		// Botón Warn (shutdown en 5 minutos)
		if (LOWORD(wParam) == 1007) {
			int iResult = MessageBox(hWnd, "Are you sure you want to initiate a 5-MINUTE SHUTDOWN warning?", 
				"Shutdown Warning Confirmation", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

			if (iResult == IDYES && G_pGame != NULL && G_bShutdownPending == FALSE) {
				int iCount = 0;
				for (int i = 1; i < DEF_MAXCLIENTS; i++) {
					if (G_pGame->m_pClientList[i] != NULL && G_pGame->m_pClientList[i]->m_bIsInitComplete == TRUE) {
						// Enviar aviso inicial con 5 minutos
						G_pGame->SendNotifyMsg(NULL, i, DEF_NOTIFY_SERVERSHUTDOWN, 1, 5, NULL, NULL);
						iCount++;
					}
				}
				// Iniciar timer de 5 minutos
				G_dwShutdownTimer = timeGetTime();
				G_bShutdownPending = TRUE;
				G_iLastMinuteWarned = 5; // Ya avisamos de 5 minutos
				char cLogMsg[128];
				wsprintf(cLogMsg, "(!) SHUTDOWN WARNING: 5 minutes - Sent to %d players", iCount);
				PutLogList(cLogMsg);
				// Deshabilitar botón para evitar doble click
				EnableWindow(G_hBtnWarn2, FALSE);
				SetWindowText(G_hBtnWarn2, "4:59...");
			}
		}
		// Botón Shutdown
		else if (LOWORD(wParam) == 1002) {
			int iResult = MessageBox(hWnd, "Are you sure you want to SHUTDOWN the server?\n\nAll players will be disconnected!", 
				"Server Shutdown Confirmation", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
			if (iResult == IDYES) {
				PutLogList("(!) SERVER SHUTDOWN initiated by admin button...");
				if (G_pGame != NULL) {
					G_pGame->m_bF1pressed = TRUE;
					G_pGame->m_bF4pressed = TRUE;
				}
			}
		}
		// Botón Save Logs
		else if (LOWORD(wParam) == 1004) {
			// Guardar logs a archivo
			SYSTEMTIME st;
			GetLocalTime(&st);
			char cFileName[128];
			wsprintf(cFileName, "ServerLogs_%04d%02d%02d_%02d%02d%02d.txt",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			
			FILE* fp = fopen(cFileName, "w");
			if (fp != NULL) {
				int iLen = GetWindowTextLength(G_hRichEditLogs);
				char* pBuffer = (char*)malloc(iLen + 1);
				if (pBuffer != NULL) {
					GetWindowText(G_hRichEditLogs, pBuffer, iLen + 1);
					fprintf(fp, "%s", pBuffer);
					free(pBuffer);
				}
				fclose(fp);
				
				char cMsg[256];
				wsprintf(cMsg, "Logs saved to: %s", cFileName);
				PutLogList(cMsg);
				MessageBox(hWnd, cMsg, "Logs Saved", MB_OK | MB_ICONINFORMATION);
			}
		}
		// Checkbox Errors Only
		else if (LOWORD(wParam) == 1005) {
			G_bErrorsOnlyFilter = (SendMessage(G_hChkErrorsOnly, BM_GETCHECK, 0, 0) == BST_CHECKED);
			if (G_bErrorsOnlyFilter) {
				PutLogList("(i) Filter: Showing errors and warnings only");
			} else {
				PutLogList("(i) Filter: Showing all messages");
			}
		}
		// Botón Accounts
		else if (LOWORD(wParam) == 1008) {
			ShellExecute(NULL, "explore", "..\\Files\\Accounts", NULL, NULL, SW_SHOWNORMAL);
		}
		// Botón Characters
		else if (LOWORD(wParam) == 1009) {
			ShellExecute(NULL, "explore", "..\\Files\\Characters", NULL, NULL, SW_SHOWNORMAL);
		}
		// Botón Account Creator
		else if (LOWORD(wParam) == IDC_BTN_ACC_CREATOR) {
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ACCOUNT_CREATOR), hWnd, (DLGPROC)AccountCreatorDlgProc);
		}
		break;

	case WM_KEYDOWN:
		G_pGame->OnKeyDown(wParam, lParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_KEYUP:
		G_pGame->OnKeyUp(wParam, lParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;
	
	case WM_USER_STARTGAMESIGNAL:
		G_pGame->OnStartGameSignal();
		break;
	
	case WM_USER_TIMERSIGNAL:
		G_pGame->OnTimer(NULL);
		break;

	case WM_USER_ACCEPT:
		OnAccept();
		break;

	case WM_USER_ACCEPT_LOGIN:
		OnAcceptLogin();
		break;

	//case WM_KEYUP:
	//	OnKeyUp(wParam, lParam);
	//	break;

	case WM_PAINT:
		OnPaint();
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_CLOSE:
		if (G_pGame->bOnClose() == TRUE) return (DefWindowProc(hWnd, message, wParam, lParam));;
		//G_iQuitProgramCount++;
		//if (G_iQuitProgramCount >= 2) {
		//	return (DefWindowProc(hWnd, message, wParam, lParam));
		//}
		break;

	case WM_ONGATESOCKETEVENT:
		G_pGame->OnGateSocketEvent(message, wParam, lParam);
		break;

	case WM_ONLOGSOCKETEVENT:
		G_pGame->OnMainLogSocketEvent(message, wParam, lParam);
		break;
	
	default: 
		/*if ((message >= WM_ONLOGSOCKETEVENT + 1) && (message <= WM_ONLOGSOCKETEVENT + DEF_MAXSUBLOGSOCK))
			G_pGame->OnSubLogSocketEvent(message, wParam, lParam);*/

		if ((message >= WM_USER_BOT_ACCEPT + 1) && (message <= WM_USER_BOT_ACCEPT + DEF_MAXCLIENTLOGINSOCK))
			G_pGame->OnSubLogSocketEvent(message, wParam, lParam);
		
		if ((message >= WM_ONCLIENTSOCKETEVENT) && (message < WM_ONCLIENTSOCKETEVENT + DEF_MAXCLIENTS)) 
			G_pGame->OnClientSocketEvent(message, wParam, lParam);

		return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	
	return NULL;
}

/*void GetOSName(){
	OSVERSIONINFOEX osvi;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
			return;
	}

	//OS Info
	strcat(G_cCrashTxt, "System Information\r\n");
	strcat(G_cCrashTxt, "Operating System : ");

	switch (osvi.dwPlatformId)
	{
		// Test for the Windows NT product family.
	case VER_PLATFORM_WIN32_NT:

		// Test for the specific product family.
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			strcat(G_cCrashTxt,"Microsoft Windows Server 2003 family, ");

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			strcat(G_cCrashTxt,"Microsoft Windows XP ");

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			strcat(G_cCrashTxt,"Microsoft Windows 2000 ");

		if ( osvi.dwMajorVersion <= 4 )
			strcat(G_cCrashTxt,"Microsoft Windows NT ");

		// Test for specific product on Windows NT 4.0 SP6 and later.
		if( bOsVersionInfoEx )
		{
			// Test for the workstation type.
			if ( osvi.wProductType == VER_NT_WORKSTATION )
			{
				if( osvi.dwMajorVersion == 4 )
					strcat(G_cCrashTxt, "Workstation 4.0 " );
				else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
					strcat(G_cCrashTxt, "Home Edition " );
				else
					strcat(G_cCrashTxt, "Professional " );
			}

			// Test for the server type.
			else if ( osvi.wProductType == VER_NT_SERVER || 
				osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
			{
				if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strcat(G_cCrashTxt, "Datacenter Edition " );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strcat(G_cCrashTxt, "Enterprise Edition " );
					else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
						strcat(G_cCrashTxt, "Web Edition " );
					else
						strcat(G_cCrashTxt, "Standard Edition " );
				}

				else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
				{
					if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
						strcat(G_cCrashTxt, "Datacenter Server " );
					else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strcat(G_cCrashTxt, "Advanced Server " );
					else
						strcat(G_cCrashTxt, "Server " );
				}

				else  // Windows NT 4.0 
				{
					if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
						strcat(G_cCrashTxt,"Server 4.0, Enterprise Edition " );
					else
						strcat(G_cCrashTxt, "Server 4.0 " );
				}
			}
		}
		else  // Test for specific product on Windows NT 4.0 SP5 and earlier
		{
			#define BUFSIZE 80
			HKEY hKey;
			char szProductType[BUFSIZE];
			DWORD dwBufLen=BUFSIZE;
			LONG lRet;

			lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
				0, KEY_QUERY_VALUE, &hKey );
			if( lRet != ERROR_SUCCESS )
				return;

			lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
				(LPBYTE) szProductType, &dwBufLen);
			if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
				return;

			RegCloseKey( hKey );

			if ( lstrcmpi( "WINNT", szProductType) == 0 )
				printf( "Workstation " );
			if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
				printf( "Server " );
			if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
				printf( "Advanced Server " );

			wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion );
		}

		// Display service pack (if any) and build number.

		if( osvi.dwMajorVersion == 4 && 
			lstrcmpi( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
		{
			HKEY hKey;
			LONG lRet;

			// Test for SP6 versus SP6a.
			lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
				"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
				0, KEY_QUERY_VALUE, &hKey );
			if( lRet == ERROR_SUCCESS )
				wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "Service Pack 6a (Build %d)\r\n", osvi.dwBuildNumber & 0xFFFF );         
			else // Windows NT 4.0 prior to SP6a
			{
				wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "%s (Build %d)\r\n",
					osvi.szCSDVersion,
					osvi.dwBuildNumber & 0xFFFF);
			}

			RegCloseKey( hKey );
		}
		else // Windows NT 3.51 and earlier or Windows 2000 and later
		{
			wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "%s (Build %d)\r\n",
				osvi.szCSDVersion,
				osvi.dwBuildNumber & 0xFFFF);
		}


		break;

		// Test for the Windows 95 product family.
	case VER_PLATFORM_WIN32_WINDOWS:

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			strcat(G_cCrashTxt,"Microsoft Windows 95 ");
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
				strcat(G_cCrashTxt,"OSR2 " );
		} 

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			strcat(G_cCrashTxt,"Microsoft Windows 98 ");
			if ( osvi.szCSDVersion[1] == 'A' )
				strcat(G_cCrashTxt,"SE " );
		} 

		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
		{
			strcat(G_cCrashTxt,"Microsoft Windows Millennium Edition\r\n");
		} 
		break;

	case VER_PLATFORM_WIN32s:

		strcat(G_cCrashTxt,"Microsoft Win32s\r\n");
		break;
	}
}*/

/*BOOL CALLBACK lpCrashDialogFunc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam){
HANDLE outHand;
char cCrashFileName[MAX_PATH];
char cLF[]={0x0d,0x0a};
char cDash ='-';
SYSTEMTIME sysTime;
DWORD written;

	switch(uMsg) {
	case WM_CLOSE:
		EndDialog(hDlg, TRUE);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_CLOSE:
			EndDialog(hDlg, TRUE);
			break;
		}
		break;

	case WM_INITDIALOG:
		//Show Crash Data
		SetWindowText(GetDlgItem(hDlg, IDC_EDIT1), G_cCrashTxt);
		GetLocalTime(&sysTime);
		wsprintf(cCrashFileName,"CrashData - %d-%d-%d.txt", sysTime.wDay, sysTime.wMonth, sysTime.wYear);
		SetWindowText(GetDlgItem(hDlg, IDC_EDITPATH), cCrashFileName);
		//Open File For Writing
		outHand = CreateFile(cCrashFileName,GENERIC_READ+GENERIC_WRITE,FILE_SHARE_READ+FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		SetFilePointer(outHand, 0, 0, FILE_END);
		WriteFile(outHand, G_cCrashTxt, strlen(G_cCrashTxt), &written, NULL);
		for (int i = 0; i < 80; i++)
			WriteFile(outHand, &cDash, 1, &written, NULL);
		WriteFile(outHand, cLF, 2, &written, NULL);
		WriteFile(outHand, cLF, 2, &written, NULL);
		CloseHandle(outHand);
		break;
//	default:
//		return 0;
	}
	return 0;
}*/

/*LONG lpTopLevelExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo){

	//Shutdown everything
	G_bIsThread = FALSE;
	_StopTimer(G_mmTimer);

	try{
		delete G_pGame;
		G_pGame = NULL;
	}
	catch (...) {
	}

	ZeroMemory(G_cCrashTxt, sizeof(G_cCrashTxt));

	//Format a nice output

	//Reason for crash
	strcpy(G_cCrashTxt, "HGServer Exception Information\r\n");
	strcat(G_cCrashTxt, "Code : ");
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "0x%.8X\r\n", ExceptionInfo->ExceptionRecord->ExceptionCode);
	strcat(G_cCrashTxt, "Flags : ");
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "0x%.8X\r\n", ExceptionInfo->ExceptionRecord->ExceptionFlags);
	strcat(G_cCrashTxt, "Address : ");
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "0x%.8X\r\n", ExceptionInfo->ExceptionRecord->ExceptionAddress);
	strcat(G_cCrashTxt, "Parameters : ");
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "0x%.8X\r\n\r\n", ExceptionInfo->ExceptionRecord->NumberParameters);

	//Retrieve OS version
	GetOSName();
	strcat(G_cCrashTxt, "\r\n");

	//Crash Details
	strcat(G_cCrashTxt, "Context :\r\n");
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "EDI: 0x%.8X\t\tESI: 0x%.8X\t\tEAX: 0x%.8X\r\n",ExceptionInfo->ContextRecord->Edi,
																						ExceptionInfo->ContextRecord->Esi,
																						ExceptionInfo->ContextRecord->Eax);
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "EBX: 0x%.8X\t\tECX: 0x%.8X\t\tEDX: 0x%.8X\r\n",ExceptionInfo->ContextRecord->Ebx,
																						ExceptionInfo->ContextRecord->Ecx,
																						ExceptionInfo->ContextRecord->Edx);
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "EIP: 0x%.8X\t\tEBP: 0x%.8X\t\tSegCs: 0x%.8X\r\n",ExceptionInfo->ContextRecord->Eip,
																						ExceptionInfo->ContextRecord->Ebp,
																						ExceptionInfo->ContextRecord->SegCs);
	wsprintf(&G_cCrashTxt[strlen(G_cCrashTxt)], "EFlags: 0x%.8X\tESP: 0x%.8X\t\tSegSs: 0x%.8X\r\n",ExceptionInfo->ContextRecord->EFlags,
																						ExceptionInfo->ContextRecord->Esp,
																						ExceptionInfo->ContextRecord->SegSs);
	// Show Dialog
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG2), NULL, (DLGPROC)lpCrashDialogFunc);
	SendMessage(0, WM_CLOSE, NULL, NULL);
	return EXCEPTION_EXECUTE_HANDLER;
}*/

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpCmdLine, int nCmdShow )
{
	// ===== OPTIMIZACIONES PARA HARDWARE MODERNO =====
	
	// 1. Prioridad alta para el servidor - mejor tiempo de respuesta
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	
	// 2. Afinidad de CPU: Asignar núcleos 0-3 (P-cores) al servidor para estabilidad
	SetProcessAffinityMask(GetCurrentProcess(), 0xF);  // Núcleos 0,1,2,3
	
	// 3. Deshabilitar Priority Boost para timing consistente
	SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
	
	// ===== FIN OPTIMIZACIONES =====
	
	// Install SEH
	// SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)lpTopLevelExceptionFilter);
	sprintf( szAppClass, "GameServer%d", hInstance);
	if (!InitApplication( hInstance))		return (FALSE);
    if (!InitInstance(hInstance, nCmdShow)) return (FALSE);
	
	Initialize();
	EventLoop();
    return 0;
}
  


BOOL InitApplication( HINSTANCE hInstance)
{     
 WNDCLASS  wc;

	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);             
	wc.hInstance     = hInstance;                    
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);  
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);	 
	wc.lpszMenuName  = NULL;                    		 
	wc.lpszClassName = szAppClass;                   
        
	return (RegisterClass(&wc));
}


BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
 char cTitle[100];
 SYSTEMTIME SysTime;
	
	// Cargar RichEdit DLL
	G_hRichEditLib = LoadLibrary("Msftedit.dll");
	if (G_hRichEditLib == NULL) {
		G_hRichEditLib = LoadLibrary("Riched20.dll");
	}
	
	// Inicializar Common Controls
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES;
	InitCommonControlsEx(&icex);

	GetLocalTime(&SysTime);
	wsprintf(cTitle, "Helbreath GameServer V%s.%s %d (Executed at: %d %d %d)", DEF_UPPERVERSION, DEF_LOWERVERSION, DEF_BUILDDATE, SysTime.wMonth, SysTime.wDay, SysTime.wHour);
	
	G_hWnd = CreateWindowEx(0,
        szAppClass,
        cTitle,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
        CW_USEDEFAULT,
        1024,
        700,
        NULL,
        NULL,
        hInstance,
        NULL );

    if (!G_hWnd) return (FALSE);

	// Establecer iconos para la ventana (grande y pequeño para barra de tareas)
	HICON hIconBig = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAINICON));
	HICON hIconSmall = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	if (hIconBig) SendMessage(G_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
	if (hIconSmall) SendMessage(G_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

	// Fuente para consola
	HFONT hConsoleFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
	HFONT hSmallFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

	// === CONTROL DE COMANDO GM ===
	G_hEditConsole = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
		85, 638, 820, 22,
		G_hWnd, (HMENU)1001, hInstance, NULL);
	SendMessage(G_hEditConsole, WM_SETFONT, (WPARAM)hConsoleFont, TRUE);

	// === BOTÓN WARN (shutdown en 5 min) ===
	G_hBtnWarn2 = CreateWindowEx(0, "BUTTON", "WARN 5MIN",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		805, 636, 100, 26,
		G_hWnd, (HMENU)1007, hInstance, NULL);
	SendMessage(G_hBtnWarn2, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

	// === BOTÓN SHUTDOWN ===
	G_hBtnShutdown = CreateWindowEx(0, "BUTTON", "SHUTDOWN",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		910, 636, 90, 26,
		G_hWnd, (HMENU)1002, hInstance, NULL);
	SendMessage(G_hBtnShutdown, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
	
	// === RICHEDIT PARA LOGS (con colores) ===
	// Intentar cargar RichEdit, si falla usar EDIT normal
	if (G_hRichEditLib != NULL) {
		G_hRichEditLogs = CreateWindowEx(WS_EX_CLIENTEDGE, "RichEdit20A", "",
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
			10, 167, 990, 180,
			G_hWnd, (HMENU)1003, hInstance, NULL);
		if (G_hRichEditLogs != NULL) {
			SendMessage(G_hRichEditLogs, EM_SETBKGNDCOLOR, 0, RGB(25, 27, 38));
		}
	}
	// Fallback a EDIT normal si RichEdit falla
	if (G_hRichEditLogs == NULL) {
		G_hRichEditLogs = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
			10, 167, 990, 180,
			G_hWnd, (HMENU)1003, hInstance, NULL);
	}
	if (G_hRichEditLogs != NULL) {
		SendMessage(G_hRichEditLogs, WM_SETFONT, (WPARAM)hConsoleFont, TRUE);
	}
	
	// === BOTÓN SAVE LOGS ===
	G_hBtnSaveLogs = CreateWindowEx(0, "BUTTON", "Save Logs",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		920, 146, 85, 18,
		G_hWnd, (HMENU)1004, hInstance, NULL);
	SendMessage(G_hBtnSaveLogs, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
	
	// === CHECKBOX ERRORS ONLY ===
	G_hChkErrorsOnly = CreateWindowEx(0, "BUTTON", "Errors Only",
		WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		810, 147, 100, 16,
		G_hWnd, (HMENU)1005, hInstance, NULL);
	SendMessage(G_hChkErrorsOnly, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
	
	// === BOTÓN ACCOUNTS ===
	G_hBtnAccounts = CreateWindowEx(0, "BUTTON", "Accounts",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		634, 636, 80, 26,
		G_hWnd, (HMENU)1008, hInstance, NULL);
	SendMessage(G_hBtnAccounts, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
	
	// === BOTÓN CHARACTERS ===
	G_hBtnCharacters = CreateWindowEx(0, "BUTTON", "Characters",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		719, 636, 90, 26,
		G_hWnd, (HMENU)1009, hInstance, NULL);
	SendMessage(G_hBtnCharacters, WM_SETFONT, (WPARAM)hSmallFont, TRUE);

	// === BOTÓN ACCOUNT CREATOR ===
	G_hBtnAccountCreator = CreateWindowEx(0, "BUTTON", "Account Creator",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		0, 0, 100, 26, // Position will be set in WM_SIZE
		G_hWnd, (HMENU)IDC_BTN_ACC_CREATOR, hInstance, NULL);
	SendMessage(G_hBtnAccountCreator, WM_SETFONT, (WPARAM)hSmallFont, TRUE);
	
	// ListBox de mapas eliminado - se usa dibujo directo en DisplayInfo
	
	// Mantener compatibilidad con G_hEditLogs
	G_hEditLogs = G_hRichEditLogs;
	
	// Cargar logs existentes al RichEdit (por si el servidor ya escribió antes de crear la ventana)
	if (G_hRichEditLogs != NULL && strlen(G_cMsgList) > 0) {
		// Los logs están almacenados en G_cMsgList, cargarlos
		for (int i = 499; i >= 0; i--) {
			char* pLine = G_cMsgList + (i * 120);
			if (strlen(pLine) > 0) {
				char cLogLine[300];
				wsprintf(cLogLine, "%s\r\n", pLine);
				COLORREF color = GetLogColor(pLine);
				AppendColoredText(G_hRichEditLogs, cLogLine, color);
			}
		}
	}
	
	// Subclassing del control EDIT para capturar Enter y TAB
	G_lpfnEditProc = (WNDPROC)SetWindowLongPtr(G_hEditConsole, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
	
	// Dar foco inicial al control EDIT
	SetFocus(G_hEditConsole);
    
	ShowWindow(G_hWnd, nCmdShow);    
	UpdateWindow(G_hWnd);
	
	// Forzar reposicionamiento inicial de controles
	SendMessage(G_hWnd, WM_SIZE, 0, 0);

	return (TRUE);                 
}



int EventLoop()
{
 static unsigned short _usCnt = 0; 
 register MSG msg;

	while( 1 ) {
		if( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
			if( !GetMessage( &msg, NULL, 0, 0 ) ) {
				return msg.wParam;
			}
			// Siempre procesar los mensajes - el subclassing del EDIT maneja Enter
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			UpdateScreen();
		}
		else WaitMessage();
	}
}



void Initialize()
{

	if (_InitWinsock() == FALSE) {
		MessageBox(G_hWnd, "Socket 1.1 not found! Cannot execute program.","ERROR", MB_ICONEXCLAMATION | MB_OK);
		PostQuitMessage(0);
		return;
	}

	G_pGame = new class CGame(G_hWnd);
	if (G_pGame->bInit() == FALSE) {
		PutLogList("(!!!) STOPPED!");
		return;
	}

	// ���� ����� Ÿ�̸� 
	G_mmTimer = _StartTimer(300);

	G_pListenSock = new class XSocket(G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
	G_pListenSock->bListen(G_pGame->m_cGameServerAddr, G_pGame->m_iGameServerPort, WM_USER_ACCEPT);

	G_pLoginSock = new class XSocket(G_hWnd, DEF_SERVERSOCKETBLOCKLIMIT);
	G_pLoginSock->bListen(G_pGame->m_cGameServerAddr, G_pGame->m_iLogServerPort, WM_USER_ACCEPT_LOGIN);

	pLogFile = NULL;
	//pLogFile = fopen("test.log","wt+");
}

void OnDestroy()
{
	if (G_pListenSock != NULL) delete G_pListenSock;
	if (G_pLogSock != NULL) delete G_pLogSock;
	if (G_pLoginSock) delete G_pLoginSock;

	if (G_pGame != NULL) {
		G_pGame->Quit();
		delete G_pGame;
	}

	if (g_login)
	{
		delete g_login;
		g_login = NULL;
	}

	if (G_mmTimer != NULL) _StopTimer(G_mmTimer);
	_TermWinsock();

	if (pLogFile != NULL) fclose(pLogFile);

	PostQuitMessage(0);
}

void OnAcceptLogin()
{
	G_pGame->bAcceptLogin(G_pLoginSock);
}

// Función auxiliar para añadir texto con color al RichEdit
void AppendColoredText(HWND hRichEdit, const char* text, COLORREF color)
{
	if (hRichEdit == NULL) return;
	
	// Mover al final
	int len = GetWindowTextLength(hRichEdit);
	SendMessage(hRichEdit, EM_SETSEL, len, len);
	
	// Si tenemos RichEdit real, usar colores
	if (G_hRichEditLib != NULL) {
		// Configurar formato de carácter con color
		CHARFORMAT2 cf;
		ZeroMemory(&cf, sizeof(cf));
		cf.cbSize = sizeof(CHARFORMAT2);
		cf.dwMask = CFM_COLOR | CFM_BOLD;
		cf.crTextColor = color;
		cf.dwEffects = 0;  // Sin negrita por defecto
		
		SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	}
	
	// Insertar el texto
	SendMessage(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
}

// Determinar color según tipo de mensaje
COLORREF GetLogColor(const char* cMsg)
{
	// Errores - Rojo
	if (strstr(cMsg, "ERROR") || strstr(cMsg, "error") || 
		strstr(cMsg, "FAIL") || strstr(cMsg, "fail") ||
		strstr(cMsg, "(!!!)") || strstr(cMsg, "CRASH") ||
		strstr(cMsg, "Exception") || strstr(cMsg, "CRITICAL")) {
		return RGB(255, 80, 80);
	}
	// Warnings - Amarillo/Naranja
	if (strstr(cMsg, "WARNING") || strstr(cMsg, "warning") ||
		strstr(cMsg, "(!)") || strstr(cMsg, "ATTACK") ||
		strstr(cMsg, "PK") || strstr(cMsg, "Hack") ||
		strstr(cMsg, "DISCONNECT") || strstr(cMsg, "disconnect")) {
		return RGB(255, 200, 80);
	}
	// Conexiones - Verde
	if (strstr(cMsg, "connected") || strstr(cMsg, "Connected") ||
		strstr(cMsg, "logged in") || strstr(cMsg, "LOGIN") ||
		strstr(cMsg, "joined") || strstr(cMsg, "SUCCESS") ||
		strstr(cMsg, "Saved") || strstr(cMsg, "SAVE")) {
		return RGB(80, 255, 120);
	}
	// Comandos GM - Morado
	if (strstr(cMsg, "[GM]") || strstr(cMsg, "[CMD]") ||
		strstr(cMsg, "GM Command") || strstr(cMsg, "Admin") ||
		strstr(cMsg, "SPAWN") || strstr(cMsg, "spawn")) {
		return RGB(200, 150, 255);
	}
	// Info del sistema - Cyan
	if (strstr(cMsg, "(i)") || strstr(cMsg, "INFO") ||
		strstr(cMsg, "Server") || strstr(cMsg, "Map")) {
		return RGB(100, 200, 255);
	}
	// Default - Gris claro
	return RGB(180, 180, 190);
}

// Verificar si es mensaje de error/warning
BOOL IsErrorOrWarning(const char* cMsg)
{
	return (strstr(cMsg, "ERROR") || strstr(cMsg, "error") || 
			strstr(cMsg, "FAIL") || strstr(cMsg, "fail") ||
			strstr(cMsg, "(!!!)") || strstr(cMsg, "CRASH") ||
			strstr(cMsg, "WARNING") || strstr(cMsg, "warning") ||
			strstr(cMsg, "(!)") || strstr(cMsg, "Hack") ||
			strstr(cMsg, "CRITICAL") || strstr(cMsg, "Exception"));
}

void PutLogList(char * cMsg)
{
	// Initialize critical section on first use
	if (!G_bLogLockInitialized) {
		InitializeCriticalSection(&G_csLogLock);
		G_bLogLockInitialized = TRUE;
	}
	
	// Enter critical section to prevent concurrent access
	EnterCriticalSection(&G_csLogLock);
	
	char cTemp[120*500];
	
	G_cMsgUpdated = TRUE;
	ZeroMemory(cTemp, sizeof(cTemp));
	memcpy((cTemp + 120), G_cMsgList, 120*499);
	memcpy(cTemp, cMsg, strlen(cMsg));
	memcpy(G_cMsgList, cTemp, 120*500);
	PutAdminLogFileList(cMsg);
	
	// Agregar al control de logs (RichEdit o EDIT normal)
	if (G_hRichEditLogs != NULL && IsWindow(G_hRichEditLogs)) {
		// Filtrar si está activado "Errors Only"
		if (G_bErrorsOnlyFilter && !IsErrorOrWarning(cMsg)) {
			LeaveCriticalSection(&G_csLogLock);
			return;
		}
		
		// Limitar tamaño del log
		int len = GetWindowTextLength(G_hRichEditLogs);
		if (len > 50000) {
			// Limpiar y empezar de nuevo
			SendMessage(G_hRichEditLogs, WM_SETTEXT, 0, (LPARAM)"--- Log truncated ---\r\n");
		}
		
		// Añadir timestamp
		SYSTEMTIME st;
		GetLocalTime(&st);
		char cLogLine[300];
		wsprintf(cLogLine, "[%02d:%02d:%02d] %s\r\n", st.wHour, st.wMinute, st.wSecond, cMsg);
		
		// Mover cursor al final
		SendMessage(G_hRichEditLogs, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
		
		// Si es RichEdit real, usar colores
		if (G_hRichEditLib != NULL) {
			CHARFORMAT cf;
			ZeroMemory(&cf, sizeof(cf));
			cf.cbSize = sizeof(CHARFORMAT);
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = GetLogColor(cMsg);
			SendMessage(G_hRichEditLogs, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}
		
		// Insertar texto
		SendMessage(G_hRichEditLogs, EM_REPLACESEL, FALSE, (LPARAM)cLogLine);
		
		// Auto-scroll al final - método robusto
		int lineCount = (int)SendMessage(G_hRichEditLogs, EM_GETLINECOUNT, 0, 0);
		SendMessage(G_hRichEditLogs, EM_LINESCROLL, 0, lineCount);
	}
	
	LeaveCriticalSection(&G_csLogLock);
}

void PutXSocketLogList(char * cMsg)
{
// char cTemp[120*50];
	
	//G_cMsgUpdated = TRUE;
	//ZeroMemory(cTemp, sizeof(cTemp));
	//memcpy((cTemp + 120), G_cMsgList, 120*49);
	//memcpy(cTemp, cMsg, strlen(cMsg));
	//memcpy(G_cMsgList, cTemp, 120*50);
	PutXSocketLogFileList(cMsg);

}

void UpdateScreen()
{
	if (G_cMsgUpdated == TRUE) {
		InvalidateRect(G_hWnd, NULL, FALSE);
		G_cMsgUpdated = FALSE;
	}
	
	// Verificar timer de shutdown automático
	if (G_bShutdownPending == TRUE && G_dwShutdownTimer > 0) {
		DWORD dwElapsed = timeGetTime() - G_dwShutdownTimer;
		DWORD dwTotalTime = 5 * 60 * 1000; // 5 minutos en ms
		
		if (dwElapsed >= dwTotalTime) {
			// 5 minutos pasados - ejecutar shutdown
			G_bShutdownPending = FALSE;
			G_iLastMinuteWarned = 6;
			PutLogList("(!) AUTO-SHUTDOWN: 5 minutes elapsed - Shutting down server...");
			
			if (G_pGame != NULL) {
				G_pGame->m_bF1pressed = TRUE;
				G_pGame->m_bF4pressed = TRUE;
			}
			SetWindowText(G_hBtnWarn2, "CLOSING");
		} else {
			// Calcular tiempo restante (proteger contra overflow)
			DWORD dwRemaining = dwTotalTime - dwElapsed;
			int iTotalSecsRemaining = (int)(dwRemaining / 1000);
			int iMinsRemaining = iTotalSecsRemaining / 60;
			int iSecs = iTotalSecsRemaining % 60;
			
			// Enviar aviso cada minuto (4, 3, 2, 1, 0) - el 5 ya se envió al pulsar
			// Se dispara cuando pasamos de X:00 a (X-1):59
			if (iMinsRemaining < G_iLastMinuteWarned) {
				G_iLastMinuteWarned = iMinsRemaining;
				
				if (G_pGame != NULL) {
					int iCount = 0;
					for (int i = 1; i < DEF_MAXCLIENTS; i++) {
						if (G_pGame->m_pClientList[i] != NULL && G_pGame->m_pClientList[i]->m_bIsInitComplete == TRUE) {
							// Enviar aviso de shutdown con minutos restantes
							G_pGame->SendNotifyMsg(NULL, i, DEF_NOTIFY_SERVERSHUTDOWN, 1, iMinsRemaining + 1, NULL, NULL);
							
							// Si queda 1 minuto (iMinsRemaining == 0), forzar logout (10 seg cuenta atrás)
							if (iMinsRemaining == 0) {
								G_pGame->SendNotifyMsg(NULL, i, DEF_NOTIFY_FORCEDISCONN, 10, NULL, NULL, NULL);
							}
							iCount++;
						}
					}
					char cLogMsg[128];
					if (iMinsRemaining > 0) {
						wsprintf(cLogMsg, "(!) SHUTDOWN WARNING: %d minute(s) remaining - Sent to %d players", iMinsRemaining + 1, iCount);
					} else {
						wsprintf(cLogMsg, "(!) SHUTDOWN: <1 minute remaining - FORCING LOGOUT for %d players!", iCount);
					}
					PutLogList(cLogMsg);
				}
			}
			
			// Actualizar texto del botón con tiempo restante
			char cBtnText[16];
			wsprintf(cBtnText, "%d:%02d...", iMinsRemaining, iSecs);
			SetWindowText(G_hBtnWarn2, cBtnText);
		}
	}
}


// Función auxiliar para dibujar paneles con borde
void DrawPanel(HDC hdc, int x, int y, int w, int h, COLORREF borderColor, COLORREF bgColor)
{
	HBRUSH hBrush = CreateSolidBrush(bgColor);
	HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
	HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
	Rectangle(hdc, x, y, x + w, y + h);
	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
	DeleteObject(hBrush);
	DeleteObject(hPen);
}

void OnPaint()
{
	HDC hdc;
	PAINTSTRUCT ps;
	register short i;
	char * cMsg;
	char cTxt[256];
	HFONT hFont, hFontBold, hFontSmall, hOldFont;
	HBRUSH hBgBrush;
	RECT rcClient;

	hdc = BeginPaint(G_hWnd, &ps);
	GetClientRect(G_hWnd, &rcClient);
	
	int winWidth = rcClient.right;
	int winHeight = rcClient.bottom;
	
	// ========== DOBLE BUFFER PARA EVITAR PARPADEO ==========
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, winWidth, winHeight);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
	
	// Usar hdcMem en lugar de hdc para todo el dibujado
	HDC hdcDraw = hdcMem;
	
	int panelWidth = winWidth - 10;  // Margen de 5 a cada lado
	int halfWidth = (winWidth - 15) / 2;  // Mitad del ancho para paneles lado a lado

	// Fondo oscuro principal
	hBgBrush = CreateSolidBrush(RGB(20, 22, 30));
	FillRect(hdcDraw, &rcClient, hBgBrush);
	DeleteObject(hBgBrush);

	// Crear fuentes
	hFont = CreateFont(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");
	hFontBold = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
	hFontSmall = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Consolas");

	SetBkMode(hdcDraw, TRANSPARENT);
	hOldFont = (HFONT)SelectObject(hdcDraw, hFontBold);

	// ========== FILA SUPERIOR (Info del servidor + Rendimiento) ==========
	// Panel de información del servidor (izquierda)
	DrawPanel(hdcDraw, 5, 5, halfWidth, 75, RGB(60, 60, 80), RGB(30, 32, 45));
	
	// Panel de rendimiento (derecha)
	DrawPanel(hdcDraw, halfWidth + 10, 5, halfWidth, 55, RGB(60, 60, 80), RGB(30, 32, 45));
	SetTextColor(hdcDraw, RGB(100, 200, 150));
	TextOut(hdcDraw, halfWidth + 20, 8, "PERFORMANCE", 11);

	// ========== SEGUNDA FILA (Eventos + Conexiones) ==========
	// Panel de estado de eventos (izquierda)
	DrawPanel(hdcDraw, 5, 85, halfWidth, 55, RGB(60, 60, 80), RGB(30, 32, 45));
	SetTextColor(hdcDraw, RGB(255, 200, 100));
	TextOut(hdcDraw, 15, 88, "EVENTS STATUS", 13);

	// Panel de últimas conexiones (derecha)
	DrawPanel(hdcDraw, halfWidth + 10, 65, halfWidth, 75, RGB(60, 60, 80), RGB(30, 32, 45));
	SetTextColor(hdcDraw, RGB(100, 180, 255));
	TextOut(hdcDraw, halfWidth + 20, 68, "RECENT CONNECTIONS", 18);

	// Calcular alturas dinámicas para logs y mapas
	int logsTop = 145;
	int gmPanelHeight = 35;
	int gmPanelTop = winHeight - gmPanelHeight - 5;
	int availableHeight = gmPanelTop - logsTop - 10;  // Espacio disponible
	int logsHeight = availableHeight * 55 / 100;  // 55% para logs
	int mapsTop = logsTop + logsHeight + 5;
	int mapsHeight = gmPanelTop - mapsTop - 5;  // Resto para mapas
	
	// Calcular ancho del panel de logs (dejar espacio para PLAYERS ONLINE - 330px)
	int logsPanelWidth = panelWidth - 330;
	if (logsPanelWidth < 300) logsPanelWidth = 300;

	// ========== PANEL DE LOGS ========== (Usa control EDIT - G_hEditLogs)
	// Dibujamos el marco/fondo del panel, el control EDIT está encima
	DrawPanel(hdcDraw, 5, logsTop, logsPanelWidth, logsHeight, RGB(50, 50, 70), RGB(25, 27, 38));
	SetTextColor(hdcDraw, RGB(120, 180, 255));
	TextOut(hdcDraw, 15, logsTop + 3, "SERVER LOG (Ctrl+A/C)", 21);

	// ========== PANEL DE MAPAS ==========
	DrawPanel(hdcDraw, 5, mapsTop, logsPanelWidth, mapsHeight, RGB(50, 50, 70), RGB(25, 27, 38));
	// Titulo y contenido se dibujan en DisplayInfo()

	// ========== PANEL DE CONSOLA GM ==========
	DrawPanel(hdcDraw, 5, gmPanelTop, panelWidth, gmPanelHeight, RGB(80, 60, 100), RGB(35, 30, 50));
	SetTextColor(hdcDraw, RGB(255, 180, 100));
	TextOut(hdcDraw, 15, gmPanelTop + 8, "GM CMD >", 8);

	// Los logs ahora se muestran en G_hRichEditLogs (RichEdit con colores)

	// Renderizar últimas conexiones con colores
	SelectObject(hdcDraw, hFontSmall);
	for (i = 0; i < 5; i++) {
		if (strlen(G_cLastConnections[i]) > 0) {
			// Color según tipo de acción
			if (strstr(G_cLastConnections[i], "CONNECT") && !strstr(G_cLastConnections[i], "DISCONNECT")) {
				SetTextColor(hdcDraw, RGB(100, 255, 150));  // Verde para conexiones
			} else if (strstr(G_cLastConnections[i], "DISCONNECT")) {
				SetTextColor(hdcDraw, RGB(255, 150, 100));  // Naranja para desconexiones
			} else {
				SetTextColor(hdcDraw, RGB(150, 200, 255));  // Azul por defecto
			}
			TextOut(hdcDraw, halfWidth + 20, 83 + i*11, G_cLastConnections[i], strlen(G_cLastConnections[i]));
		}
	}

	// Llamar a DisplayInfo para estadísticas y mapas (pasar posiciones dinámicas)
	if (G_pGame != NULL) {
		SelectObject(hdcDraw, hFontBold);
		// Guardar posiciones para DisplayInfo
		G_pGame->m_iDisplayMapsTop = mapsTop;
		G_pGame->m_iDisplayMapsHeight = mapsHeight;
		G_pGame->m_iDisplayWidth = winWidth;
		G_pGame->m_iDisplayMapsWidth = logsPanelWidth;  // Ancho sin PLAYERS ONLINE
		G_pGame->DisplayInfo(hdcDraw);
	}

	SelectObject(hdcDraw, hOldFont);
	DeleteObject(hFont);
	DeleteObject(hFontBold);
	DeleteObject(hFontSmall);

	// ========== COPIAR BUFFER A PANTALLA ==========
	BitBlt(hdc, 0, 0, winWidth, winHeight, hdcMem, 0, 0, SRCCOPY);
	
	// Limpiar doble buffer
	SelectObject(hdcMem, hbmOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);

	EndPaint(G_hWnd, &ps);
}



void  OnKeyUp(WPARAM wParam, LPARAM lParam)
{
}


void OnAccept()
{
	G_pGame->bAccept(G_pListenSock);
}

void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUSer, DWORD dw1, DWORD dw2)
{
	PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, NULL);
}



MMRESULT _StartTimer(DWORD dwTime)
{
 TIMECAPS caps;
 MMRESULT timerid;

	timeGetDevCaps(&caps, sizeof(caps));
	timeBeginPeriod(caps.wPeriodMin);
	timerid = timeSetEvent(dwTime,0,_TimerFunc,0, (UINT)TIME_PERIODIC);

	return timerid;
}



void _StopTimer(MMRESULT timerid)
{
 TIMECAPS caps;

	if (timerid != 0) {
		timeKillEvent(timerid);
		timerid = 0;
		timeGetDevCaps(&caps, sizeof(caps));
		timeEndPeriod(caps.wPeriodMin);
	}
}

/*********************************************************************************************************************
**  void PutLogFileList(char * cStr)																				**
**  description			:: writes data into "Events.log"															**
**  last updated		:: November 22, 2004; 5:40 PM; Hypnotoad													**
**	return value		:: void																						**
**********************************************************************************************************************/
void PutLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	// Original:
	// pFile = fopen("Events.log", "at");
	pFile = fopen("GameLogs\\Events.log", "at");
	if (pFile == NULL) return;
	ZeroMemory(cBuffer, sizeof(cBuffer));
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");
	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutAdminLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\AdminEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutHackLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\HackEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutPvPLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\PvPEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutXSocketLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\XSocket.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutItemLogFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\ItemEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}

void PutLogEventFileList(char * cStr)
{
 FILE * pFile;
 char cBuffer[512];
 SYSTEMTIME SysTime;
	
	pFile = fopen("GameLogs\\LogEvents.log", "at");
	if (pFile == NULL) return;

	ZeroMemory(cBuffer, sizeof(cBuffer));
	
	GetLocalTime(&SysTime);
	wsprintf(cBuffer, "(%4d:%2d:%2d:%2d:%2d) - ", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cBuffer, cStr);
	strcat(cBuffer, "\n");

	fwrite(cBuffer, 1, strlen(cBuffer), pFile);
	fclose(pFile);
}
BOOL CALLBACK AccountCreatorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
switch (message)
{
case WM_INITDIALOG:
return TRUE;

case WM_COMMAND:
switch (LOWORD(wParam))
{
case IDC_ACC_CANCEL_BTN:
EndDialog(hDlg, FALSE);
return TRUE;

case IDC_ACC_CREATE_BTN:
{
char cName[12] = {};
char cPassword[12] = {};
char cPassConfirm[12] = {};
char cEmail[52] = {};
char cQuiz[47] = {};
char cAnswer[27] = {};

GetDlgItemText(hDlg, IDC_ACC_NAME, cName, 10);
GetDlgItemText(hDlg, IDC_ACC_PASS, cPassword, 10);
GetDlgItemText(hDlg, IDC_ACC_PASS_CONFIRM, cPassConfirm, 10);
GetDlgItemText(hDlg, IDC_ACC_EMAIL, cEmail, 50);
GetDlgItemText(hDlg, IDC_ACC_QUIZ, cQuiz, 45);
GetDlgItemText(hDlg, IDC_ACC_ANSWER, cAnswer, 25);

if (strlen(cName) == 0 || strlen(cPassword) == 0 || strlen(cPassConfirm) == 0 ||
strlen(cEmail) == 0 || strlen(cQuiz) == 0 || strlen(cAnswer) == 0)
{
MessageBox(hDlg, "All fields are required!", "Error", MB_OK | MB_ICONERROR);
return TRUE;
}

if (strcmp(cPassword, cPassConfirm) != 0)
{
MessageBox(hDlg, "Passwords do not match!", "Error", MB_OK | MB_ICONERROR);
return TRUE;
}

for (int i = 0; i < (int)strlen(cName); i++)
{
if ((cName[i] == ',') || (cName[i] == '=') || (cName[i] == ' ') || (cName[i] == '\n') ||
(cName[i] == '\t') || (cName[i] == '.') || (cName[i] == '\\') || (cName[i] == '/') ||
(cName[i] == ':') || (cName[i] == '*') || (cName[i] == '?') || (cName[i] == '<') ||
						(cName[i] == '>') || (cName[i] == '|') || (cName[i] == '"') || (cName[i] == '`') ||
						(cName[i] == ';') || (cName[i] == '=') || (cName[i] == '@') || (cName[i] == '[') ||
(cName[i] == ']') || (cName[i] == '^') || (cName[i] == '_') || (cName[i] == '\'')) 
{
MessageBox(hDlg, "Invalid characters in Account Name!", "Error", MB_OK | MB_ICONERROR);
return TRUE;
}
}

char cFn[1024];
wsprintf(cFn, "Accounts\\AscII%d\\%s.txt", cName[0], cName);
FILE* pFile = fopen(cFn, "r");
if (pFile != NULL)
{
fclose(pFile);
MessageBox(hDlg, "Account already exists!", "Error", MB_OK | MB_ICONERROR);
return TRUE;
}

_mkdir("Accounts");
char Aux = cName[0];
wsprintf(cFn, "Accounts\\AscII%d", Aux);
_mkdir(cFn);

wsprintf(cFn, "Accounts\\AscII%d\\%s.txt", Aux, cName);
pFile = fopen(cFn, "wt");
if (pFile == NULL)
{
MessageBox(hDlg, "Failed to create account file!", "Error", MB_OK | MB_ICONERROR);
return TRUE;
}

SYSTEMTIME SysTime;
GetLocalTime(&SysTime);

fprintf(pFile, "Account-generated: Time(%d:%d/%d/%d/%d) LocalAdmin\n", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
fprintf(pFile, "account-name = %s\n", cName);
fprintf(pFile, "account-password = %s\n", cPassword);
fprintf(pFile, "account-email = %s\n", cEmail);
fprintf(pFile, "account-quiz = %s\n", cQuiz);
fprintf(pFile, "account-answer = %s\n", cAnswer);
fprintf(pFile, "account-block = NO\n");

fclose(pFile);

char cMsg[256];
wsprintf(cMsg, "Account '%s' created successfully!", cName);
MessageBox(hDlg, cMsg, "Success", MB_OK | MB_ICONINFORMATION);

char cLogMsg[256];
wsprintf(cLogMsg, "(!) Admin created account: %s", cName);
PutLogList(cLogMsg);

EndDialog(hDlg, TRUE);
}
return TRUE;
}
break;
}
return FALSE;
}

