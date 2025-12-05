// -------------------------------------------------------------- 
//                      Helbreath Client 						  
//
//                      1998.10 by Soph
//
// --------------------------------------------------------------


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <winbase.h>
#include <mmsystem.h>
#include <process.h>
#include <intrin.h>  // Para _mm_pause()
#include <commctrl.h> // Para controles modernos
#include <gdiplus.h>   // Para cargar PNG
#include <DbgHelp.h>
#include "resource.h"
#include "XSocket.h"
#include "winmain.h"
#include "Game.h"
#include "GlobalDef.h"
#include "RendererBridge.h"
#include "RendererConfig.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Dbghelp.lib")

// ===== FORZAR USO DE GPU DEDICADA (NVIDIA/AMD) =====
// Estos exports hacen que los drivers de NVIDIA y AMD 
// detecten automáticamente que esta aplicación necesita la GPU dedicada
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;                  // NVIDIA Optimus
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // AMD PowerXpress/Enduro
}

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

// Variables cacheadas para el tamaño del cliente (evita llamar GetClientRect cada frame)
int g_iCachedClientWidth = 800;
int g_iCachedClientHeight = 600;

// ===== CONFIGURACIÓN DE SERVIDORES =====
// Server Online (para amigos conectando remotamente)
#define ONLINE_SERVER_IP "89.7.69.125"
#define ONLINE_SERVER_PORT 2500

// Test Server (para pruebas en red local)
#define TEST_SERVER_IP "192.168.0.15"
#define TEST_SERVER_PORT 2500

