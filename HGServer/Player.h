// Player.h: interface for the CPlayer class.
//
//////////////////////////////////////////////////////////////////////

#define AFX_PLAYER_H__C3D29FC6_755B_11D2_A8E6_00001C7030A6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include "XSocket.h"

class CPlayer
{
public:
    CPlayer();
    virtual ~CPlayer();

    class XSocket* m_pXSock;

    // === Talents ===
    int m_iTalentLevel5;
    int m_iTalentLevel40;

    void SetTalent(int slot, int talentId) {
        if (slot == 5) m_iTalentLevel5 = talentId;
        else if (slot == 40) m_iTalentLevel40 = talentId;
    }

    int GetTalent(int slot) {
        if (slot == 5) return m_iTalentLevel5;
        else if (slot == 40) return m_iTalentLevel40;
        return 0;
    }

private:
    // ... resto de atributos ...
};

#endif // !defined(AFX_PLAYER_H__C3D29FC6_755B_11D2_A8E6_00001C7030A6__INCLUDED_)
