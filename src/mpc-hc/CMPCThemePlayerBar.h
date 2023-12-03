#pragma once
#include "PlayerBar.h"

class CMainFrame;
class CMPCThemePlayerBar :  public CPlayerBar
{
public:
    CMPCThemePlayerBar(CMainFrame* pMainFrame);
    virtual ~CMPCThemePlayerBar();
    DECLARE_DYNAMIC(CMPCThemePlayerBar)

    void NcPaintGripper(CDC* pDC, CRect rcClient);
    void mpc_fillNcBG(CDC* mdc, CRect rcDraw);
    DECLARE_MESSAGE_MAP()
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    void InitializeSize();
    void paintHideButton(CDC* pDC, CSCBButton b);
    virtual void NcCalcClient(LPRECT pRc, UINT nDockBarID);
protected:
    CMainFrame* m_pMainFrame;
};

