// -------------------------------------------------------------- 
//                 Helbreath Apocalypse Launcher
//
//          Launcher separado que ejecuta Game.exe
// --------------------------------------------------------------

// Evita advertencias de funciones "inseguras" de la CRT (strcpy, strcat, sprintf, fopen...)
// DEBE ir ANTES de los includes
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

// Definir WIN32_LEAN_AND_MEAN para evitar que windows.h incluya winsock.h
// y poder usar winsock2.h sin conflictos
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <objbase.h>    // Para COM (CreateStreamOnHGlobal, IStream, etc.) que GDI+ necesita
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <winbase.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include <exdisp.h>     // Para IWebBrowser2
#include <mshtml.h>     // Para IHTMLDocument2
#include <mshtmhst.h>   // Para hosting del WebBrowser
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

// ===== LAUNCHER CONFIGURATION =====
#define LAUNCHER_DEFAULT_WIDTH 520
#define LAUNCHER_DEFAULT_HEIGHT 580
#define LAUNCHER_MIN_WIDTH 480
#define LAUNCHER_MIN_HEIGHT 500
#define ID_CHECKBOX_BORDERLESS 1001
#define ID_BUTTON_PLAY 1002
#define ID_BUTTON_EXIT 1003
#define ID_RADIO_ONLINE 1004
#define ID_RADIO_TEST 1005
#define ID_TIMER_SERVER_CHECK 1006
#define ID_TIMER_NEWS_SCROLL 1007

// URL de noticias en GitHub (archivo HTML)
#define NEWS_URL "https://raw.githubusercontent.com/Omymnr/HB-Repository/main/news.html"

// Colores del tema medieval
#define COLOR_BG RGB(25, 20, 15)              // Marrón muy oscuro
#define COLOR_BG_LIGHT RGB(45, 35, 25)        // Marrón oscuro
#define COLOR_NEWS_BG RGB(18, 15, 10)         // Fondo panel noticias
#define COLOR_TEXT RGB(210, 190, 150)         // Beige/pergamino
#define COLOR_ACCENT RGB(180, 140, 60)        // Dorado
#define COLOR_BUTTON RGB(60, 45, 30)          // Marrón botón
#define COLOR_BUTTON_HOVER RGB(180, 140, 60)  // Dorado hover
#define COLOR_BORDER RGB(100, 75, 45)         // Borde marrón
#define COLOR_NEWS_TITLE RGB(220, 180, 80)    // Título noticia (dorado brillante)
#define COLOR_NEWS_DATE RGB(140, 120, 90)     // Fecha noticia
#define COLOR_NEWS_SERVER RGB(220, 80, 80)    // Categoría SERVER (rojo)
#define COLOR_NEWS_CLIENT RGB(80, 160, 220)   // Categoría CLIENT (azul)
#define COLOR_NEWS_LAUNCHER RGB(180, 100, 200)// Categoría LAUNCHER (morado)

// Estructura para almacenar noticias
#define MAX_NEWS 20
#define MAX_NEWS_TITLE 128
#define MAX_NEWS_CONTENT 512

struct NewsItem {
    char szDate[32];
    char szCategory[32];
    char szTitle[MAX_NEWS_TITLE];
    char szContent[MAX_NEWS_CONTENT];
    COLORREF color;
};

// Variables globales del launcher
int g_iLauncherWidth = LAUNCHER_DEFAULT_WIDTH;
int g_iLauncherHeight = LAUNCHER_DEFAULT_HEIGHT;
BOOL g_bBorderlessMode = TRUE;  // Por defecto borderless
BOOL g_bTestServer = FALSE;     // Por defecto Server Online
BOOL g_bLauncherResult = FALSE; // TRUE si el usuario pulsa JUGAR
HBRUSH g_hBrushBG = NULL;
HBRUSH g_hBrushBGLight = NULL;
HFONT g_hFontTitle = NULL;
HFONT g_hFontNormal = NULL;
HFONT g_hFontSmall = NULL;
HFONT g_hFontButton = NULL;
HFONT g_hFontMedieval = NULL;
HFONT g_hFontNews = NULL;
HBITMAP g_hLauncherImage = NULL;
HWND g_hTooltip = NULL;
HINSTANCE g_hInstance = NULL;
BOOL g_bServerOnline = FALSE;         // Estado del servidor principal
BOOL g_bTestServerOnline = FALSE;     // Estado del servidor de pruebas
BOOL g_bCheckingServer = FALSE;       // Indica si está comprobando

// Variables para noticias
NewsItem g_News[MAX_NEWS];
int g_iNewsCount = 0;
int g_iNewsScroll = 0;
BOOL g_bNewsLoading = TRUE;
BOOL g_bNewsLoaded = FALSE;
HWND g_hMainWnd = NULL;
HWND g_hWebBrowser = NULL;          // Contenedor del WebBrowser
IWebBrowser2* g_pWebBrowser = NULL; // Interfaz del WebBrowser
char g_szNewsHtml[65536] = {0};     // HTML descargado

// ===== CONFIGURACIÓN DE SERVIDORES =====
// Server Online (para amigos conectando remotamente)
#define ONLINE_SERVER_IP "89.7.69.125"
#define ONLINE_SERVER_PORT 2500

// Test Server (para pruebas en red local)
#define TEST_SERVER_IP "192.168.0.15"
#define TEST_SERVER_PORT 2500

// Clave del registro para guardar configuración
#define LAUNCHER_REG_KEY "SOFTWARE\\HelbreathApocalypse\\Launcher"

// Nombre del ejecutable del juego
#define GAME_EXECUTABLE "Game.exe"

// Cargar configuración del launcher desde el Registro de Windows
void LoadLauncherSettings()
{
    HKEY hKey;
    DWORD dwValue;
    DWORD dwSize = sizeof(DWORD);
    
    if (RegOpenKeyEx(HKEY_CURRENT_USER, LAUNCHER_REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "Borderless", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            g_bBorderlessMode = (dwValue != 0);
        }
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "TestServer", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            g_bTestServer = (dwValue != 0);
        }
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "LauncherWidth", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            if ((int)dwValue >= LAUNCHER_MIN_WIDTH) g_iLauncherWidth = (int)dwValue;
        }
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, "LauncherHeight", NULL, NULL, (LPBYTE)&dwValue, &dwSize) == ERROR_SUCCESS) {
            if ((int)dwValue >= LAUNCHER_MIN_HEIGHT) g_iLauncherHeight = (int)dwValue;
        }
        RegCloseKey(hKey);
    }
}