// Clave del registro para guardar configuración
#define LAUNCHER_REG_KEY "SOFTWARE\\HelbreathApocalypse\\Launcher"

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
        // Imagen base: 490x140, contentStartY = imageDisplayHeight + 25 + 22 (linea)
        RECT rc;
        GetClientRect(hWnd, &rc);
        int centerX = rc.right / 2;
        
        // Calcular escala de imagen
        float scale = (float)(rc.right - 20) / 490.0f;
        if (scale > 1.5f) scale = 1.5f;
        if (scale < 0.8f) scale = 0.8f;
        int imageDisplayHeight = (int)(180 * scale);
        
        // Los controles van debajo de la etiqueta + línea
        int contentY = imageDisplayHeight + 25 + 25;  // imagen + etiqueta + linea + margen
        
        // Radio buttons para selección de servidor (sin IPs visibles)
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
        
        // Checkbox para modo ventana (debajo de la segunda línea divisoria)
        // optionsY en paint = contentStartY + 95, luego + 22 para la linea + margen
        int optionsY = contentY + 95 + 25;
        hCheckBorderless = CreateWindow("BUTTON", "  Modo Ventana",
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            centerX - 100, optionsY, 200, 25, hWnd, (HMENU)ID_CHECKBOX_BORDERLESS, NULL, NULL);
        SendMessage(hCheckBorderless, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
        SendMessage(hCheckBorderless, BM_SETCHECK, g_bBorderlessMode ? BST_CHECKED : BST_UNCHECKED, 0);
        
        // Crear tooltip para el checkbox de modo ventana
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
        
        // Tamaño base de la imagen: 490x180, se escala proporcionalmente con la ventana
        int baseImageWidth = 490;
        int baseImageHeight = 180;
        
        // Calcular escala basada en el ancho de la ventana
        float scale = (float)(width - 20) / (float)baseImageWidth;
        if (scale > 1.5f) scale = 1.5f;  // Limitar escala máxima
        if (scale < 0.8f) scale = 0.8f;  // Limitar escala mínima
        
        int imageDisplayWidth = (int)(baseImageWidth * scale);
        int imageDisplayHeight = (int)(baseImageHeight * scale);
        
        // Dibujar imagen del launcher en la parte superior (estilo Olympia)
        if (g_hLauncherImage) {
            HDC hdcImg = CreateCompatibleDC(hdcMem);
            SelectObject(hdcImg, g_hLauncherImage);
            
            BITMAP bm;
            GetObject(g_hLauncherImage, sizeof(bm), &bm);
            
            // Centrar horizontalmente
            int imgX = (width - imageDisplayWidth) / 2;
            int imgY = 10;
            
            // Dibujar imagen escalada con alta calidad
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
        int contentStartY = imageDisplayHeight + 25;
        
        // Líneas decorativas y etiquetas
        SetBkMode(hdcMem, TRANSPARENT);
        SelectObject(hdcMem, hPenGold);
        SetTextColor(hdcMem, COLOR_ACCENT);
        SelectObject(hdcMem, g_hFontMedieval);
        
        // Etiqueta "Seleccionar Reino" (ENCIMA de la línea)
        RECT rcServerLabel = {0, contentStartY, width, contentStartY + 20};
        DrawText(hdcMem, "- Seleccionar Reino -", -1, &rcServerLabel, DT_CENTER | DT_SINGLELINE);
        
        // Línea debajo de la etiqueta
        int lineY = contentStartY + 22;
        MoveToEx(hdcMem, 30, lineY, NULL);
        LineTo(hdcMem, width - 30, lineY);
        
        // Línea de opciones
        int optionsY = contentStartY + 95;
        
        // Etiqueta "Opciones" (ENCIMA de la línea)
        RECT rcOptionsLabel = {0, optionsY, width, optionsY + 20};
        DrawText(hdcMem, "- Opciones -", -1, &rcOptionsLabel, DT_CENTER | DT_SINGLELINE);
        
        // Línea debajo de la etiqueta
        int optionsLineY = optionsY + 22;
        MoveToEx(hdcMem, 30, optionsLineY, NULL);
        LineTo(hdcMem, width - 30, optionsLineY);
        
        DeleteObject(hPenBorder);
        DeleteObject(hPenGold);
        
        // Calcular posición de botones basado en tamaño actual
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
            
            // Área de título para arrastrar (parte superior)
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
            g_bLauncherResult = TRUE;
            DestroyWindow(hWnd);
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

// Mostrar el Launcher y esperar resultado
BOOL ShowLauncher(HINSTANCE hInstance)
{
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
        return FALSE;
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
    
    // Crear ventana del launcher (con bordes para redimensionar)
    HWND hLauncher = CreateWindowEx(
        WS_EX_LAYERED,
        "HelbreathLauncher",
        "Helbreath Apocalypse",
        WS_POPUP | WS_VISIBLE | WS_THICKFRAME,
        posX, posY, windowW, windowH,
        NULL, NULL, hInstance, NULL);
    
    if (!hLauncher) {
        return FALSE;
    }
    
    // Hacer la ventana ligeramente transparente para efecto moderno
    SetLayeredWindowAttributes(hLauncher, 0, 245, LWA_ALPHA);
    
    // Loop de mensajes del launcher
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    UnregisterClass("HelbreathLauncher", hInstance);
    
    return g_bLauncherResult;
}

extern "C" __declspec( dllimport) int __FindHackingDll__(char *);

// --------------------------------------------------------------

#define WM_USER_TIMERSIGNAL		WM_USER + 500
#define WM_USER_CALCSOCKETEVENT WM_USER + 600

int				G_iAddTable31[64][510], G_iAddTable63[64][510]; 
int				G_iAddTransTable31[510][64], G_iAddTransTable63[510][64]; 

long    G_lTransG100[64][64], G_lTransRB100[64][64];
long    G_lTransG70[64][64], G_lTransRB70[64][64];
long    G_lTransG50[64][64], G_lTransRB50[64][64];
long    G_lTransG25[64][64], G_lTransRB25[64][64];
long    G_lTransG2[64][64], G_lTransRB2[64][64];

char			szAppClass[32];
HWND			G_hWnd = NULL;
HWND			G_hEditWnd = NULL;
HINSTANCE       G_hInstance = NULL;
MMRESULT		G_mmTimer;
char   G_cSpriteAlphaDegree;
class CGame * G_pGame;
class XSocket * G_pCalcSocket = NULL;
BOOL  G_bIsCalcSocketConnected = TRUE;
DWORD G_dwCalcSocketTime = NULL, G_dwCalcSocketSendTime = NULL;

char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120], G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];

// ===== TIMER DE ALTA PRECISIÓN PARA CPUs MODERNAS =====
LARGE_INTEGER G_PerformanceFrequency;
BOOL G_bUseHighPrecisionTimer = FALSE;

// ===== FPS LIMITER =====
#define TARGET_FPS 144
#define TARGET_FRAME_TIME_MS (1000.0 / TARGET_FPS)  // ~6.944ms per frame
LARGE_INTEGER G_LastFrameTime;

