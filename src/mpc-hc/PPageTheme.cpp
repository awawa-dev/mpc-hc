/*
 * (C) 2003-2006 Gabest
 * (C) 2006-2014 see Authors.txt
 *
 * This file is part of MPC-HC.
 *
 * MPC-HC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-HC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "mplayerc.h"
#include "MainFrm.h"
#include "PPageTheme.h"
#include "CMPCTheme.h"
#include "CMPCThemeUtil.h"
#include "ColorProfileUtil.h"
#include "resource.h"
#include "Translations.h"

// CPPageTheme dialog

IMPLEMENT_DYNAMIC(CPPageTheme, CMPCThemePPageBase)
CPPageTheme::CPPageTheme()
    : CMPCThemePPageBase(CPPageTheme::IDD, CPPageTheme::IDD)
    , m_bUseModernTheme(FALSE)
    , m_iModernSeekbarHeight(DEF_MODERN_SEEKBAR_HEIGHT)
    , m_iThemeMode(0)
    , m_nPosLangEnglish(0)
    , m_iDefaultToolbarSize(DEF_TOOLBAR_HEIGHT)
    , m_fUseSeekbarHover(TRUE)
    , m_nOSDSize(0)
    , m_fShowChapters(TRUE)
    , m_iSeekPreviewSize(15)
    , m_fShowOSD(FALSE)
    , m_bShowVideoInfoInStatusbar(FALSE)
    , m_bShowLangInStatusbar(FALSE)
    , m_bShowFPSInStatusbar(FALSE)
    , m_bShowABMarksInStatusbar(FALSE)
    , m_fSnapToDesktopEdges(FALSE)
    , m_fLimitWindowProportions(TRUE)
    , m_bHideWindowedControls(FALSE)
    , m_bUseEnhancedTaskBar(TRUE)
    , m_bUseSMTC(FALSE)
{
    EventRouter::EventSelection fires;
    fires.insert(MpcEvent::CHANGING_UI_LANGUAGE);
    fires.insert(MpcEvent::DEFAULT_TOOLBAR_SIZE_CHANGED);
    GetEventd().Connect(m_eventc, fires);
}

CPPageTheme::~CPPageTheme()
{
}

void CPPageTheme::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK1, m_bUseModernTheme);

    DDX_Text(pDX, IDC_MODERNSEEKBARHEIGHT, m_iModernSeekbarHeight);
    DDX_Control(pDX, IDC_MODERNSEEKBARHEIGHT, m_ModernSeekbarHeightEdit);
    DDV_MinMaxInt(pDX, m_iModernSeekbarHeight, MIN_MODERN_SEEKBAR_HEIGHT, MAX_MODERN_SEEKBAR_HEIGHT);
    DDX_Control(pDX, IDC_MODERNSEEKBARHEIGHT_SPIN, m_ModernSeekbarHeightCtrl);

    DDX_Control(pDX, IDC_COMBO1, m_ThemeMode);
    DDX_CBIndex(pDX, IDC_COMBO1, m_iThemeMode);
    DDX_Control(pDX, IDC_COMBO2, m_langsComboBox);

    DDX_Control(pDX, IDC_COMBO5, m_FontType);
    DDX_Control(pDX, IDC_COMBO6, m_FontSize);

    DDX_Check(pDX, IDC_CHECK8, m_fUseSeekbarHover);
    DDX_Control(pDX, IDC_SEEK_PREVIEW, m_HoverType);
    DDX_Text(pDX, IDC_EDIT4, m_iSeekPreviewSize);
    DDX_Control(pDX, IDC_EDIT4, m_SeekPreviewSizeEdit);
    DDX_Control(pDX, IDC_SPIN2, m_SeekPreviewSizeCtrl);
    DDX_Check(pDX, IDC_CHECK2, m_fShowChapters);


    DDX_Text(pDX, IDC_EDIT1, m_iDefaultToolbarSize);
    DDV_MinMaxInt(pDX, m_iDefaultToolbarSize, MIN_TOOLBAR_HEIGHT, MAX_TOOLBAR_HEIGHT);
    DDX_Control(pDX, IDC_SPIN1, m_DefaultToolbarSizeCtrl);

    DDX_Control(pDX, IDC_COMBO3, m_HoverPosition);
    DDX_Check(pDX, IDC_SHOW_OSD, m_fShowOSD);
    DDX_Check(pDX, IDC_CHECK4, m_bShowVideoInfoInStatusbar);
    DDX_Check(pDX, IDC_CHECK3, m_bShowLangInStatusbar);
    DDX_Check(pDX, IDC_CHECK5, m_bShowFPSInStatusbar);
    DDX_Check(pDX, IDC_CHECK6, m_bShowABMarksInStatusbar);

    DDX_Check(pDX, IDC_CHECK7, m_fLimitWindowProportions);
    DDX_Check(pDX, IDC_CHECK9, m_fSnapToDesktopEdges);
    DDX_Check(pDX, IDC_CHECK10, m_bHideWindowedControls);
    DDX_Check(pDX, IDC_CHECK_ENHANCED_TASKBAR, m_bUseEnhancedTaskBar);
    DDX_Check(pDX, IDC_CHECK11, m_bUseSMTC);
}


BEGIN_MESSAGE_MAP(CPPageTheme, CMPCThemePPageBase)
    ON_BN_CLICKED(IDC_CHECK8, OnHoverClicked)
    ON_BN_CLICKED(IDC_CHECK1, OnThemeClicked)
    ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
    ON_CBN_SELCHANGE(IDC_COMBO5, OnChngOSDCombo)
    ON_CBN_SELCHANGE(IDC_COMBO6, OnChngOSDCombo)
END_MESSAGE_MAP()

int CALLBACK EnumFontProc(ENUMLOGFONT FAR* lf, NEWTEXTMETRIC FAR* tm, int FontType, LPARAM dwData) {
    CAtlArray<CString>* fntl = (CAtlArray<CString>*)dwData;
    if (FontType == TRUETYPE_FONTTYPE) {
        fntl->Add(lf->elfFullName);
    }
    return 1; /* Continue the enumeration */
}
// CPPageTheme message handlers