// Guardar configuración del launcher en el Registro de Windows
void SaveLauncherSettings()
{
    HKEY hKey;
    DWORD dwDisposition;
    
    if (RegCreateKeyEx(HKEY_CURRENT_USER, LAUNCHER_REG_KEY, 0, NULL, 
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, &dwDisposition) == ERROR_SUCCESS) {
        
        DWORD dwValue = g_bBorderlessMode ? 1 : 0;
        RegSetValueEx(hKey, "Borderless", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = g_bTestServer ? 1 : 0;
        RegSetValueEx(hKey, "TestServer", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = (DWORD)g_iLauncherWidth;
        RegSetValueEx(hKey, "LauncherWidth", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        dwValue = (DWORD)g_iLauncherHeight;
        RegSetValueEx(hKey, "LauncherHeight", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
        
        RegCloseKey(hKey);
    }
}

// Escribir LOGIN.CFG con la IP del servidor seleccionado
void WriteLoginConfig()
{
    char cPath[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, cPath);
    strcat(cPath, "\\CONTENTS\\LOGIN.CFG");
    
    FILE* fp = fopen(cPath, "w");
    if (fp != NULL) {
        fprintf(fp, "[CONFIG]\n\n");
        if (g_bTestServer) {
            // Test Server - localhost
            fprintf(fp, "log-server-address  = %s\n", TEST_SERVER_IP);
            fprintf(fp, "log-server-port     = %d\n", TEST_SERVER_PORT);
        } else {
            // Server Online
            fprintf(fp, "log-server-address  = %s\n", ONLINE_SERVER_IP);
            fprintf(fp, "log-server-port     = %d\n", ONLINE_SERVER_PORT);
        }
        fprintf(fp, "game-server-port    = 9907\n");
        fprintf(fp, "game-server-mode    = LAN\n");
        fclose(fp);
    }
}

// Verificar si un servidor está online intentando conectar al puerto
BOOL CheckServerOnline(const char* szIP, int iPort)
{
    SOCKET sock = INVALID_SOCKET;
    BOOL bOnline = FALSE;
    
    // Crear socket
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return FALSE;
    }
    
    // Configurar socket como no-bloqueante para timeout rápido
    unsigned long ulMode = 1;
    ioctlsocket(sock, FIONBIO, &ulMode);
    
    // Configurar dirección del servidor
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((u_short)iPort);
    
    // Convertir IP string a formato binario
    if (inet_pton(AF_INET, szIP, &serverAddr.sin_addr) <= 0) {
        closesocket(sock);
        return FALSE;
    }
    
    // Intentar conectar
    int iResult = connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    
    if (iResult == SOCKET_ERROR) {
        int iError = WSAGetLastError();
        if (iError == WSAEWOULDBLOCK) {
            // Esperar a que la conexión se complete (o falle) con timeout de 2 segundos
            fd_set writeSet, errorSet;
            FD_ZERO(&writeSet);
            FD_ZERO(&errorSet);
            FD_SET(sock, &writeSet);
            FD_SET(sock, &errorSet);
            
            struct timeval timeout;
            timeout.tv_sec = 2;
            timeout.tv_usec = 0;
            
            iResult = select(0, NULL, &writeSet, &errorSet, &timeout);
            if (iResult > 0 && FD_ISSET(sock, &writeSet)) {
                // Verificar si realmente conectó
                int optVal;
                int optLen = sizeof(optVal);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&optVal, &optLen) == 0) {
                    if (optVal == 0) {
                        bOnline = TRUE;
                    }
                }
            }
        }
    } else {
        bOnline = TRUE;
    }
    
    closesocket(sock);
    return bOnline;
}

// Hilo para verificar estado de servidores (evita bloquear la UI)
DWORD WINAPI ServerCheckThread(LPVOID lpParam)
{
    HWND hWnd = (HWND)lpParam;
    
    g_bCheckingServer = TRUE;
    
    // Verificar servidor principal
    g_bServerOnline = CheckServerOnline(ONLINE_SERVER_IP, ONLINE_SERVER_PORT);
    
    // Verificar servidor de pruebas
    g_bTestServerOnline = CheckServerOnline(TEST_SERVER_IP, TEST_SERVER_PORT);
    
    g_bCheckingServer = FALSE;
    
    // Forzar repintado de la ventana
    if (hWnd != NULL && IsWindow(hWnd)) {
        InvalidateRect(hWnd, NULL, FALSE);
    }
    
    return 0;
}

// Iniciar verificación de servidor en segundo plano
void StartServerCheck(HWND hWnd)
{
    if (!g_bCheckingServer) {
        CreateThread(NULL, 0, ServerCheckThread, (LPVOID)hWnd, 0, NULL);
    }
}

// ===== SISTEMA DE NOTICIAS =====

// Función auxiliar para extraer texto entre dos delimitadores
char* ExtractBetween(const char* szStart, const char* szStartTag, const char* szEndTag, char* szOut, int iMaxLen)
{
    if (szStart == NULL) return NULL;
    
    const char* pStart = strstr(szStart, szStartTag);
    if (pStart == NULL) return NULL;
    pStart += strlen(szStartTag);
    
    const char* pEnd = strstr(pStart, szEndTag);
    if (pEnd == NULL) return NULL;
    
    int iLen = (int)(pEnd - pStart);
    if (iLen >= iMaxLen) iLen = iMaxLen - 1;
    if (iLen > 0) {
        strncpy(szOut, pStart, iLen);
        szOut[iLen] = '\0';
    } else {
        szOut[0] = '\0';
    }
    
    return (char*)(pEnd + strlen(szEndTag));
}

// Función para decodificar entidades HTML básicas
void DecodeHtmlEntities(char* szText)
{
    char* pRead = szText;
    char* pWrite = szText;
    
    while (*pRead) {
        if (*pRead == '&') {
            if (strncmp(pRead, "&amp;", 5) == 0) { *pWrite++ = '&'; pRead += 5; }
            else if (strncmp(pRead, "&lt;", 4) == 0) { *pWrite++ = '<'; pRead += 4; }
            else if (strncmp(pRead, "&gt;", 4) == 0) { *pWrite++ = '>'; pRead += 4; }
            else if (strncmp(pRead, "&quot;", 6) == 0) { *pWrite++ = '"'; pRead += 6; }
            else if (strncmp(pRead, "&#39;", 5) == 0) { *pWrite++ = '\''; pRead += 5; }
            else if (strncmp(pRead, "&nbsp;", 6) == 0) { *pWrite++ = ' '; pRead += 6; }
            else { *pWrite++ = *pRead++; }
        } else {
            *pWrite++ = *pRead++;
        }
    }
    *pWrite = '\0';
}

// Parsear el contenido HTML de noticias
void ParseNewsContent(char* szContent)
{
    g_iNewsCount = 0;
    
    if (szContent == NULL || szContent[0] == '\0') return;
    
    // Buscar cada <article class="news-item">
    char* pPos = szContent;
    
    while (pPos != NULL && g_iNewsCount < MAX_NEWS) {
        // Buscar inicio de artículo
        pPos = strstr(pPos, "<article class=\"news-item\">");
        if (pPos == NULL) break;
        
        // Buscar fin de artículo
        char* pEndArticle = strstr(pPos, "</article>");
        if (pEndArticle == NULL) break;
        
        NewsItem* pNews = &g_News[g_iNewsCount];
        memset(pNews, 0, sizeof(NewsItem));
        
        // Extraer fecha: <div class="news-date">📅 FECHA</div>
        char szTemp[256];
        char* pDate = strstr(pPos, "<div class=\"news-date\">");
        if (pDate != NULL && pDate < pEndArticle) {
            ExtractBetween(pDate, "\">", "</div>", szTemp, sizeof(szTemp));
            // Quitar el emoji 📅 si existe (puede ser "📅 " o similar)
            char* pDateText = szTemp;
            // Saltar caracteres UTF-8 del emoji (normalmente 4 bytes) y espacio
            if ((unsigned char)szTemp[0] >= 0xF0) pDateText = szTemp + 4;
            while (*pDateText == ' ') pDateText++;
            strncpy(pNews->szDate, pDateText, sizeof(pNews->szDate) - 1);
        }
        
        // Extraer categoría: <span class="news-category category-XXX">CATEGORIA</span>
        char* pCat = strstr(pPos, "<span class=\"news-category");
        if (pCat != NULL && pCat < pEndArticle) {
            // Determinar color por la clase
            if (strstr(pCat, "category-server") != NULL) {
                pNews->color = COLOR_NEWS_SERVER;
            } else if (strstr(pCat, "category-client") != NULL) {
                pNews->color = COLOR_NEWS_CLIENT;
            } else if (strstr(pCat, "category-launcher") != NULL) {
                pNews->color = COLOR_NEWS_LAUNCHER;
            } else {
                pNews->color = COLOR_ACCENT;
            }
            
            // Extraer texto de categoría
            ExtractBetween(pCat, ">", "</span>", pNews->szCategory, sizeof(pNews->szCategory));
        }
        
        // Extraer título: <h2 class="news-title">TITULO</h2>
        char* pTitle = strstr(pPos, "<h2 class=\"news-title\">");
        if (pTitle != NULL && pTitle < pEndArticle) {
            ExtractBetween(pTitle, "\">", "</h2>", pNews->szTitle, sizeof(pNews->szTitle));
            DecodeHtmlEntities(pNews->szTitle);
        }
        
        // Extraer contenido: <div class="news-content"><p>CONTENIDO</p>
        char* pContent = strstr(pPos, "<div class=\"news-content\">");
        if (pContent != NULL && pContent < pEndArticle) {
            char* pFirstP = strstr(pContent, "<p>");
            if (pFirstP != NULL && pFirstP < pEndArticle) {
                ExtractBetween(pFirstP, "<p>", "</p>", pNews->szContent, sizeof(pNews->szContent));
                DecodeHtmlEntities(pNews->szContent);
                
                // Limpiar tags HTML internos como <span>, <strong>, etc.
                char* pTag = pNews->szContent;
                char* pWrite = pNews->szContent;
                BOOL bInTag = FALSE;
                while (*pTag) {
                    if (*pTag == '<') bInTag = TRUE;
                    else if (*pTag == '>') { bInTag = FALSE; pTag++; continue; }
                    if (!bInTag) *pWrite++ = *pTag;
                    pTag++;
                }
                *pWrite = '\0';
            }
        }
        
        // Solo añadir si tiene título
        if (pNews->szTitle[0] != '\0') {
            g_iNewsCount++;
        }
        
        pPos = pEndArticle + 1;
    }
}

// Descargar noticias usando WinINet
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

DWORD WINAPI NewsDownloadThread(LPVOID lpParam)
{
    g_bNewsLoading = TRUE;
    g_bNewsLoaded = FALSE;
    
    HINTERNET hInternet = InternetOpen("HelbreathLauncher/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet == NULL) {
        g_bNewsLoading = FALSE;
        return 1;
    }
    
    HINTERNET hUrl = InternetOpenUrl(hInternet, NEWS_URL, NULL, 0, 
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (hUrl == NULL) {
        InternetCloseHandle(hInternet);
        g_bNewsLoading = FALSE;
        return 1;
    }
    
    // Leer contenido
    char szBuffer[8192];
    char szContent[32768];
    DWORD dwBytesRead;
    DWORD dwTotalBytes = 0;
    
    szContent[0] = '\0';
    
    while (InternetReadFile(hUrl, szBuffer, sizeof(szBuffer) - 1, &dwBytesRead) && dwBytesRead > 0) {
        szBuffer[dwBytesRead] = '\0';
        if (dwTotalBytes + dwBytesRead < sizeof(szContent) - 1) {
            strcat(szContent, szBuffer);
            dwTotalBytes += dwBytesRead;
        }
    }
    
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    
    // Guardar HTML descargado
    strncpy(g_szNewsHtml, szContent, sizeof(g_szNewsHtml) - 1);
    
    // Parsear noticias (para respaldo)
    ParseNewsContent(szContent);
    
    g_bNewsLoading = FALSE;
    g_bNewsLoaded = TRUE;
    
    // Cargar HTML en el WebBrowser si está disponible
    if (g_pWebBrowser != NULL) {
        // Enviar mensaje para actualizar WebBrowser en el hilo principal
        PostMessage(g_hMainWnd, WM_USER + 100, 0, 0);
    }
    
    // Forzar repintado
    if (g_hMainWnd != NULL && IsWindow(g_hMainWnd)) {
        InvalidateRect(g_hMainWnd, NULL, FALSE);
    }
    
    return 0;
}

// Iniciar descarga de noticias
void StartNewsDownload()
{
    CreateThread(NULL, 0, NewsDownloadThread, NULL, 0, NULL);
}

// ===== WEBBROWSER EMBEBIDO =====

// Estructura para hosting del WebBrowser
class CWebBrowserHost : public IOleClientSite, public IOleInPlaceSite, public IOleInPlaceFrame, public IDocHostUIHandler
{
public:
    HWND m_hWnd;
    LONG m_cRef;
    IOleObject* m_pOleObject;
    IOleInPlaceObject* m_pInPlaceObject;
    
    CWebBrowserHost(HWND hWnd) : m_hWnd(hWnd), m_cRef(1), m_pOleObject(NULL), m_pInPlaceObject(NULL) {}
    
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
        if (riid == IID_IUnknown || riid == IID_IOleClientSite) {
            *ppvObject = static_cast<IOleClientSite*>(this);
        } else if (riid == IID_IOleInPlaceSite) {
            *ppvObject = static_cast<IOleInPlaceSite*>(this);
        } else if (riid == IID_IOleInPlaceFrame) {
            *ppvObject = static_cast<IOleInPlaceFrame*>(this);
        } else if (riid == IID_IDocHostUIHandler) {
            *ppvObject = static_cast<IDocHostUIHandler*>(this);
        } else {
            *ppvObject = NULL;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_cRef); }
    STDMETHODIMP_(ULONG) Release() {
        LONG cRef = InterlockedDecrement(&m_cRef);
        if (cRef == 0) delete this;
        return cRef;
    }
    
    // IOleClientSite
    STDMETHODIMP SaveObject() { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker**) { return E_NOTIMPL; }
    STDMETHODIMP GetContainer(IOleContainer**) { return E_NOTIMPL; }
    STDMETHODIMP ShowObject() { return S_OK; }
    STDMETHODIMP OnShowWindow(BOOL) { return S_OK; }
    STDMETHODIMP RequestNewObjectLayout() { return E_NOTIMPL; }
    
    // IOleWindow (base de IOleInPlaceSite)
    STDMETHODIMP GetWindow(HWND* phwnd) { *phwnd = m_hWnd; return S_OK; }
    STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }
    
    // IOleInPlaceSite
    STDMETHODIMP CanInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnUIActivate() { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, 
                                   LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) {
        *ppFrame = static_cast<IOleInPlaceFrame*>(this);
        AddRef();
        *ppDoc = NULL;
        GetClientRect(m_hWnd, lprcPosRect);
        GetClientRect(m_hWnd, lprcClipRect);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->hwndFrame = m_hWnd;
        lpFrameInfo->haccel = NULL;
        lpFrameInfo->cAccelEntries = 0;
        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE) { return E_NOTIMPL; }
    STDMETHODIMP OnUIDeactivate(BOOL) { return S_OK; }
    STDMETHODIMP OnInPlaceDeactivate() { return S_OK; }
    STDMETHODIMP DiscardUndoState() { return E_NOTIMPL; }
    STDMETHODIMP DeactivateAndUndo() { return E_NOTIMPL; }
    STDMETHODIMP OnPosRectChange(LPCRECT lprcPosRect) {
        if (m_pInPlaceObject) {
            m_pInPlaceObject->SetObjectRects(lprcPosRect, lprcPosRect);
        }
        return S_OK;
    }
    
    // IOleInPlaceUIWindow
    STDMETHODIMP GetBorder(LPRECT) { return E_NOTIMPL; }
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject*, LPCOLESTR) { return S_OK; }
    
    // IOleInPlaceFrame
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) { return E_NOTIMPL; }
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND) { return S_OK; }
    STDMETHODIMP RemoveMenus(HMENU) { return E_NOTIMPL; }
    STDMETHODIMP SetStatusText(LPCOLESTR) { return S_OK; }
    STDMETHODIMP EnableModeless(BOOL) { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD) { return E_NOTIMPL; }
    
    // IDocHostUIHandler - Para personalizar el comportamiento del WebBrowser
    STDMETHODIMP ShowContextMenu(DWORD, POINT*, IUnknown*, IDispatch*) { return S_OK; } // Deshabilita menú contextual
    STDMETHODIMP GetHostInfo(DOCHOSTUIINFO* pInfo) {
        pInfo->cbSize = sizeof(DOCHOSTUIINFO);
        pInfo->dwFlags = DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE;
        pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;
        return S_OK;
    }
    STDMETHODIMP ShowUI(DWORD, IOleInPlaceActiveObject*, IOleCommandTarget*, IOleInPlaceFrame*, IOleInPlaceUIWindow*) { return S_OK; }
    STDMETHODIMP HideUI() { return S_OK; }
    STDMETHODIMP UpdateUI() { return S_OK; }
    STDMETHODIMP OnDocWindowActivate(BOOL) { return S_OK; }
    STDMETHODIMP OnFrameWindowActivate(BOOL) { return S_OK; }
    STDMETHODIMP ResizeBorder(LPCRECT, IOleInPlaceUIWindow*, BOOL) { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, const GUID*, DWORD) { return S_FALSE; }
    STDMETHODIMP GetOptionKeyPath(LPOLESTR*, DWORD) { return E_NOTIMPL; }
    STDMETHODIMP GetDropTarget(IDropTarget*, IDropTarget**) { return E_NOTIMPL; }
    STDMETHODIMP GetExternal(IDispatch** ppDispatch) { *ppDispatch = NULL; return S_FALSE; }
    STDMETHODIMP TranslateUrl(DWORD, LPWSTR, LPWSTR*) { return S_FALSE; }
    STDMETHODIMP FilterDataObject(IDataObject*, IDataObject**) { return S_FALSE; }
};

