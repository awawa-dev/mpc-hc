#include "stdafx.h"
#include "CMPCThemeDialog.h"
#include "CMPCTheme.h"
#include "mplayerc.h"
#undef SubclassWindow


CMPCThemeDialog::CMPCThemeDialog(bool isDummy /* = false */)
{
    this->isDummy = isDummy;
}

CMPCThemeDialog::CMPCThemeDialog(UINT nIDTemplate, CWnd* pParentWnd) : CDialog(nIDTemplate, pParentWnd)
{
}


CMPCThemeDialog::~CMPCThemeDialog()
{
}

IMPLEMENT_DYNAMIC(CMPCThemeDialog, CDialog)

BEGIN_MESSAGE_MAP(CMPCThemeDialog, CDialog)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CMPCThemeDialog::OnInitDialog()
{
    BOOL ret = __super::OnInitDialog();
    CMPCThemeUtil::enableWindows10DarkFrame(this);
    return ret;
}

HBRUSH CMPCThemeDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    if (AppIsThemeLoaded()) {
        return getCtlColor(pDC, pWnd, nCtlColor);
    } else {
        HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
        return hbr;
    }
}
BOOL CMPCThemeDialog::PreTranslateMessage(MSG* pMsg) {
    if (isDummy) {
        return FALSE;
    } else {
        return CDialog::PreTranslateMessage(pMsg);
    }
}