BOOL CPPageTheme::OnInitDialog()
{
    __super::OnInitDialog();

    const CAppSettings& s = AfxGetAppSettings();
    m_bUseModernTheme = s.bMPCTheme;

    ThemeEnableSubControls(m_bUseModernTheme);

    m_ModernSeekbarHeightCtrl.SetRange32(MIN_MODERN_SEEKBAR_HEIGHT, MAX_MODERN_SEEKBAR_HEIGHT);
    m_iModernSeekbarHeight = s.iModernSeekbarHeight;

    m_DefaultToolbarSizeCtrl.SetRange32(MIN_TOOLBAR_HEIGHT, MAX_TOOLBAR_HEIGHT);
    m_iDefaultToolbarSize = s.nDefaultToolbarSize;

    m_iThemeMode = static_cast<int>(s.eModernThemeMode);

    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_DARK));
    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_LIGHT));
    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_WINDOWS));
    CorrectComboListWidth(m_ThemeMode);

    for (auto& lr : Translations::GetAvailableLanguageResources()) {
        int pos = m_langsComboBox.AddString(lr.name);
        if (pos != CB_ERR) {
            m_langsComboBox.SetItemData(pos, lr.localeID);
            if (lr.localeID == s.language) {
                m_langsComboBox.SetCurSel(pos);
            }
            if (lr.localeID == 0) {
                m_nPosLangEnglish = pos;
            }
        } else {
            ASSERT(FALSE);
        }
    }
    CorrectComboListWidth(m_langsComboBox);

    m_fUseSeekbarHover = s.fUseSeekbarHover;

    m_HoverPosition.AddString(ResStr(IDS_TIME_TOOLTIP_ABOVE));
    m_HoverPosition.AddString(ResStr(IDS_TIME_TOOLTIP_BELOW));
    m_HoverPosition.SetCurSel(s.nHoverPosition);
    CorrectComboListWidth(m_HoverPosition);

    m_nOSDSize = s.nOSDSize;
    m_strOSDFont = s.strOSDFont;

    //m_fSeekPreview = s.fSeekPreview;
    m_HoverType.AddString(ResStr(IDS_SEEKBAR_HOVER_TOOLTIP));
    m_HoverType.AddString(ResStr(IDS_SEEKBAR_HOVER_PREVIEW));
    m_HoverType.SetCurSel(s.fSeekPreview ? 1 : 0);
    CorrectComboListWidth(m_HoverType);

    HoverEnableSubControls(m_fUseSeekbarHover);

    m_iSeekPreviewSize = s.iSeekPreviewSize;
    m_SeekPreviewSizeCtrl.SetRange32(5, 40);

    m_fShowChapters = s.fShowChapters;

    m_FontType.Clear();
    m_FontSize.Clear();
    HDC dc = CreateDC(_T("DISPLAY"), nullptr, nullptr, nullptr);
    CAtlArray<CString> fntl;
    EnumFontFamilies(dc, nullptr, (FONTENUMPROC)EnumFontProc, (LPARAM)&fntl);
    DeleteDC(dc);
    for (size_t i = 0; i < fntl.GetCount(); ++i) {
        if (i > 0 && fntl[i - 1] == fntl[i]) {
            continue;
        }
        m_FontType.AddString(fntl[i]);
    }
    CorrectComboListWidth(m_FontType);
    int iSel = m_FontType.FindStringExact(0, m_strOSDFont);
    if (iSel == CB_ERR) {
        iSel = m_FontType.FindString(0, m_strOSDFont);
        if (iSel == CB_ERR) {
            iSel = 0;
        }
    }
    m_FontType.SetCurSel(iSel);

    CString str;
    for (int i = 10; i < 26; ++i) {
        str.Format(_T("%d"), i);
        m_FontSize.AddString(str);
        if (m_nOSDSize == i) {
            iSel = i;
        }
    }
    m_FontSize.SetCurSel(iSel - 10);

    m_fShowOSD = s.fShowOSD;
    m_bShowVideoInfoInStatusbar = s.bShowVideoInfoInStatusbar;
    m_bShowLangInStatusbar = s.bShowLangInStatusbar;
    m_bShowFPSInStatusbar = s.bShowFPSInStatusbar;
    m_bShowABMarksInStatusbar = s.bShowABMarksInStatusbar;
    m_fSnapToDesktopEdges = s.fSnapToDesktopEdges;
    m_fLimitWindowProportions = s.fLimitWindowProportions;
    m_bHideWindowedControls = s.bHideWindowedControls;
    m_bUseEnhancedTaskBar = s.bUseEnhancedTaskBar;
    m_bUseSMTC = s.bUseSMTC;

    CreateToolTip();
    EnableThemedDialogTooltips(this);

    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageTheme::OnApply()
{
    UpdateData();

    CAppSettings& s = AfxGetAppSettings();
    int nOldDefaultToolbarSize = s.nDefaultToolbarSize;

    s.bMPCTheme = !!m_bUseModernTheme;
    s.iModernSeekbarHeight = m_iModernSeekbarHeight;
    s.nDefaultToolbarSize = m_iDefaultToolbarSize;
    s.eModernThemeMode = static_cast<CMPCTheme::ModernThemeMode>(m_iThemeMode);

    int iLangSel = m_langsComboBox.GetCurSel();
    if (iLangSel != CB_ERR) {
        LANGID language = (LANGID)m_langsComboBox.GetItemData(iLangSel);
        if (s.language != language) {
            // Show a warning when switching to Arabic or Hebrew (must not be translated)
            if (PRIMARYLANGID(language) == LANG_ARABIC || PRIMARYLANGID(language) == LANG_HEBREW) {
                AfxMessageBox(_T("The Arabic and Hebrew translations will be correctly displayed (with a right-to-left layout) after restarting the application.\n"),
                    MB_ICONINFORMATION | MB_OK);
            }

            if (!Translations::SetLanguage(language)) {
                // In case of error, reset the language to English
                language = 0;
                m_langsComboBox.SetCurSel(m_nPosLangEnglish);
            }
            s.language = language;

            // Inform all interested listeners that the UI language changed
            m_eventc.FireEvent(MpcEvent::CHANGING_UI_LANGUAGE);
        }
    } else {
        ASSERT(FALSE);
    }

    if (nOldDefaultToolbarSize != s.nDefaultToolbarSize) {
        m_eventc.FireEvent(MpcEvent::DEFAULT_TOOLBAR_SIZE_CHANGED);
        if (CMainFrame* pMainFrame = AfxGetMainFrame()) {
            pMainFrame->RecalcLayout();
        }
    }
    s.nHoverPosition = m_HoverPosition.GetCurSel();

    s.fUseSeekbarHover = !!m_fUseSeekbarHover;
    s.nOSDSize = m_nOSDSize;
    m_FontType.GetLBText(m_FontType.GetCurSel(), s.strOSDFont);
    s.fShowChapters = !!m_fShowChapters;

    s.fSeekPreview = m_HoverType.GetCurSel() == 1;
    if (m_iSeekPreviewSize < 5) m_iSeekPreviewSize = 5;
    if (m_iSeekPreviewSize > 40) m_iSeekPreviewSize = 40;
    CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
    if (s.iSeekPreviewSize != m_iSeekPreviewSize) {
        s.iSeekPreviewSize = m_iSeekPreviewSize;
        pFrame->m_wndPreView.SetRelativeSize(m_iSeekPreviewSize);
    }

    // There is no main frame when the option dialog is displayed stand-alone
    if (CMainFrame* pMainFrame = AfxGetMainFrame()) {
        pMainFrame->UpdateControlState(CMainFrame::UPDATE_SKYPE);
        pMainFrame->UpdateControlState(CMainFrame::UPDATE_SEEKBAR_CHAPTERS);
    }

    s.fShowOSD = !!m_fShowOSD;
    s.bShowVideoInfoInStatusbar = !!m_bShowVideoInfoInStatusbar;
    s.bShowLangInStatusbar = !!m_bShowLangInStatusbar;
    s.bShowFPSInStatusbar = !!m_bShowFPSInStatusbar;
    s.bShowABMarksInStatusbar = !!m_bShowABMarksInStatusbar;
    s.fSnapToDesktopEdges = !!m_fSnapToDesktopEdges;
    s.fLimitWindowProportions = !!m_fLimitWindowProportions;
    s.bHideWindowedControls = !!m_bHideWindowedControls;

    s.bUseEnhancedTaskBar = !!m_bUseEnhancedTaskBar;
    if (m_bUseEnhancedTaskBar) {
        pFrame->CreateThumbnailToolbar();
    }
    pFrame->UpdateThumbarButton();

    s.bUseSMTC = !!m_bUseSMTC;

    return __super::OnApply();
}