// Función de tiempo de alta precisión - usa QueryPerformanceCounter si está disponible
inline DWORD GetHighPrecisionTime()
{
	if (G_bUseHighPrecisionTimer) {
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return (DWORD)((counter.QuadPart * 1000) / G_PerformanceFrequency.QuadPart);
	}
	return timeGetTime();
}

// Función para obtener tiempo en microsegundos (más preciso para frame limiting)
inline double GetHighPrecisionTimeUs()
{
	if (G_bUseHighPrecisionTimer) {
		LARGE_INTEGER counter;
		QueryPerformanceCounter(&counter);
		return (double)(counter.QuadPart * 1000000.0) / G_PerformanceFrequency.QuadPart;
	}
	return (double)timeGetTime() * 1000.0;
}

// Frame limiter preciso - limita a TARGET_FPS
inline void LimitFrameRate()
{
	if (!G_bUseHighPrecisionTimer) {
		// Fallback: usar Sleep simple si no hay timer de alta precisión
		Sleep(1);
		return;
	}
	
	static double lastFrameTimeUs = 0;
	static const double targetFrameTimeUs = 1000000.0 / TARGET_FPS;  // microsegundos por frame
	
	double currentTimeUs = GetHighPrecisionTimeUs();
	double elapsedUs = currentTimeUs - lastFrameTimeUs;
	double remainingUs = targetFrameTimeUs - elapsedUs;
	
	if (remainingUs > 0) {
		// Si queda más de 1.5ms, dormir para ahorrar CPU
		if (remainingUs > 1500) {
			Sleep((DWORD)((remainingUs - 1000) / 1000));  // Dejar 1ms de margen
		}
		
		// Spin-wait para el tiempo restante (más preciso que Sleep)
		while (GetHighPrecisionTimeUs() - lastFrameTimeUs < targetFrameTimeUs) {
			// Busy wait - muy preciso pero usa CPU
			_mm_pause();  // Pausa el CPU brevemente para reducir consumo
		}
	}
	
	lastFrameTimeUs = GetHighPrecisionTimeUs();
}

