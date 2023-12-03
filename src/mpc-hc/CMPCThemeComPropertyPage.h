#pragma once
#include "ComPropertyPage.h"
#include "CMPCThemeUtil.h"
class CMPCThemeComPropertyPage : public CComPropertyPage, public CMPCThemeUtil
{
public:
    CMPCThemeComPropertyPage(IPropertyPage* pPage);
    virtual ~CMPCThemeComPropertyPage();


    void fulfillThemeReqs(SpecialThemeCases specialCase = SpecialThemeCases::NoSpecialCase) { CMPCThemeUtil::fulfillThemeReqs((CWnd*)this, specialCase); };
    static void SetDialogType(CLSID clsid) { dialogClsid = clsid; };
    DECLARE_DYNAMIC(CMPCThemeComPropertyPage)

    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
protected:
    virtual BOOL OnInitDialog();
    static CLSID dialogClsid;
};

