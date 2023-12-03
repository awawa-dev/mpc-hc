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

// CPPageTheme dialog

IMPLEMENT_DYNAMIC(CPPageTheme, CMPCThemePPageBase)
CPPageTheme::CPPageTheme()
    : CMPCThemePPageBase(CPPageTheme::IDD, CPPageTheme::IDD)
    , m_bUseModernTheme(FALSE)
    , m_iModernSeekbarHeight(DEF_MODERN_SEEKBAR_HEIGHT)
    , m_iThemeMode(0)
{
}

CPPageTheme::~CPPageTheme()
{
}

void CPPageTheme::DoDataExchange(CDataExchange* pDX)
{
    __super::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_CHECK1, m_bUseModernTheme);
    DDX_Text(pDX, IDC_MODERNSEEKBARHEIGHT, m_iModernSeekbarHeight);
    DDV_MinMaxInt(pDX, m_iModernSeekbarHeight, MIN_MODERN_SEEKBAR_HEIGHT, MAX_MODERN_SEEKBAR_HEIGHT);
    DDX_Control(pDX, IDC_MODERNSEEKBARHEIGHT_SPIN, m_ModernSeekbarHeightCtrl);
    DDX_Control(pDX, IDC_COMBO1, m_ThemeMode);
    DDX_CBIndex(pDX, IDC_COMBO1, m_iThemeMode);
}


BEGIN_MESSAGE_MAP(CPPageTheme, CMPCThemePPageBase)
END_MESSAGE_MAP()


// CPPageTheme message handlers

BOOL CPPageTheme::OnInitDialog()
{
    __super::OnInitDialog();

    const CAppSettings& s = AfxGetAppSettings();
    m_bUseModernTheme = s.bMPCTheme;
    m_ModernSeekbarHeightCtrl.SetRange32(MIN_MODERN_SEEKBAR_HEIGHT, MAX_MODERN_SEEKBAR_HEIGHT);
    m_iModernSeekbarHeight = s.iModernSeekbarHeight;
    m_iThemeMode = static_cast<int>(s.eModernThemeMode);

    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_DARK));
    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_LIGHT));
    m_ThemeMode.AddString(ResStr(IDS_THEMEMODE_WINDOWS));
    CorrectComboListWidth(m_ThemeMode);

    UpdateData(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPageTheme::OnApply()
{
    UpdateData();

    CAppSettings& s = AfxGetAppSettings();
    s.bMPCTheme = !!m_bUseModernTheme;
    s.iModernSeekbarHeight = m_iModernSeekbarHeight;
    s.eModernThemeMode = static_cast<CMPCTheme::ModernThemeMode>(m_iThemeMode);

    return __super::OnApply();
}
