// DXC_dinput.h: interface for the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
#define AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define INITGUID
#include <windows.h>
#include "dinput.h"

// Internal raw button flags
enum {
    RAW_BTN_LEFT = 1,
    RAW_BTN_RIGHT = 2,
    RAW_BTN_MIDDLE = 4
};

class DXC_dinput  
{
public:
	DXC_dinput();
	virtual ~DXC_dinput();
	void UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB);
	void SetAcquire(BOOL bFlag);
	BOOL bInit(HWND hWnd, HINSTANCE hInst);

	// Raw Input support
	// Call RegisterRawInput after window is created to enable raw input handling.
	void RegisterRawInput(HWND hWnd, BOOL bEnable = TRUE);
	// Call HandleRawInput from your window procedure when you receive WM_INPUT
	void HandleRawInput(LPARAM lParam);

	DIMOUSESTATE dims;
	IDirectInput *           m_pDI;
	IDirectInputDevice *     m_pMouse;
	short m_sX, m_sY, m_sZ;
	// store the window handle so we can fallback to GetCursorPos when DirectInput is unavailable
	HWND m_hWnd;

	// Raw input state
	bool m_bUseRawInput;
	unsigned int m_uiRawButtons; // current raw button state flags

	BOOL m_bWindowedMode;
	void SetWindowedMode(BOOL bWindowed);
	
	// Flag para indicar si el programa está activo (tiene el foco)
	BOOL m_bProgramActive;
	void SetProgramActive(BOOL bActive);
	
	// High-frequency cursor update from WM_MOUSEMOVE for smoother cursor on high-refresh monitors
	void UpdateFromWindowsMessage(int x, int y, int clientWidth, int clientHeight);
};

#endif // !defined(AFX_DXC_DINPUT_H__639F0280_78D8_11D2_A8E6_00001C7030A6__INCLUDED_)
