#include "stdafx.h"
#include "CMPCThemeGroupBox.h"
#include "CMPCTheme.h"
#include "CMPCThemeUtil.h"
#include "mplayerc.h"

IMPLEMENT_DYNAMIC(CMPCThemeGroupBox, CStatic)

CMPCThemeGroupBox::CMPCThemeGroupBox()
    :manuallySetFont(nullptr)
{

}


CMPCThemeGroupBox::~CMPCThemeGroupBox()
{
}

BEGIN_MESSAGE_MAP(CMPCThemeGroupBox, CStatic)
    ON_WM_PAINT()
    ON_WM_ENABLE()
    ON_MESSAGE(WM_SETFONT, OnSetFont)
    ON_MESSAGE(WM_GETFONT, OnGetFont)
END_MESSAGE_MAP()


void CMPCThemeGroupBox::OnPaint()
{
    if (AppNeedsThemedControls()) {

        CPaintDC dc(this);

        CRect r, rborder, rtext;
        GetClientRect(r);
        HDC hDC = ::GetDC(NULL);
        CString text;
        GetWindowText(text);

        CFont* font = GetFont();
        
        CSize cs = CMPCThemeUtil::GetTextSize(_T("W"), hDC, font);
        ::ReleaseDC(NULL, hDC);

        CBrush fb;
        fb.CreateSolidBrush(CMPCTheme::GroupBoxBorderColor);
        rborder = r;
        rborder.top += cs.cy / 2;
        dc.FrameRect(rborder, &fb);

        if (!text.IsEmpty()) {
            COLORREF oldClr;
            //see https://stackoverflow.com/questions/26481189/how-to-make-the-group-box-text-to-be-disabled-when-group-box-is-disabled
            //even if common controls doesn't always honor disabled group boxes, we can in the themed version
            if (IsWindowEnabled()) { 
                oldClr = dc.SetTextColor(CMPCTheme::TextFGColor);
            } else {
                oldClr = dc.SetTextColor(CMPCTheme::ContentTextDisabledFGColorFade);
            }
            COLORREF oldBkClr = dc.SetBkColor(CMPCTheme::WindowBGColor);
            CFont* pOldFont = dc.SelectObject(font);

            rtext = r;
            rtext.left += CMPCTheme::GroupBoxTextIndent;

            text += _T(" "); //seems to be the default behavior
            dc.DrawText(text, rtext, DT_TOP | DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL);

            dc.SelectObject(pOldFont);
            dc.SetTextColor(oldClr);
            dc.SetBkColor(oldBkClr);
        }
        fb.DeleteObject();
        ::ReleaseDC(NULL, hDC);
    } else {
        __super::OnPaint();
    }
}

void CMPCThemeGroupBox::OnEnable(BOOL bEnable) {
    if (AppNeedsThemedControls()) {
        SetRedraw(FALSE);
        __super::OnEnable(bEnable);
        SetRedraw(TRUE);
        Invalidate(); //WM_PAINT not handled when enabling/disabling
        RedrawWindow();
    } else {
        __super::OnEnable(bEnable);
    }
}

LRESULT CMPCThemeGroupBox::OnSetFont(WPARAM wParam, LPARAM lParam) {
    manuallySetFont = (HFONT)wParam;
    if ((BOOL)lParam) {
        Invalidate();
    }
    return 0;
}

LRESULT CMPCThemeGroupBox::OnGetFont(WPARAM wParam, LPARAM lParam) {
    if (manuallySetFont) {
        return (LRESULT)manuallySetFont;
    } else {
        return (LRESULT)Default();
    }
}
