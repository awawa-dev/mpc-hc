/*
 * (C) 2010-2020 see Authors.txt
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

#pragma once

#include "../mpc-hc/CMPCThemeInternalPropertyPageWnd.h"
#include "AudioDevice.h"
#include "IMpcAudioRenderer.h"
#include "resource.h"
#include "../mpc-hc/DpiHelper.h"
#include "../mpc-hc/CMPCThemeButton.h"
#include "../mpc-hc/CMPCThemeRadioOrCheck.h"
#include "../mpc-hc/CMPCThemeStatic.h"
#include "../mpc-hc/CMPCThemeComboBox.h"
#include "../mpc-hc/CMPCThemeGroupBox.h"
#include "../mpc-hc/CMPCThemeEdit.h"

class __declspec(uuid("1E53BA32-3BCC-4dff-9342-34E46BE3F5A5"))
	CMpcAudioRendererSettingsWnd : public CMPCThemeInternalPropertyPageWnd
{
private :
	CComQIPtr<IMpcAudioRendererFilter> m_pMAR;

		DpiHelper m_dpi;

	AudioDevices::deviceList_t m_deviceList;

	CMPCThemeGroupBox m_output_group;
	CMPCThemeStatic   m_txtWasapiMode;
	CMPCThemeComboBox m_cbWasapiMode;
	CMPCThemeStatic   m_txtWasapiMethod;
	CMPCThemeComboBox m_cbWasapiMethod;
	CMPCThemeStatic   m_txtDevicePeriod;
	CMPCThemeComboBox m_cbDevicePeriod;

	CMPCThemeRadioOrCheck m_cbUseBitExactOutput;
	CMPCThemeRadioOrCheck m_cbUseSystemLayoutChannels;
	CMPCThemeRadioOrCheck m_cbAltCheckFormat;
	CMPCThemeRadioOrCheck m_cbReleaseDeviceIdle;
	CMPCThemeRadioOrCheck m_cbUseCrossFeed;
	CMPCThemeRadioOrCheck m_cbDummyChannels;
	CMPCThemeButton       m_btnReset;

	CMPCThemeStatic	  m_txtSoundDevice;
	CMPCThemeComboBox m_cbSoundDevice;

	enum {
		IDC_PP_SOUND_DEVICE = 10000,
		IDC_PP_WASAPI_MODE,
		IDC_PP_WASAPI_METHOD,
		IDC_PP_WASAPI_DEVICE_PERIOD,
		IDC_PP_USE_BITEXACT_OUTPUT,
		IDC_PP_USE_SYSTEM_LAYOUT_CHANNELS,
		IDC_PP_ALT_FORMAT_CHECK,
		IDC_PP_FREE_DEVICE_INACTIVE,
		IDC_PP_USE_CROSSFEED,
		IDC_PP_DUMMY_CHANNELS,
		IDC_PP_RESET,
	};

public:
	CMpcAudioRendererSettingsWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks) override;
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();
	bool OnApply();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCEW(IDS_FILTER_SETTINGS_CAPTION); }
	static CSize GetWindowSize() { return CSize(340, 211); }

	DECLARE_MESSAGE_MAP()

	afx_msg void OnClickedWasapiMode();
	afx_msg void OnClickedBitExact();
	afx_msg void OnBnClickedReset();
	afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);
};


class __declspec(uuid("E3D0704B-1579-4E9E-8674-2674CB90D07A"))
	CMpcAudioRendererStatusWnd : public CMPCThemeInternalPropertyPageWnd
{
private :
	CComQIPtr<IMpcAudioRendererFilter> m_pMAR;

	CMPCThemeStatic	m_txtDevice;
	CMPCThemeEdit		m_edtDevice;
	CMPCThemeStatic	m_txtMode;
	CMPCThemeEdit		m_edtMode;

	CMPCThemeGroupBox	m_grpInput;
	CMPCThemeStatic		m_InputFormatLabel;
	CMPCThemeStatic		m_InputFormatText;
	CMPCThemeStatic		m_InputChannelLabel;
	CMPCThemeStatic		m_InputChannelText;
	CMPCThemeStatic		m_InputRateLabel;
	CMPCThemeStatic		m_InputRateText;

	CMPCThemeGroupBox	m_grpOutput;
	CMPCThemeStatic		m_OutputFormatLabel;
	CMPCThemeStatic		m_OutputFormatText;
	CMPCThemeStatic		m_OutputChannelLabel;
	CMPCThemeStatic		m_OutputChannelText;
	CMPCThemeStatic		m_OutputRateLabel;
	CMPCThemeStatic		m_OutputRateText;

	void UpdateStatus();

protected:
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

public:
	CMpcAudioRendererStatusWnd();

	bool OnConnect(const CInterfaceList<IUnknown, &IID_IUnknown>& pUnks) override;
	void OnDisconnect();
	bool OnActivate();
	void OnDeactivate();

	static LPCWSTR GetWindowTitle() { return MAKEINTRESOURCEW(IDS_ARS_STATUS); }
	static CSize GetWindowSize() { return CSize(392, 143); }

	DECLARE_MESSAGE_MAP()
};