CWebBrowserHost* g_pHost = NULL;

// Crear el WebBrowser embebido
BOOL CreateEmbeddedBrowser(HWND hParent, int x, int y, int width, int height)
{
    // Crear ventana contenedora
    g_hWebBrowser = CreateWindowEx(
        0, "STATIC", NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, width, height,
        hParent, NULL, g_hInstance, NULL);
    
    if (!g_hWebBrowser) return FALSE;
    
    // Crear host
    g_pHost = new CWebBrowserHost(g_hWebBrowser);
    
    // Crear instancia del WebBrowser
    HRESULT hr = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER, 
                                   IID_IOleObject, (void**)&g_pHost->m_pOleObject);
    if (FAILED(hr)) {
        delete g_pHost;
        g_pHost = NULL;
        return FALSE;
    }
    
    // Configurar el cliente
    g_pHost->m_pOleObject->SetClientSite(g_pHost);
    
    // Activar in-place
    RECT rc;
    GetClientRect(g_hWebBrowser, &rc);
    hr = g_pHost->m_pOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, g_pHost, 0, g_hWebBrowser, &rc);
    
    // Obtener interfaz IWebBrowser2
    hr = g_pHost->m_pOleObject->QueryInterface(IID_IWebBrowser2, (void**)&g_pWebBrowser);
    if (FAILED(hr)) {
        g_pHost->m_pOleObject->Release();
        delete g_pHost;
        g_pHost = NULL;
        return FALSE;
    }
    
    // Obtener IOleInPlaceObject para redimensionamiento
    g_pHost->m_pOleObject->QueryInterface(IID_IOleInPlaceObject, (void**)&g_pHost->m_pInPlaceObject);
    
    // Configurar tamaño
    g_pWebBrowser->put_Left(0);
    g_pWebBrowser->put_Top(0);
    g_pWebBrowser->put_Width(width);
    g_pWebBrowser->put_Height(height);
    
    return TRUE;
}