// --------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam, LPARAM lParam)
{ 
	if(G_pGame->GetText( hWnd, message, wParam, lParam)) return 0;

	switch (message) {
    case WM_INPUT:
        // route raw input to DXC_dinput handler
        if (G_pGame) G_pGame->m_DInput.HandleRawInput(lParam);
        break;
    
    // High-frequency cursor update for smoother movement on 144Hz+ monitors
    case WM_MOUSEMOVE:
        if (G_pGame) {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // Usar tamaño cacheado en lugar de llamar GetClientRect cada frame
            G_pGame->m_DInput.UpdateFromWindowsMessage(x, y, g_iCachedClientWidth, g_iCachedClientHeight);
        }
        break;
    
    // Actualizar tamaño cacheado cuando cambia la ventana
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            g_iCachedClientWidth = LOWORD(lParam);
            g_iCachedClientHeight = HIWORD(lParam);
            if (g_iCachedClientWidth <= 0) g_iCachedClientWidth = 800;
            if (g_iCachedClientHeight <= 0) g_iCachedClientHeight = 600;
        }
        break;
    
    // Mouse wheel support for windowed/borderless mode
    case WM_MOUSEWHEEL:
        if (G_pGame) {
            short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            G_pGame->m_DInput.m_sZ = zDelta;
        }
        break;
        
	case WM_USER_CALCSOCKETEVENT:
		G_pGame->_CalcSocketClosed();
		break;
	
	case WM_CLOSE:
		if ( (G_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINGAME) && ( G_pGame->m_bForceDisconn == FALSE ) )
		{
			// Alt+F4 durante el juego: iniciar proceso de logout, NO cerrar ventana
			if (strcmp(G_pGame->m_cMapName, "cityhall_1") == 0 || strcmp(G_pGame->m_cMapName, "cityhall_2") == 0 ||
				strcmp(G_pGame->m_cMapName, "bsmith_1") == 0 || strcmp(G_pGame->m_cMapName, "bsmith_1f") == 0 ||
				strcmp(G_pGame->m_cMapName, "bsmith_2") == 0 || strcmp(G_pGame->m_cMapName, "bsmith_2f") == 0 ||
				strcmp(G_pGame->m_cMapName, "gshop_1") == 0 || strcmp(G_pGame->m_cMapName, "gshop_1f") == 0 ||
				strcmp(G_pGame->m_cMapName, "gshop_2") == 0 || strcmp(G_pGame->m_cMapName, "gshop_2f") == 0 ||
				strcmp(G_pGame->m_cMapName, "arewrhus") == 0 || strcmp(G_pGame->m_cMapName, "elvwrhus") == 0 ||
				strcmp(G_pGame->m_cMapName, "wrhus_1") == 0 || strcmp(G_pGame->m_cMapName, "wrhus_1f") == 0 ||
				strcmp(G_pGame->m_cMapName, "wrhus_2") == 0 || strcmp(G_pGame->m_cMapName, "wrhus_2f") == 0) {

				if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > 1) G_pGame->m_cLogOutCount = 1;
			}
			else {
#ifdef _DEBUG
				if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > 2) G_pGame->m_cLogOutCount = 1;
#else
				if (G_pGame->m_cLogOutCount == -1 || G_pGame->m_cLogOutCount > 11) G_pGame->m_cLogOutCount = 11;
#endif
			}
			return 0;  // Bloquear cierre de ventana, el logout se procesará normalmente
		}
		else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING) {
			return 0;  // Bloquear cierre durante la carga
		}
		else if (G_pGame->m_cGameMode == DEF_GAMEMODE_ONMAINMENU) {
			G_pGame->ChangeGameMode(DEF_GAMEMODE_ONQUIT);
			return 0;
		}
		// En otros modos (QUIT, etc), permitir cierre normal
		break;
	
	case WM_SYSCOMMAND:
		if((wParam&0xFFF0)==SC_SCREENSAVE || (wParam&0xFFF0)==SC_MONITORPOWER) 
			return 0; 
		return DefWindowProc(hWnd, message, wParam, lParam);
			
	case WM_USER_TIMERSIGNAL:
		G_pGame->OnTimer();
		break;

	case WM_KEYDOWN:
		G_pGame->OnKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		
	case WM_KEYUP:
		G_pGame->OnKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_SYSKEYDOWN:
		G_pGame->OnSysKeyDown(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_SYSKEYUP:
		G_pGame->OnSysKeyUp(wParam);
		return (DefWindowProc(hWnd, message, wParam, lParam));
		break;

	case WM_ACTIVATEAPP:
		{
			char dbgMsg[256];
			sprintf(dbgMsg, "[HB-WMAIN] WM_ACTIVATEAPP wParam=%d (0=deactivate, 1=activate)\\n", (int)wParam);
			OutputDebugStringA(dbgMsg);
		}
		
		if( wParam == 0 ) 
		{	// Aplicación desactivada (Alt+Tab fuera)
			OutputDebugStringA("[HB-WMAIN] === APP DESACTIVADA (Alt+Tab out) ===\\n");
			G_pGame->m_bIsProgramActive = FALSE;
			G_pGame->m_DInput.SetAcquire(FALSE);
			G_pGame->m_DInput.SetProgramActive(FALSE);  // Ignorar clics cuando no tiene el foco
			
			// Notificar al renderer D3D11 que perdimos el foco
			if (CRendererBridge::GetInstance().IsUsingD3D11()) {
				OutputDebugStringA("[HB-WMAIN] Llamando OnAppActivate(FALSE)...\\n");
				CRendererBridge::GetInstance().OnAppActivate(FALSE);
				OutputDebugStringA("[HB-WMAIN] OnAppActivate(FALSE) completado\\n");
			} else {
				OutputDebugStringA("[HB-WMAIN] No usando D3D11, no se llama OnAppActivate\\n");
			}
		}else 
		{	// Aplicación activada (volviendo al juego)
			OutputDebugStringA("[HB-WMAIN] === APP ACTIVADA (volviendo) ===\\n");
			G_pGame->m_bIsProgramActive = TRUE;
			G_pGame->m_DInput.SetAcquire(TRUE);
			G_pGame->m_DInput.SetProgramActive(TRUE);  // Aceptar clics de nuevo
			G_pGame->m_bCtrlPressed = FALSE;
			G_pGame->m_bIsRedrawPDBGS = TRUE;
			
			// Notificar al renderer D3D11 que recuperamos el foco
			if (CRendererBridge::GetInstance().IsUsingD3D11()) {
				OutputDebugStringA("[HB-WMAIN] Llamando OnAppActivate(TRUE)...\\n");
				CRendererBridge::GetInstance().OnAppActivate(TRUE);
				OutputDebugStringA("[HB-WMAIN] OnAppActivate(TRUE) completado\\n");
			} else {
				// Solo DirectDraw necesita ChangeDisplayMode
				OutputDebugStringA("[HB-WMAIN] Usando DirectDraw, llamando ChangeDisplayMode...\\n");
				G_pGame->m_DDraw.ChangeDisplayMode(G_hWnd);
				OutputDebugStringA("[HB-WMAIN] ChangeDisplayMode completado\\n");
			}

			if (G_pGame->bCheckImportantFile() == FALSE) 
			{	MessageBox(G_pGame->m_hWnd, "File checksum error! Get Update again please!", "ERROR1", MB_ICONEXCLAMATION | MB_OK);
				PostQuitMessage(0);
				return 0;
			}			
			if (__FindHackingDll__("CRCCHECK") != 1) 
			{	G_pGame->ChangeGameMode(DEF_GAMEMODE_ONQUIT);
				return NULL;
		}	}
		return DefWindowProc(hWnd, message, wParam, lParam);

	case WM_SETCURSOR:
		SetCursor(NULL);
		return TRUE;

	case WM_DESTROY:
		OnDestroy();
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
		
	case WM_USER_GAMESOCKETEVENT:
		G_pGame->OnGameSocketEvent(wParam, lParam);
		break;

	case WM_USER_LOGSOCKETEVENT:
		G_pGame->OnLogSocketEvent(wParam, lParam);
		break;
		
	default: 
		return (DefWindowProc(hWnd, message, wParam, lParam));
	}	
	return NULL;
}

