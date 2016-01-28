// ConnectShouterDlg.h : header file
//
#include "DFH.h"


#pragma once


// CConnectShouterDlg dialog
class CConnectShouterDlg : public CDialogEx
{
	DFH dfh;

// Construction
public:
	CConnectShouterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CONNECTSHOUTER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	void DisableAllVideoButtons();
	void DisableAllAudioButtons();
	void LoadRegistryValues();
	void SaveRegistryValues();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchangeComboVideo();
	CComboBox m_ComboBoxVideo;
	CComboBox m_ComboBoxAudio;
	CButton m_ButtonPlay;
	CButton m_ButtonPause;
	CButton m_ButtonStop;
	CButton m_ButtonPlay2;
	CButton m_ButtonPause2;
	CButton m_ButtonStop2;
	CEdit m_EditHost;
	CEdit m_EditPort;
	CEdit m_EditMount;
	CEdit m_EditURL;
	CEdit m_EditPassword;


	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonPause();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedSavebutton();
	afx_msg void OnBnClickedClearbutton();
	afx_msg void OnBnClickedButtonPlay2();
	afx_msg void OnBnClickedButtonPause2();
	afx_msg void OnBnClickedButtonStop2();
	afx_msg void OnCbnSelchangeComboAudio();
};