// Cargar HTML en el WebBrowser
void LoadHtmlInBrowser(const char* szHtml)
{
    if (g_pWebBrowser == NULL || szHtml == NULL || szHtml[0] == '\0') return;
    
    // Navegar a about:blank primero
    VARIANT vEmpty;
    VariantInit(&vEmpty);
    BSTR bstrUrl = SysAllocString(L"about:blank");
    g_pWebBrowser->Navigate(bstrUrl, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
    SysFreeString(bstrUrl);
    
    // Esperar a que cargue
    READYSTATE rs;
    int timeout = 50;
    do {
        Sleep(10);
        g_pWebBrowser->get_ReadyState(&rs);
    } while (rs != READYSTATE_COMPLETE && --timeout > 0);
    
    // Obtener documento
    IDispatch* pDisp = NULL;
    g_pWebBrowser->get_Document(&pDisp);
    if (pDisp == NULL) return;
    
    IHTMLDocument2* pDoc = NULL;
    HRESULT hr = pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc);
    pDisp->Release();
    
    if (FAILED(hr) || pDoc == NULL) return;
    
    // Convertir HTML a BSTR
    int len = MultiByteToWideChar(CP_UTF8, 0, szHtml, -1, NULL, 0);
    WCHAR* wszHtml = new WCHAR[len];
    MultiByteToWideChar(CP_UTF8, 0, szHtml, -1, wszHtml, len);
    
    // Escribir HTML
    SAFEARRAY* psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
    VARIANT* pVar;
    SafeArrayAccessData(psa, (void**)&pVar);
    pVar->vt = VT_BSTR;
    pVar->bstrVal = SysAllocString(wszHtml);
    SafeArrayUnaccessData(psa);
    
    pDoc->write(psa);
    pDoc->close();
    
    // Limpiar
    SafeArrayDestroy(psa);
    delete[] wszHtml;
    pDoc->Release();
}