// --- Crash handler: write minidump on unhandled exception ---
LONG WINAPI HelbreathUnhandledExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
    const char* dumpPath = "helbreath_crash.dmp";
    const char* logPath = "helbreath_crash.log";
    HANDLE hFile = CreateFileA(dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = pExceptionInfo;
        mei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
        CloseHandle(hFile);
    }
    FILE* f = fopen(logPath, "a");
    if (f) {
        fprintf(f, "Unhandled exception at time %u\n", (unsigned)time(NULL));
        if (pExceptionInfo && pExceptionInfo->ExceptionRecord)
            fprintf(f, "ExceptionCode=0x%08X, Address=%p\n", (unsigned)pExceptionInfo->ExceptionRecord->ExceptionCode, pExceptionInfo->ExceptionRecord->ExceptionAddress);
        fclose(f);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
               LPSTR lpCmdLine, int nCmdShow )
{
    FILE* fLog = fopen("helbreath_boot.log", "w");
    if (fLog) { fprintf(fLog, "[WinMain] Inicio launcher\n"); fclose(fLog); }

    // ===== OPTIMIZACIONES PARA HARDWARE MODERNO =====
	
	// 1. DPI Awareness - Evita escalado borroso en monitores HiDPI (4K, 1440p, etc)
	HMODULE hUser32 = GetModuleHandleA("user32.dll");
	if (hUser32) {
		typedef BOOL (WINAPI *SetProcessDPIAwareFunc)(void);
		SetProcessDPIAwareFunc pSetProcessDPIAware = 
			(SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");
		if (pSetProcessDPIAware) pSetProcessDPIAware();
	}
	
	// 2. Deshabilitar Priority Boost para timing más consistente en CPUs modernas
	SetProcessPriorityBoost(GetCurrentProcess(), TRUE);
	
	// 3. Prioridad Above Normal - mejor respuesta en sistemas modernos multi-tarea
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	
	// 4. Afinidad de CPU: Asignar núcleos 4-15 (resto de P-cores y E-cores) al cliente para gráficos
	//SetProcessAffinityMask(GetCurrentProcess(), 0xFFF0);  // Núcleos 4-15
	
	// 5. Inicializar timer de alta precisión si está disponible
	if (QueryPerformanceFrequency(&G_PerformanceFrequency)) {
		G_bUseHighPrecisionTimer = TRUE;
	}
	
	// ===== FIN OPTIMIZACIONES =====

	srand((unsigned)time(NULL));
	char *pJammer = new char[(rand() % 100) +1];
	
    // ===== LAUNCHER DESHABILITADO - IR DIRECTO AL JUEGO =====
	// El launcher ahora es un ejecutable separado (Launcher.exe)
	// Game.exe siempre inicia directamente el juego
	
	// Cargar configuración guardada del registro (del Launcher externo)
	LoadLauncherSettings();
	
	// Permitir sobreescribir con parámetros de línea de comandos
	if (lpCmdLine != NULL && strlen(lpCmdLine) > 0) {
		if (strstr(lpCmdLine, "-borderless") != NULL || 
		    strstr(lpCmdLine, "/borderless") != NULL) {
			g_bBorderlessMode = TRUE;
		}
		else if (strstr(lpCmdLine, "-fullscreen") != NULL || 
		         strstr(lpCmdLine, "/fullscreen") != NULL) {
			g_bBorderlessMode = FALSE;
		}
	}
	
    // Crear el juego después del launcher
    fLog = fopen("helbreath_boot.log", "a");
    if (fLog) { fprintf(fLog, "[WinMain] Creando objeto CGame\n"); fclose(fLog); }
    G_pGame = new class CGame;

    // Declaraciones necesarias para DLL
    HINSTANCE hDll;
    char cSearchDll[] = "rd`qbg-ckk";
    char cRealName[12];

    ZeroMemory(cRealName, sizeof(cRealName));
    strcpy(cRealName, cSearchDll);
    for (WORD i = 0; i < strlen(cRealName); i++)
        if (cRealName[i] != NULL) cRealName[i]++;

    hDll = LoadLibrary(cRealName);
    if( hDll == NULL ) 
    {   MessageBox(NULL, "don't find search.dll", "ERROR!", MB_OK);
        return 0;
    }

#ifdef DEF_USING_WIN_IME
	HINSTANCE hRichDll = LoadLibrary( "Riched20.dll" );
#endif

	typedef int (MYPROC)(char *) ;
	MYPROC *pFindHook; 
	pFindHook = (MYPROC *) GetProcAddress(hDll, "__FindHackingDll__") ;

	if (pFindHook== NULL) 
	{	MessageBox(NULL, "can't find search.dll", "ERROR!", MB_OK);
		return 0 ;
	}else if ((*pFindHook)("CRCCHECK") != 1) 
	{	return 0 ;
	}
	FreeLibrary(hDll);
	sprintf( szAppClass, "Client-I%d", hInstance);
    fLog = fopen("helbreath_boot.log", "a");
    if (fLog) { fprintf(fLog, "[WinMain] InitApplication\n"); fclose(fLog); }
    if (!InitApplication( hInstance))        return (FALSE);
    fLog = fopen("helbreath_boot.log", "a");
    if (fLog) { fprintf(fLog, "[WinMain] InitInstance\n"); fclose(fLog); }
    if (!InitInstance(hInstance, nCmdShow)) return (FALSE);

    // AHORA aplicar configuración del launcher (después de que la ventana existe)
    fLog = fopen("helbreath_boot.log", "a");
    if (fLog) { fprintf(fLog, "[WinMain] Configuración launcher aplicada\n"); fclose(fLog); }
    if (g_bBorderlessMode) {
        G_pGame->m_DDraw.SetFullMode(FALSE);
        G_pGame->m_DInput.SetWindowedMode(TRUE);
    } else {
        G_pGame->m_DDraw.SetFullMode(TRUE);
        G_pGame->m_DInput.SetWindowedMode(FALSE);
    }

    fLog = fopen("helbreath_boot.log", "a");
    if (fLog) { fprintf(fLog, "[WinMain] Initialize\n"); fclose(fLog); }
    Initialize((char *)lpCmdLine);

/* [PATCHED] Multiclient mutex check removed at startup. */
	
	EventLoop();

/* [PATCHED] Multiclient mutex release removed at shutdown. */

	delete[] pJammer;
	delete G_pGame;

#ifdef DEF_USING_WIN_IME
	FreeLibrary(hRichDll);
#endif

	return 0;
}

BOOL InitApplication( HINSTANCE hInstance)
{WNDCLASS  wc;
	wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS);
	wc.lpfnWndProc   = (WNDPROC)WndProc;             
	wc.cbClsExtra    = 0;                            
	wc.cbWndExtra    = sizeof (int);
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szAppClass;        
	return (RegisterClass(&wc));
}

