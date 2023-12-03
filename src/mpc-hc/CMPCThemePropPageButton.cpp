#include "stdafx.h"
#include "CMPCThemePropPageButton.h"
#include "CMPCTheme.h"
#include "CMPCThemeUtil.h"
#include "mplayerc.h"


CMPCThemePropPageButton::CMPCThemePropPageButton() {
}

CMPCThemePropPageButton::~CMPCThemePropPageButton() {
}

IMPLEMENT_DYNAMIC(CMPCThemePropPageButton, CMPCThemeButton)
BEGIN_MESSAGE_MAP(CMPCThemePropPageButton, CMPCThemeButton)
    ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CMPCThemePropPageButton::RedrawComboBoxes() { //redraw combo boxes, issue with external prop pages (MPCVR)
    CWnd* pChild = GetParent()->GetWindow(GW_CHILD);
    while (pChild) {
        LRESULT lRes = pChild->SendMessage(WM_GETDLGCODE, 0, 0);
        CWnd* tChild = pChild;
        pChild = pChild->GetNextWindow();
        TCHAR windowClass[MAX_PATH];
        ::GetClassName(tChild->GetSafeHwnd(), windowClass, _countof(windowClass));

        if (0 == _tcsicmp(windowClass, WC_COMBOBOX)) {
            tChild->RedrawWindow(0, 0, RDW_INVALIDATE);
        }
    }
}

void CMPCThemePropPageButton::OnLButtonUp(UINT nFlags, CPoint point) {
    CMPCThemeButton::OnLButtonUp(nFlags, point);
    if (AppNeedsThemedControls()) {
        RedrawComboBoxes();
    }
}
