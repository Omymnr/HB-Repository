// -------------------------------------------------------------- 
//                 Helbreath Apocalypse Launcher
//
//          Launcher separado que ejecuta Game.exe
// --------------------------------------------------------------

// Evita advertencias de funciones "inseguras" de la CRT (strcpy, strcat, sprintf, fopen...)
// DEBE ir ANTES de los includes
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4996)

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <winbase.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <shellapi.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

// ===== LAUNCHER CONFIGURATION =====
#define LAUNCHER_DEFAULT_WIDTH 500
#define LAUNCHER_DEFAULT_HEIGHT 400
#define LAUNCHER_MIN_WIDTH 400
#define LAUNCHER_MIN_HEIGHT 350
#define ID_CHECKBOX_BORDERLESS 1001
#define ID_BUTTON_PLAY 1002
#define ID_BUTTON_EXIT 1003
#define ID_RADIO_ONLINE 1004
#define ID_RADIO_TEST 1005

// Colores del tema medieval
#define COLOR_BG RGB(25, 20, 15)              // Marrón muy oscuro
#define COLOR_BG_LIGHT RGB(45, 35, 25)        // Marrón oscuro
#define COLOR_TEXT RGB(210, 190, 150)         // Beige/pergamino
#define COLOR_ACCENT RGB(180, 140, 60)        // Dorado
#define COLOR_BUTTON RGB(60, 45, 30)          // Marrón botón
#define COLOR_BUTTON_HOVER RGB(180, 140, 60)  // Dorado hover
#define COLOR_BORDER RGB(100, 75, 45)         // Borde marrón

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
HFONT g_hFontButton = NULL;
HFONT g_hFontMedieval = NULL;
HBITMAP g_hLauncherImage = NULL;
HWND g_hTooltip = NULL;
HINSTANCE g_hInstance = NULL;

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
        // Cargar imagen del launcher
        g_hLauncherImage = LoadLauncherImage();
        
        // Crear fuentes medievales
        g_hFontTitle = CreateFont(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Times New Roman");
        g_hFontMedieval = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DECORATIVE, "Times New Roman");
        g_hFontNormal = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        g_hFontButton = CreateFont(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        
        // Crear brushes
        g_hBrushBG = CreateSolidBrush(COLOR_BG);
        g_hBrushBGLight = CreateSolidBrush(COLOR_BG_LIGHT);
        
        // Calcular posiciones basadas en tamaño de ventana
        RECT rc;
        GetClientRect(hWnd, &rc);
        int centerX = rc.right / 2;
        
        // Calcular escala de imagen
        float scale = (float)(rc.right - 20) / 490.0f;
        if (scale > 1.5f) scale = 1.5f;
        if (scale < 0.8f) scale = 0.8f;
        int imageDisplayHeight = (int)(180 * scale);
        
        // Los controles van debajo de la etiqueta + línea
        int contentY = imageDisplayHeight + 25 + 25;
        
        // Radio buttons para selección de servidor
        hRadioOnline = CreateWindow("BUTTON", "  Servidor Principal",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
            centerX - 100, contentY, 200, 25, hWnd, (HMENU)ID_RADIO_ONLINE, NULL, NULL);
        SendMessage(hRadioOnline, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        hRadioTest = CreateWindow("BUTTON", "  Servidor de Pruebas",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            centerX - 100, contentY + 28, 200, 25, hWnd, (HMENU)ID_RADIO_TEST, NULL, NULL);
        SendMessage(hRadioTest, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        
        // Seleccionar el radio button según configuración guardada
        SendMessage(g_bTestServer ? hRadioTest : hRadioOnline, BM_SETCHECK, BST_CHECKED, 0);
        
        // Checkbox para modo ventana
        int optionsY = contentY + 95 + 25;
        hCheckBorderless = CreateWindow("BUTTON", "  Modo Ventana",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            centerX - 100, optionsY, 200, 25, hWnd, (HMENU)ID_CHECKBOX_BORDERLESS, NULL, NULL);
        SendMessage(hCheckBorderless, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(hCheckBorderless, BM_SETCHECK, g_bBorderlessMode ? BST_CHECKED : BST_UNCHECKED, 0);
        
        // Crear tooltip
        g_hTooltip = CreateTooltip(hWnd, hCheckBorderless, "Modo ventana sin bordes");
        
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
            
            // Recalcular escala de imagen
            float scale = (float)(g_iLauncherWidth - 20) / 490.0f;
            if (scale > 1.5f) scale = 1.5f;
            if (scale < 0.8f) scale = 0.8f;
            int imageDisplayHeight = (int)(180 * scale);
            
            // Reposicionar controles
            int centerX = g_iLauncherWidth / 2;
            int contentY = imageDisplayHeight + 25 + 25;
            int optionsY = contentY + 95 + 25;
            
            if (hRadioOnline) SetWindowPos(hRadioOnline, NULL, centerX - 100, contentY, 200, 25, SWP_NOZORDER);
            if (hRadioTest) SetWindowPos(hRadioTest, NULL, centerX - 100, contentY + 28, 200, 25, SWP_NOZORDER);
            if (hCheckBorderless) SetWindowPos(hCheckBorderless, NULL, centerX - 100, optionsY, 200, 25, SWP_NOZORDER);
            
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
        if (scale > 1.5f) scale = 1.5f;
        if (scale < 0.8f) scale = 0.8f;
        
        int imageDisplayWidth = (int)(baseImageWidth * scale);
        int imageDisplayHeight = (int)(baseImageHeight * scale);
        
        // Dibujar imagen del launcher
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
        
        // Posición del contenido
        int contentStartY = imageDisplayHeight + 25;
        
        // Líneas decorativas y etiquetas
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
        
        // Línea de opciones
        int optionsY = contentStartY + 95;
        
        // Etiqueta "Opciones"
        RECT rcOptionsLabel = {0, optionsY, width, optionsY + 20};
        DrawText(hdcMem, "- Opciones -", -1, &rcOptionsLabel, DT_CENTER | DT_SINGLELINE);
        
        // Línea debajo de la etiqueta
        int optionsLineY = optionsY + 22;
        MoveToEx(hdcMem, 30, optionsLineY, NULL);
        LineTo(hdcMem, width - 30, optionsLineY);
        
        DeleteObject(hPenBorder);
        DeleteObject(hPenGold);
        
        // Calcular posición de botones
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 90;
        int btnExitY = height - 40;
        
        // Botón JUGAR
        RECT rcPlay = {(width - btnWidth) / 2, btnY, (width + btnWidth) / 2, btnY + btnHeight};
        DrawCustomButton(hdcMem, &rcPlay, "ENTRAR AL REINO", bPlayHover, TRUE);
        
        // Botón Salir
        int exitBtnWidth = 100;
        RECT rcExit = {(width - exitBtnWidth) / 2, btnExitY, (width + exitBtnWidth) / 2, btnExitY + 28};
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
        
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 90;
        int btnExitY = height - 40;
        int exitBtnWidth = 100;
        
        BOOL newPlayHover = (x >= (width - btnWidth) / 2 && x <= (width + btnWidth) / 2 && 
                            y >= btnY && y <= btnY + btnHeight);
        BOOL newExitHover = (x >= (width - exitBtnWidth) / 2 && x <= (width + exitBtnWidth) / 2 && 
                            y >= btnExitY && y <= btnExitY + 28);
        
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
        
        int btnWidth = 150;
        int btnHeight = 40;
        int btnY = height - 90;
        int btnExitY = height - 40;
        int exitBtnWidth = 100;
        
        // Click en JUGAR
        if (x >= (width - btnWidth) / 2 && x <= (width + btnWidth) / 2 && 
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
        else if (x >= (width - exitBtnWidth) / 2 && x <= (width + exitBtnWidth) / 2 && 
                 y >= btnExitY && y <= btnExitY + 28) {
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
        // Limpiar recursos
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontButton) DeleteObject(g_hFontButton);
        if (g_hFontMedieval) DeleteObject(g_hFontMedieval);
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
    
    UnregisterClass("HelbreathLauncher", hInstance);
    
    return 0;
}