BOOL InitInstance( HINSTANCE hInstance, int nCmdShow )
{
    int cx = GetSystemMetrics(SM_CXFULLSCREEN)/2;
    int cy = GetSystemMetrics(SM_CYFULLSCREEN)/2;
    if(cy> 340) cy -= 40;
    G_hWnd = CreateWindowEx(NULL, szAppClass, "Helbreath", WS_POPUP, cx- 400, cy- 300,
                            800, 600, NULL, NULL, hInstance, NULL);
    if (!G_hWnd) return FALSE;
    G_hInstance = hInstance;

    // Inicializar tamaño cacheado del cliente
    g_iCachedClientWidth = 800;
    g_iCachedClientHeight = 600;

    ShowWindow(G_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(G_hWnd);
    return TRUE;
}

// Simple timer helper (used by EventLoop / OnDestroy)
void CALLBACK _TimerFunc(UINT wID, UINT wUser, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    PostMessage(G_hWnd, WM_USER_TIMERSIGNAL, wID, 0);
}

MMRESULT _StartTimer(DWORD dwTime)
{
    TIMECAPS caps;
    timeGetDevCaps(&caps, sizeof(caps));
    timeBeginPeriod(caps.wPeriodMin);
    return timeSetEvent(dwTime, 0, _TimerFunc, 0, (UINT)TIME_PERIODIC);
}

void _StopTimer(MMRESULT timerid)
{
    TIMECAPS caps;
    if (timerid != 0) {
        timeKillEvent(timerid);
        timeGetDevCaps(&caps, sizeof(caps));
        timeEndPeriod(caps.wPeriodMin);
    }
}

void Initialize(char * pCmdLine)
{
    int iX, iY, iSum;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int iErrCode = WSAStartup(wVersionRequested, &wsaData);
    if (iErrCode) {
        MessageBox(G_hWnd, "Winsock-V1.1 not found! Cannot execute program.", "ERROR", MB_ICONEXCLAMATION | MB_OK);
        PostQuitMessage(0);
        return;
    }
    
    // CRITICAL: Initialize the game (DirectDraw, DirectInput, sprites, etc.)
    if (G_pGame->bInit(G_hWnd, G_hInstance, pCmdLine) == FALSE) {
        PostQuitMessage(0);
        return;
    }
    
    // Start the game timer
    G_mmTimer = _StartTimer(1000);
    
    // Initialize lookup tables for color blending
    for (iX = 0; iX < 64; iX++)
    for (iY = 0; iY < 510; iY++) {
        iSum = iX + (iY - 255);
        if (iSum <= 0)  iSum = 1;
        if (iSum >= 31) iSum = 31;
        G_iAddTable31[iX][iY] = iSum;
        iSum = iX + (iY - 255);
        if (iSum <= 0)  iSum = 1;
        if (iSum >= 63) iSum = 63;
        G_iAddTable63[iX][iY] = iSum;
        if ((iY - 255) < iX) G_iAddTransTable31[iY][iX] = iX;
        else if ((iY - 255) > 31) G_iAddTransTable31[iY][iX] = 31;
        else G_iAddTransTable31[iY][iX] = iY - 255;
        if ((iY - 255) < iX) G_iAddTransTable63[iY][iX] = iX;
        else if ((iY - 255) > 63) G_iAddTransTable63[iY][iX] = 63;
        else G_iAddTransTable63[iY][iX] = iY - 255;
    }
}

void EventLoop()
{
    MSG msg;
    while (1) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (!GetMessage(&msg, NULL, 0, 0)) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else if (G_pGame && (G_pGame->m_bIsProgramActive || !G_pGame->m_DDraw.m_bFullMode)) {
            G_pGame->UpdateScreen();
            LimitFrameRate();
        }
        else if (G_pGame && G_pGame->m_cGameMode == DEF_GAMEMODE_ONLOADING) {
            G_pGame->UpdateScreen_OnLoading(FALSE);
            LimitFrameRate();
        }
        else {
            Sleep(10);
            WaitMessage();
        }
    }
}

void OnDestroy()
{
    if (G_pGame) {
        G_pGame->m_bIsProgramActive = FALSE;
        _StopTimer(G_mmTimer);
        G_pGame->m_DInput.RegisterRawInput(G_hWnd, FALSE);
        G_pGame->Quit();
    }
    WSACleanup();
    PostQuitMessage(0);
}
////////////////////////////////////////////////////////////////////////