void CPPageTheme::HoverEnableSubControls(bool hoverEnabled) {
    m_HoverPosition.EnableWindow(hoverEnabled);
    m_HoverType.EnableWindow(hoverEnabled);
    m_SeekPreviewSizeEdit.EnableWindow(hoverEnabled);
    m_SeekPreviewSizeCtrl.EnableWindow(hoverEnabled);
}

void CPPageTheme::OnHoverClicked() {
    HoverEnableSubControls(IsDlgButtonChecked(IDC_CHECK8));
    SetModified();
}

void CPPageTheme::ThemeEnableSubControls(bool themeEnabled) {
    m_ThemeMode.EnableWindow(themeEnabled);
    m_ModernSeekbarHeightCtrl.EnableWindow(themeEnabled);
    m_ModernSeekbarHeightEdit.EnableWindow(themeEnabled);
}

void CPPageTheme::OnThemeClicked() {
    ThemeEnableSubControls(IsDlgButtonChecked(IDC_CHECK1));
    SetModified();
}

BOOL CPPageTheme::OnToolTipNotify(UINT id, NMHDR* pNMH, LRESULT* pResult) {
    LPTOOLTIPTEXT pTTT = reinterpret_cast<LPTOOLTIPTEXT>(pNMH);

    UINT_PTR nID = pNMH->idFrom;
    if (pTTT->uFlags & TTF_IDISHWND) {
        nID = ::GetDlgCtrlID((HWND)nID);
    }

    BOOL bRet = FALSE;

    switch (nID) {
    case IDC_COMBO5:
        bRet = FillComboToolTip(m_FontType, pTTT);
        break;
    case IDC_COMBO3:
        bRet = FillComboToolTip(m_HoverPosition, pTTT);
        break;
    }

    if (bRet) {
        PlaceThemedDialogTooltip(nID);
    }

    return bRet;
}

void CPPageTheme::OnChngOSDCombo() {
    CString str;
    m_nOSDSize = m_FontSize.GetCurSel() + 10;
    m_FontType.GetLBText(m_FontType.GetCurSel(), str);
    if (CMainFrame* pMainFrame = AfxGetMainFrame()) {
        pMainFrame->m_OSD.DisplayMessage(OSD_TOPLEFT, _T("Test"), 2000, m_nOSDSize, str);
    }
    SetModified();
}