// Redimensionar WebBrowser
void ResizeWebBrowser(int x, int y, int width, int height)
{
    if (g_hWebBrowser) {
        MoveWindow(g_hWebBrowser, x, y, width, height, TRUE);
    }
    if (g_pWebBrowser) {
        g_pWebBrowser->put_Left(0);
        g_pWebBrowser->put_Top(0);
        g_pWebBrowser->put_Width(width);
        g_pWebBrowser->put_Height(height);
    }
    if (g_pHost && g_pHost->m_pInPlaceObject) {
        RECT rc = {0, 0, width, height};
        g_pHost->m_pInPlaceObject->SetObjectRects(&rc, &rc);
    }
}

// Destruir WebBrowser
void DestroyEmbeddedBrowser()
{
    if (g_pWebBrowser) {
        g_pWebBrowser->Release();
        g_pWebBrowser = NULL;
    }
    if (g_pHost) {
        if (g_pHost->m_pInPlaceObject) {
            g_pHost->m_pInPlaceObject->Release();
        }
        if (g_pHost->m_pOleObject) {
            g_pHost->m_pOleObject->Close(OLECLOSE_NOSAVE);
            g_pHost->m_pOleObject->Release();
        }
        g_pHost->Release();
        g_pHost = NULL;
    }
    if (g_hWebBrowser) {
        DestroyWindow(g_hWebBrowser);
        g_hWebBrowser = NULL;
    }
}

// ===== FIN WEBBROWSER EMBEBIDO =====

// ===== FIN SISTEMA DE NOTICIAS =====

// Ejecutar el juego (Game.exe) con los parámetros adecuados
BOOL LaunchGame()
{
    char szPath[MAX_PATH];
    char szParams[256];
    char szWorkingDir[MAX_PATH];
    
    // Obtener directorio actual
    GetCurrentDirectory(MAX_PATH, szWorkingDir);
    
    // Construir ruta completa al ejecutable
    sprintf(szPath, "%s\\%s", szWorkingDir, GAME_EXECUTABLE);
    
    // Construir parámetros
    // -nolauncher: indica al juego que no muestre su propio launcher
    // -borderless o -fullscreen: modo de pantalla
    if (g_bBorderlessMode) {
        strcpy(szParams, "-nolauncher -borderless");
    } else {
        strcpy(szParams, "-nolauncher -fullscreen");
    }
    
    // Verificar que el ejecutable existe
    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        char szError[512];
        sprintf(szError, "No se encontró el archivo del juego:\n%s\n\nAsegúrate de que Launcher.exe está en la misma carpeta que Game.exe", szPath);
        MessageBox(NULL, szError, "Error - Helbreath Apocalypse", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    
    // Ejecutar el juego
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = NULL;
    sei.lpVerb = "open";
    sei.lpFile = szPath;
    sei.lpParameters = szParams;
    sei.lpDirectory = szWorkingDir;
    sei.nShow = SW_SHOW;
    
    if (!ShellExecuteEx(&sei)) {
        DWORD dwError = GetLastError();
        char szError[256];
        sprintf(szError, "Error al iniciar el juego (código: %d)", dwError);
        MessageBox(NULL, szError, "Error - Helbreath Apocalypse", MB_ICONERROR | MB_OK);
        return FALSE;
    }
    
    return TRUE;
}

// Dibujar botón personalizado estilo medieval
void DrawCustomButton(HDC hdc, RECT* rc, const char* text, BOOL hover, BOOL isPlayButton)
{
    // Fondo del botón con gradiente simulado
    HBRUSH hBrush;
    if (isPlayButton) {
        hBrush = CreateSolidBrush(hover ? RGB(200, 160, 70) : COLOR_ACCENT);
    } else {
        hBrush = CreateSolidBrush(hover ? COLOR_BG_LIGHT : COLOR_BUTTON);
    }
    
    SelectObject(hdc, hBrush);
    
    // Borde del botón
    HPEN hPen = CreatePen(PS_SOLID, 2, COLOR_BORDER);
    SelectObject(hdc, hPen);
    RoundRect(hdc, rc->left, rc->top, rc->right, rc->bottom, 6, 6);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    
    // Borde interior dorado para efecto medieval
    if (isPlayButton) {
        HPEN hPenGold = CreatePen(PS_SOLID, 1, RGB(220, 180, 80));
        SelectObject(hdc, hPenGold);
        RECT rcInner = {rc->left + 3, rc->top + 3, rc->right - 3, rc->bottom - 3};
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        RoundRect(hdc, rcInner.left, rcInner.top, rcInner.right, rcInner.bottom, 4, 4);
        DeleteObject(hPenGold);
    }
    
    // Texto con sombra
    SetBkMode(hdc, TRANSPARENT);
    
    // Sombra del texto
    SetTextColor(hdc, RGB(20, 15, 10));
    RECT rcShadow = {rc->left + 1, rc->top + 1, rc->right + 1, rc->bottom + 1};
    SelectObject(hdc, g_hFontButton);
    DrawText(hdc, text, -1, &rcShadow, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    // Texto principal
    SetTextColor(hdc, COLOR_TEXT);
    DrawText(hdc, text, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// Cargar imagen del launcher desde recursos (PNG incrustado en el exe)
HBITMAP LoadLauncherImage()
{
    using namespace Gdiplus;
    
    // Inicializar GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    HBITMAP hBmp = NULL;
    
    // Buscar recurso PNG en el ejecutable
    HRSRC hResource = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LAUNCHER_IMAGE), "PNG");
    if (hResource) {
        DWORD imageSize = SizeofResource(GetModuleHandle(NULL), hResource);
        HGLOBAL hGlobal = LoadResource(GetModuleHandle(NULL), hResource);
        if (hGlobal) {
            void* pResourceData = LockResource(hGlobal);
            if (pResourceData) {
                // Crear stream desde memoria
                IStream* pStream = NULL;
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, imageSize);
                if (hMem) {
                    void* pMem = GlobalLock(hMem);
                    memcpy(pMem, pResourceData, imageSize);
                    GlobalUnlock(hMem);
                    
                    if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) == S_OK) {
                        Image* pImage = Image::FromStream(pStream);
                        if (pImage && pImage->GetLastStatus() == Ok) {
                            Bitmap* pBitmap = static_cast<Bitmap*>(pImage);
                            pBitmap->GetHBITMAP(Color(0, 0, 0), &hBmp);
                            delete pImage;
                        }
                        pStream->Release();
                    }
                }
            }
        }
    }
    
    GdiplusShutdown(gdiplusToken);
    return hBmp;
}

// Crear tooltip para controles
HWND CreateTooltip(HWND hWnd, HWND hControl, const char* text)
{
    HWND hTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hWnd, NULL, GetModuleHandle(NULL), NULL);
    
    if (!hTip) return NULL;
    
    TOOLINFO ti = {0};
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = hWnd;
    ti.uId = (UINT_PTR)hControl;
    ti.lpszText = (LPSTR)text;
    
    SendMessage(hTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    SendMessage(hTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 10000);
    SendMessage(hTip, TTM_SETDELAYTIME, TTDT_INITIAL, 300);
    
    return hTip;
}

