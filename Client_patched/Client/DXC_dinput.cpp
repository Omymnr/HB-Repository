// DXC_dinput.cpp: implementation of the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_pDI    = NULL;
	m_pMouse = NULL;
	m_sX     = 0;
	m_sY     = 0;
	m_sZ     = 0;
	m_hWnd   = NULL;
	m_bUseRawInput = false;
	m_uiRawButtons = 0;
	m_bWindowedMode = FALSE;
}

DXC_dinput::~DXC_dinput()
{
	if (m_pMouse != NULL) {
		m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = NULL;
	}
	if (m_pDI != NULL) {
		m_pDI->Release();
        m_pDI = NULL;
	}
	m_hWnd = NULL;
}

BOOL DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
 HRESULT hr;
 DIMOUSESTATE dims;
 POINT Point;

	GetCursorPos(&Point);
	ScreenToClient(hWnd, &Point);
	m_sX     = (short)(Point.x);
	m_sY     = (short)(Point.y);
	m_hWnd   = hWnd;

	hr = DirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, NULL );
    if (hr != DI_OK) return FALSE;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, NULL );
	if (hr != DI_OK) return FALSE;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return FALSE;
	// DISCL_FOREGROUND: Solo recibir input cuando la ventana tiene el foco
	// Esto evita que clicks en otras ventanas afecten al juego
	hr = m_pMouse->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (hr != DI_OK) return FALSE;

	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
	}

	return TRUE;
}


void DXC_dinput::SetAcquire(BOOL bFlag)
{
 DIMOUSESTATE dims;

	if (m_pMouse == NULL) return;
	if (bFlag == TRUE) {
		m_pMouse->Acquire();
		m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );
	}
	else m_pMouse->Unacquire();
}

static const short INTERNAL_RES_X = 800;
static const short INTERNAL_RES_Y = 600;

void DXC_dinput::SetWindowedMode(BOOL bWindowed)
{
	m_bWindowedMode = bWindowed;
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	DIMOUSESTATE dims;
	
	// If raw input is enabled, use cached coordinates from HandleRawInput/UpdateFromWindowsMessage
	if (m_bUseRawInput) {
		if (m_sX < 0) m_sX = 0;
		if (m_sY < 0) m_sY = 0;
		if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
		if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
		*pX = m_sX;
		*pY = m_sY;
		*pZ = m_sZ;
		*pLB = (char)((m_uiRawButtons & RAW_BTN_LEFT) ? 1 : 0);
		*pRB = (char)((m_uiRawButtons & RAW_BTN_RIGHT) ? 1 : 0);
		return;
	}

	// En modo WINDOWED: usar coordenadas absolutas de Windows (via WM_MOUSEMOVE)
	// Esto evita el problema de doble actualización con DirectInput deltas
	if (m_bWindowedMode) {
		// m_sX/m_sY ya fueron actualizados por UpdateFromWindowsMessage()
		// Solo necesitamos obtener el estado de los botones de DirectInput
		if (m_sX < 0) m_sX = 0;
		if (m_sY < 0) m_sY = 0;
		if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
		if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
		*pX = m_sX;
		*pY = m_sY;
		*pZ = m_sZ;
		
		// Obtener estado de botones de DirectInput
		if (m_pMouse != NULL && m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &dims) == DI_OK) {
			*pLB = (char)dims.rgbButtons[0];
			*pRB = (char)dims.rgbButtons[1];
		} else {
			// Fallback: usar GetAsyncKeyState para botones del mouse
			*pLB = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ? 1 : 0;
			*pRB = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ? 1 : 0;
			if (m_pMouse) m_pMouse->Acquire();
		}
		return;
	}

	// En modo FULLSCREEN: usar DirectInput con deltas para movimiento suave
	if (m_pMouse == NULL) {
		// Fallback si no hay DirectInput
		POINT pt;
		if (m_hWnd && GetCursorPos(&pt)) {
			ScreenToClient(m_hWnd, &pt);
			RECT rc;
			if (GetClientRect(m_hWnd, &rc)) {
				int w = rc.right - rc.left;
				int h = rc.bottom - rc.top;
				if (w > 0 && h > 0) {
					m_sX = (short)((pt.x * (int)INTERNAL_RES_X) / w);
					m_sY = (short)((pt.y * (int)INTERNAL_RES_Y) / h);
				}
			}
		}
		if (m_sX < 0) m_sX = 0;
		if (m_sY < 0) m_sY = 0;
		if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
		if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
		*pX = m_sX;
		*pY = m_sY;
		*pZ = m_sZ;
		*pLB = 0;
		*pRB = 0;
		return;
	}

	if (m_pMouse->GetDeviceState(sizeof(DIMOUSESTATE), &dims) != DI_OK)
	{
		// DirectInput falló - la ventana no tiene el foco
		// Re-adquirir para cuando vuelva el foco
		m_pMouse->Acquire();
		
		// Sincronizar posición del cursor para evitar saltos al recuperar foco
		POINT pt;
		if (m_hWnd && GetCursorPos(&pt)) {
			ScreenToClient(m_hWnd, &pt);
			RECT rc;
			if (GetClientRect(m_hWnd, &rc)) {
				int w = rc.right - rc.left;
				int h = rc.bottom - rc.top;
				if (w > 0 && h > 0) {
					m_sX = (short)((pt.x * (int)INTERNAL_RES_X) / w);
					m_sY = (short)((pt.y * (int)INTERNAL_RES_Y) / h);
				}
			}
		}
		if (m_sX < 0) m_sX = 0;
		if (m_sY < 0) m_sY = 0;
		if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
		if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
		*pX = m_sX;
		*pY = m_sY;
		*pZ = m_sZ;
		// NO registrar clicks cuando no tenemos foco - evita clicks fantasma
		*pLB = 0;
		*pRB = 0;
		return;
	}

	// DirectInput exitoso en fullscreen - usar deltas para movimiento suave
	m_sX += (short)dims.lX;
	m_sY += (short)dims.lY;
	if ((short)dims.lZ != 0) m_sZ = (short)dims.lZ;
	
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
	if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
	
	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = (char)dims.rgbButtons[0];
	*pRB = (char)dims.rgbButtons[1];
}


