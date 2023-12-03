#pragma once
#include "CMPCThemeButton.h"

class CMPCThemePropPageButton : public CMPCThemeButton {
public:
    CMPCThemePropPageButton();
    virtual ~CMPCThemePropPageButton();
    DECLARE_DYNAMIC(CMPCThemePropPageButton)
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
private:
    void RedrawComboBoxes();
};