// Procedimiento de ventana del Launcher
LRESULT CALLBACK LauncherWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hCheckBorderless = NULL;
    static HWND hRadioOnline = NULL;
    static HWND hRadioTest = NULL;
    static HWND hBtnPlay = NULL;
    static HWND hBtnExit = NULL;
    static BOOL bPlayHover = FALSE;
    static BOOL bExitHover = FALSE;
    static BOOL bDragging = FALSE;
    static POINT ptDragStart;
    
    switch (message) {
    case WM_CREATE:
    {
        g_hMainWnd = hWnd;
        
        // Inicializar COM para WebBrowser
        OleInitialize(NULL);
        
        // Cargar imagen del launcher
        g_hLauncherImage = LoadLauncherImage();
        
        // Crear fuentes
        g_hFontTitle = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Times New Roman");
        g_hFontMedieval = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Times New Roman");
        g_hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontSmall = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontButton = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontNews = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        
        // Crear brushes
        g_hBrushBG = CreateSolidBrush(COLOR_BG);
        g_hBrushBGLight = CreateSolidBrush(COLOR_BG_LIGHT);
        
        // Calcular posiciones centradas
        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right;
        int height = rc.bottom;
        int centerX = width / 2;
        int controlsY = 220;  // Debajo de la imagen
        
        // Radio buttons para selección de servidor
        hRadioOnline = CreateWindow("BUTTON", "  Servidor Principal",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
            centerX - 100, controlsY, 200, 25, hWnd, (HMENU)ID_RADIO_ONLINE, NULL, NULL);
        SendMessage(hRadioOnline, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        hRadioTest = CreateWindow("BUTTON", "  Servidor de Pruebas",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            centerX - 100, controlsY + 28, 200, 25, hWnd, (HMENU)ID_RADIO_TEST, NULL, NULL);
        SendMessage(hRadioTest, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        // Seleccionar el radio button según configuración guardada
        SendMessage(g_bTestServer ? hRadioTest : hRadioOnline, BM_SETCHECK, BST_CHECKED, 0);
        
        // Checkbox para modo ventana
        hCheckBorderless = CreateWindow("BUTTON", "  Modo Ventana",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            centerX - 100, controlsY + 75, 200, 25, hWnd, (HMENU)ID_CHECKBOX_BORDERLESS, NULL, NULL);
        SendMessage(hCheckBorderless, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(hCheckBorderless, BM_SETCHECK, g_bBorderlessMode ? BST_CHECKED : BST_UNCHECKED, 0);
        
        // Crear tooltip
        g_hTooltip = CreateTooltip(hWnd, hCheckBorderless, "Modo ventana sin bordes");
        
        // Crear WebBrowser embebido para noticias
        int newsStartY = controlsY + 150;
        int newsPanelHeight = height - newsStartY - 70;
        if (newsPanelHeight > 100) {
            CreateEmbeddedBrowser(hWnd, 20, newsStartY, width - 40, newsPanelHeight);
            
            // Mostrar página de carga inicial
            const char* szLoadingHtml = 
                "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                "<style>body{margin:0;padding:20px;background:#12100a;color:#d2be96;font-family:'Segoe UI',sans-serif;}"
                ".loading{text-align:center;padding-top:40px;color:#b48c3c;}</style></head>"
                "<body><div class='loading'>Cargando noticias...</div></body></html>";
            LoadHtmlInBrowser(szLoadingHtml);
        }
        
        // Iniciar verificación de estado de servidores
        StartServerCheck(hWnd);
        
        // Iniciar descarga de noticias
        StartNewsDownload();
        
        // Configurar timer para verificar estado cada 5 segundos
        SetTimer(hWnd, ID_TIMER_SERVER_CHECK, 5000, NULL);
        
        return 0;
    }
    
    case WM_TIMER:
    {
        if (wParam == ID_TIMER_SERVER_CHECK) {
            StartServerCheck(hWnd);
        }
        return 0;
    }
    
    // Mensaje personalizado para cargar HTML en el WebBrowser
    case WM_USER + 100:
    {
        if (g_szNewsHtml[0] != '\0') {
            LoadHtmlInBrowser(g_szNewsHtml);
        }
        return 0;
    }
    
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = LAUNCHER_MIN_WIDTH;
        mmi->ptMinTrackSize.y = LAUNCHER_MIN_HEIGHT;
        return 0;
    }
    
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED) {
            g_iLauncherWidth = LOWORD(lParam);
            g_iLauncherHeight = HIWORD(lParam);
            
            // Reposicionar controles centrados
            int centerX = g_iLauncherWidth / 2;
            int controlsY = 220;
            
            if (hRadioOnline) SetWindowPos(hRadioOnline, NULL, centerX - 100, controlsY, 200, 25, SWP_NOZORDER);
            if (hRadioTest) SetWindowPos(hRadioTest, NULL, centerX - 100, controlsY + 28, 200, 25, SWP_NOZORDER);
            if (hCheckBorderless) SetWindowPos(hCheckBorderless, NULL, centerX - 100, controlsY + 75, 200, 25, SWP_NOZORDER);
            
            // Redimensionar WebBrowser
            int newsStartY = controlsY + 150;
            int newsPanelHeight = g_iLauncherHeight - newsStartY - 70;
            if (newsPanelHeight > 100) {
                ResizeWebBrowser(20, newsStartY, g_iLauncherWidth - 40, newsPanelHeight);
            }
            
            InvalidateRect(hWnd, NULL, TRUE);
        }
        return 0;
    }
    
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXT);
        SetBkColor(hdcStatic, COLOR_BG);
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)g_hBrushBG;
    }
    
    case WM_CTLCOLORBTN:
    {
        HDC hdcBtn = (HDC)wParam;
        SetTextColor(hdcBtn, COLOR_TEXT);
        SetBkColor(hdcBtn, COLOR_BG);
        return (LRESULT)g_hBrushBG;
    }
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        int width = rcClient.right;
        int height = rcClient.bottom;
        
        // Doble buffer para evitar parpadeo
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hdcMem, hbmMem);
        
        // Fondo
        FillRect(hdcMem, &rcClient, g_hBrushBG);
        
        // Tamaño base de la imagen
        int baseImageWidth = 490;
        int baseImageHeight = 180;
        
        // Calcular escala
        float scale = (float)(width - 20) / (float)baseImageWidth;
        if (scale > 1.2f) scale = 1.2f;
        if (scale < 0.8f) scale = 0.8f;
        
        int imageDisplayWidth = (int)(baseImageWidth * scale);
        int imageDisplayHeight = (int)(baseImageHeight * scale);
        
        // Dibujar imagen del launcher centrada arriba
        if (g_hLauncherImage) {
            HDC hdcImg = CreateCompatibleDC(hdcMem);
            SelectObject(hdcImg, g_hLauncherImage);
            
            BITMAP bm;
            GetObject(g_hLauncherImage, sizeof(bm), &bm);
            
            int imgX = (width - imageDisplayWidth) / 2;
            int imgY = 10;
            
            SetStretchBltMode(hdcMem, HALFTONE);
            StretchBlt(hdcMem, imgX, imgY, imageDisplayWidth, imageDisplayHeight, 
                      hdcImg, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            
            DeleteDC(hdcImg);
        } else {
            // Si no hay imagen, mostrar texto decorativo
            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, COLOR_ACCENT);
            SelectObject(hdcMem, g_hFontTitle);
            RECT rcTitle = {0, 40, width, 80};
            DrawText(hdcMem, "HELBREATH", -1, &rcTitle, DT_CENTER | DT_SINGLELINE);
            
            SelectObject(hdcMem, g_hFontMedieval);
            SetTextColor(hdcMem, COLOR_TEXT);
            RECT rcSubtitle = {0, 75, width, 110};
            DrawText(hdcMem, "~ Apocalypse ~", -1, &rcSubtitle, DT_CENTER | DT_SINGLELINE);
        }
        
        // Borde decorativo medieval
        HPEN hPenBorder = CreatePen(PS_SOLID, 3, COLOR_BORDER);
        HPEN hPenGold = CreatePen(PS_SOLID, 1, COLOR_ACCENT);
        SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        
        // Borde exterior
        SelectObject(hdcMem, hPenBorder);
        Rectangle(hdcMem, 2, 2, width - 2, height - 2);
        
        // Borde interior dorado
        SelectObject(hdcMem, hPenGold);
        Rectangle(hdcMem, 6, 6, width - 6, height - 6);
        
        // Posición del contenido (debajo de la imagen)
        int contentStartY = imageDisplayHeight + 15;
        
        SetBkMode(hdcMem, TRANSPARENT);
        SelectObject(hdcMem, hPenGold);
        SetTextColor(hdcMem, COLOR_ACCENT);
        SelectObject(hdcMem, g_hFontMedieval);
        
        // Etiqueta "Seleccionar Reino"
        RECT rcServerLabel = {0, contentStartY, width, contentStartY + 20};
        DrawText(hdcMem, "- Seleccionar Reino -", -1, &rcServerLabel, DT_CENTER | DT_SINGLELINE);
        
        // Línea debajo de la etiqueta
        int lineY = contentStartY + 22;
        MoveToEx(hdcMem, 30, lineY, NULL);
        LineTo(hdcMem, width - 30, lineY);
        
        // ===== INDICADORES DE ESTADO DEL SERVIDOR =====
        int statusX = width / 2 + 110;
        int statusY1 = contentStartY + 30;
        int statusY2 = contentStartY + 58;
        int statusRadius = 6;
        
        // Servidor Principal - indicador
        HBRUSH hBrushStatus;
        if (g_bCheckingServer) {
            hBrushStatus = CreateSolidBrush(RGB(255, 200, 0));
        } else if (g_bServerOnline) {
            hBrushStatus = CreateSolidBrush(RGB(50, 200, 50));
        } else {
            hBrushStatus = CreateSolidBrush(RGB(200, 50, 50));
        }
        SelectObject(hdcMem, hBrushStatus);
        HPEN hPenStatus = CreatePen(PS_SOLID, 1, RGB(80, 60, 40));
        SelectObject(hdcMem, hPenStatus);
        Ellipse(hdcMem, statusX - statusRadius, statusY1 - statusRadius, 
                statusX + statusRadius, statusY1 + statusRadius);
        DeleteObject(hBrushStatus);
        
        // Servidor de Pruebas - indicador
        if (g_bCheckingServer) {
            hBrushStatus = CreateSolidBrush(RGB(255, 200, 0));
        } else if (g_bTestServerOnline) {
            hBrushStatus = CreateSolidBrush(RGB(50, 200, 50));
        } else {
            hBrushStatus = CreateSolidBrush(RGB(200, 50, 50));
        }
        SelectObject(hdcMem, hBrushStatus);
        Ellipse(hdcMem, statusX - statusRadius, statusY2 - statusRadius, 
                statusX + statusRadius, statusY2 + statusRadius);
        DeleteObject(hBrushStatus);
        DeleteObject(hPenStatus);
        
        // Texto de estado
        SelectObject(hdcMem, g_hFontNormal);
        SetTextColor(hdcMem, COLOR_TEXT);
        const char* szStatus1 = g_bCheckingServer ? "..." : (g_bServerOnline ? "Online" : "Offline");
        const char* szStatus2 = g_bCheckingServer ? "..." : (g_bTestServerOnline ? "Online" : "Offline");
        RECT rcStatus1 = {statusX + 12, statusY1 - 8, statusX + 70, statusY1 + 12};
        RECT rcStatus2 = {statusX + 12, statusY2 - 8, statusX + 70, statusY2 + 12};
        DrawText(hdcMem, szStatus1, -1, &rcStatus1, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        DrawText(hdcMem, szStatus2, -1, &rcStatus2, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        // Línea de opciones
        int optionsY = contentStartY + 85;
        SetTextColor(hdcMem, COLOR_ACCENT);
        SelectObject(hdcMem, g_hFontMedieval);
        RECT rcOptionsLabel = {0, optionsY, width, optionsY + 20};
        DrawText(hdcMem, "- Opciones -", -1, &rcOptionsLabel, DT_CENTER | DT_SINGLELINE);
        
        SelectObject(hdcMem, hPenGold);
        MoveToEx(hdcMem, 30, optionsY + 22, NULL);
        LineTo(hdcMem, width - 30, optionsY + 22);
        
        // ===== PANEL DE NOTICIAS (WebBrowser embebido - espacio reservado) =====
        // El WebBrowser se dibuja automáticamente como control hijo
        // Solo dibujamos el borde decorativo
        int newsStartY = optionsY + 75;
        int newsPanelHeight = height - newsStartY - 70;
        
        if (newsPanelHeight > 80) {
            // Borde decorativo alrededor del WebBrowser
            SelectObject(hdcMem, hPenGold);
            Rectangle(hdcMem, 18, newsStartY - 2, width - 18, newsStartY + newsPanelHeight + 2);
        }
        
        DeleteObject(hPenBorder);
        DeleteObject(hPenGold);
        
        // ===== BOTONES =====
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 60;
        
        // Botón JUGAR (izquierda)
        RECT rcPlay = {width / 2 - btnWidth - 20, btnY, width / 2 - 20, btnY + btnHeight};
        DrawCustomButton(hdcMem, &rcPlay, "ENTRAR AL REINO", bPlayHover, TRUE);
        
        // Botón Salir (derecha)
        int exitBtnWidth = 100;
        RECT rcExit = {width / 2 + 20, btnY + 6, width / 2 + 20 + exitBtnWidth, btnY + 34};
        DrawCustomButton(hdcMem, &rcExit, "Salir", bExitHover, FALSE);
        
        // Copiar buffer a pantalla
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        
        EndPaint(hWnd, &ps);
        return 0;
    }
    
    case WM_NCHITTEST:
    {
        // Permitir redimensionar desde los bordes
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ScreenToClient(hWnd, &pt);
            
            RECT rc;
            GetClientRect(hWnd, &rc);
            
            int border = 8;
            
            // Esquinas
            if (pt.x < border && pt.y < border) return HTTOPLEFT;
            if (pt.x > rc.right - border && pt.y < border) return HTTOPRIGHT;
            if (pt.x < border && pt.y > rc.bottom - border) return HTBOTTOMLEFT;
            if (pt.x > rc.right - border && pt.y > rc.bottom - border) return HTBOTTOMRIGHT;
            
            // Bordes
            if (pt.x < border) return HTLEFT;
            if (pt.x > rc.right - border) return HTRIGHT;
            if (pt.y < border) return HTTOP;
            if (pt.y > rc.bottom - border) return HTBOTTOM;
            
            // Área de título para arrastrar
            if (pt.y < 30) return HTCAPTION;
        }
        return hit;
    }
    
    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right;
        int height = rc.bottom;
        
        // Posiciones de botones (centrados abajo)
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 60;
        int exitBtnWidth = 100;
        
        BOOL newPlayHover = (x >= width / 2 - btnWidth - 20 && x <= width / 2 - 20 && 
                            y >= btnY && y <= btnY + btnHeight);
        BOOL newExitHover = (x >= width / 2 + 20 && x <= width / 2 + 20 + exitBtnWidth && 
                            y >= btnY + 6 && y <= btnY + 34);
        
        if (newPlayHover != bPlayHover || newExitHover != bExitHover) {
            bPlayHover = newPlayHover;
            bExitHover = newExitHover;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        
        RECT rc;
        GetClientRect(hWnd, &rc);
        int width = rc.right;
        int height = rc.bottom;
        
        // Posiciones de botones (centrados abajo)
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 60;
        int exitBtnWidth = 100;
        
        // Click en JUGAR
        if (x >= width / 2 - btnWidth - 20 && x <= width / 2 - 20 && 
            y >= btnY && y <= btnY + btnHeight) {
            g_bBorderlessMode = (SendMessage(hCheckBorderless, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_bTestServer = (SendMessage(hRadioTest, BM_GETCHECK, 0, 0) == BST_CHECKED);
            
            // Guardar tamaño actual de la ventana
            RECT rcWnd;
            GetClientRect(hWnd, &rcWnd);
            g_iLauncherWidth = rcWnd.right;
            g_iLauncherHeight = rcWnd.bottom;
            
            SaveLauncherSettings();
            WriteLoginConfig();
            
            // Ejecutar el juego
            if (LaunchGame()) {
                g_bLauncherResult = TRUE;
                DestroyWindow(hWnd);
            }
        }
        // Click en Salir
        else if (x >= width / 2 + 20 && x <= width / 2 + 20 + exitBtnWidth && 
                 y >= btnY + 6 && y <= btnY + 34) {
            // Guardar tamaño antes de salir
            RECT rcWnd;
            GetClientRect(hWnd, &rcWnd);
            g_iLauncherWidth = rcWnd.right;
            g_iLauncherHeight = rcWnd.bottom;
            SaveLauncherSettings();
            
            g_bLauncherResult = FALSE;
            DestroyWindow(hWnd);
        }
        return 0;
    }
    
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_CHECKBOX_BORDERLESS) {
            g_bBorderlessMode = (SendMessage(hCheckBorderless, BM_GETCHECK, 0, 0) == BST_CHECKED);
        }
        else if (LOWORD(wParam) == ID_RADIO_ONLINE || LOWORD(wParam) == ID_RADIO_TEST) {
            g_bTestServer = (SendMessage(hRadioTest, BM_GETCHECK, 0, 0) == BST_CHECKED);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    
    case WM_DESTROY:
        // Detener timer
        KillTimer(hWnd, ID_TIMER_SERVER_CHECK);
        
        // Destruir WebBrowser embebido
        DestroyEmbeddedBrowser();
        
        // Desinicializar OLE
        OleUninitialize();
        
        // Limpiar recursos
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontSmall) DeleteObject(g_hFontSmall);
        if (g_hFontButton) DeleteObject(g_hFontButton);
        if (g_hFontMedieval) DeleteObject(g_hFontMedieval);
        if (g_hFontNews) DeleteObject(g_hFontNews);
        if (g_hBrushBG) DeleteObject(g_hBrushBG);
        if (g_hBrushBGLight) DeleteObject(g_hBrushBGLight);
        if (g_hLauncherImage) DeleteObject(g_hLauncherImage);
        if (g_hTooltip) DestroyWindow(g_hTooltip);
        PostQuitMessage(0);
        return 0;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Punto de entrada principal
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow)
{
    g_hInstance = hInstance;
    
    // Inicializar WinSock para verificación de servidor
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        MessageBox(NULL, "Error al inicializar WinSock", "Error", MB_ICONERROR);
        return 1;
    }
    
    // DPI Awareness - Evita escalado borroso en monitores HiDPI
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);
        SetProcessDPIAwareFunc pSetProcessDPIAware = 
            (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (pSetProcessDPIAware) pSetProcessDPIAware();
    }
    
    // Cargar configuración previa
    LoadLauncherSettings();
    
    // Inicializar controles comunes
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Registrar clase de ventana del launcher
    WNDCLASSEX wcLauncher = {0};
    wcLauncher.cbSize = sizeof(WNDCLASSEX);
    wcLauncher.style = CS_HREDRAW | CS_VREDRAW;
    wcLauncher.lpfnWndProc = LauncherWndProc;
    wcLauncher.hInstance = hInstance;
    wcLauncher.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcLauncher.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcLauncher.lpszClassName = "HelbreathLauncher";
    wcLauncher.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcLauncher.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    
    if (!RegisterClassEx(&wcLauncher)) {
        MessageBox(NULL, "Error al registrar la clase de ventana", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Calcular tamaño de ventana con bordes
    RECT rcWindow = {0, 0, g_iLauncherWidth, g_iLauncherHeight};
    AdjustWindowRectEx(&rcWindow, WS_POPUP | WS_THICKFRAME, FALSE, WS_EX_LAYERED);
    int windowW = rcWindow.right - rcWindow.left;
    int windowH = rcWindow.bottom - rcWindow.top;
    
    // Centrar ventana en pantalla
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - windowW) / 2;
    int posY = (screenH - windowH) / 2;
    
    // Crear ventana del launcher
    HWND hLauncher = CreateWindowEx(
        WS_EX_LAYERED,
        "HelbreathLauncher",
        "Helbreath Apocalypse - Launcher",
        WS_POPUP | WS_VISIBLE | WS_THICKFRAME,
        posX, posY, windowW, windowH,
        NULL, NULL, hInstance, NULL);
    
    if (!hLauncher) {
        MessageBox(NULL, "Error al crear la ventana del launcher", "Error", MB_ICONERROR);
        return 1;
    }
    
    // Hacer la ventana ligeramente transparente
    SetLayeredWindowAttributes(hLauncher, 0, 245, LWA_ALPHA);
    
    // Loop de mensajes
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Limpiar WinSock
    WSACleanup();
    
    UnregisterClass("HelbreathLauncher", hInstance);
    
    return 0;
}