void DXC_dinput::RegisterRawInput(HWND hWnd, BOOL bEnable)
{
	if (bEnable) {
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01; // Generic desktop controls
		Rid.usUsage = 0x02;     // Mouse
		Rid.dwFlags = RIDEV_INPUTSINK; // receive even when not in focus
		Rid.hwndTarget = hWnd;
		if (RegisterRawInputDevices(&Rid, 1, sizeof(Rid)) == FALSE) {
			// failed
			m_bUseRawInput = false;
			return;
		}
		m_bUseRawInput = true;
		m_hWnd = hWnd;
	}
	else {
		RAWINPUTDEVICE Rid;
		Rid.usUsagePage = 0x01;
		Rid.usUsage = 0x02;
		Rid.dwFlags = RIDEV_REMOVE;
		Rid.hwndTarget = NULL;
		RegisterRawInputDevices(&Rid, 1, sizeof(Rid));
		m_bUseRawInput = false;
	}
}


void DXC_dinput::HandleRawInput(LPARAM lParam)
{
	UINT dwSize = 0;
	GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
	if (dwSize == 0) return;

	LPBYTE lpb = new BYTE[dwSize];
	if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
		delete[] lpb;
		return;
	}

	RAWINPUT* raw = (RAWINPUT*)lpb;
	if (raw->header.dwType == RIM_TYPEMOUSE) {
		RAWMOUSE& rm = raw->data.mouse;
		
		// En modo windowed, el cursor se maneja via WM_MOUSEMOVE, no raw input
		// Solo procesar en modo fullscreen
		if (!m_bWindowedMode) {
			// Use relative movement
			if (rm.usFlags == MOUSE_MOVE_RELATIVE) {
				m_sX += (short)rm.lLastX;
				m_sY += (short)rm.lLastY;
				if (m_sX < 0) m_sX = 0;
				if (m_sY < 0) m_sY = 0;
				if (m_sX > INTERNAL_RES_X-1) m_sX = INTERNAL_RES_X-1;
				if (m_sY > INTERNAL_RES_Y-1) m_sY = INTERNAL_RES_Y-1;
			}

			// Buttons en fullscreen
			if (rm.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)    m_uiRawButtons |= RAW_BTN_LEFT;
			if (rm.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)      m_uiRawButtons &= ~RAW_BTN_LEFT;
			if (rm.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)   m_uiRawButtons |= RAW_BTN_RIGHT;
			if (rm.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)     m_uiRawButtons &= ~RAW_BTN_RIGHT;
			if (rm.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)  m_uiRawButtons |= RAW_BTN_MIDDLE;
			if (rm.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)    m_uiRawButtons &= ~RAW_BTN_MIDDLE;
		}
	}

	delete[] lpb;
}

// High-frequency cursor position update from WM_MOUSEMOVE
// This provides smoother cursor movement on high-refresh rate monitors (144Hz+)
// because Windows messages are processed at the OS level, independent of game framerate
void DXC_dinput::UpdateFromWindowsMessage(int x, int y, int clientWidth, int clientHeight)
{
	// Only update in windowed mode - fullscreen uses DirectInput exclusively
	if (!m_bWindowedMode) return;
	
	// Scale from window coordinates to internal resolution (800x600)
	if (clientWidth > 0 && clientHeight > 0) {
		m_sX = (short)((x * (int)INTERNAL_RES_X) / clientWidth);
		m_sY = (short)((y * (int)INTERNAL_RES_Y) / clientHeight);
	}
	else {
		m_sX = (short)x;
		m_sY = (short)y;
	}
	
	// Clamp to valid range
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > INTERNAL_RES_X - 1) m_sX = INTERNAL_RES_X - 1;
	if (m_sY > INTERNAL_RES_Y - 1) m_sY = INTERNAL_RES_Y - 1;
}