
// ConnectShouterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConnectShouter.h"
#include "ConnectShouterDlg.h"
#include "afxdialogex.h"
#include "RegistryConfig.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CConnectShouterDlg dialog

CConnectShouterDlg::CConnectShouterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConnectShouterDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CConnectShouterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_VIDEO, m_ComboBoxVideo);
	DDX_Control(pDX, IDC_COMBO_AUDIO, m_ComboBoxAudio);
	DDX_Control(pDX, IDC_BUTTON_PLAY, m_ButtonPlay);
	DDX_Control(pDX, IDC_BUTTON_PAUSE, m_ButtonPause);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_ButtonStop);
	DDX_Control(pDX, IDC_BUTTON_PLAY2, m_ButtonPlay2);
	DDX_Control(pDX, IDC_BUTTON_PAUSE2, m_ButtonPause2);
	DDX_Control(pDX, IDC_BUTTON_STOP2, m_ButtonStop2);
	DDX_Control(pDX, IDC_EDIT_HOST, m_EditHost);
	DDX_Control(pDX, IDC_EDIT_PORT, m_EditPort);
	DDX_Control(pDX, IDC_EDIT_MOUNT, m_EditMount);
	DDX_Control(pDX, IDC_EDIT_URL, m_EditURL);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_EditPassword);
}

BEGIN_MESSAGE_MAP(CConnectShouterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO_VIDEO, &CConnectShouterDlg::OnCbnSelchangeComboVideo)
	ON_CBN_SELCHANGE(IDC_COMBO_AUDIO, &CConnectShouterDlg::OnCbnSelchangeComboAudio)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CConnectShouterDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CConnectShouterDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CConnectShouterDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDCANCEL, &CConnectShouterDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_SAVEBUTTON, &CConnectShouterDlg::OnBnClickedSavebutton)
	ON_BN_CLICKED(IDC_CLEARBUTTON, &CConnectShouterDlg::OnBnClickedClearbutton)
	ON_BN_CLICKED(IDC_BUTTON_PLAY2, &CConnectShouterDlg::OnBnClickedButtonPlay2)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE2, &CConnectShouterDlg::OnBnClickedButtonPause2)
	ON_BN_CLICKED(IDC_BUTTON_STOP2, &CConnectShouterDlg::OnBnClickedButtonStop2)
END_MESSAGE_MAP()


// CConnectShouterDlg message handlers

BOOL CConnectShouterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	dfh.EnumerateVideoDevicesToMapSP();
	dfh.FillVideoComboBox(m_ComboBoxVideo);

	dfh.EnumerateAudioDevicesToMapSP();
	dfh.FillAudioComboBox(m_ComboBoxAudio);

	LoadRegistryValues();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CConnectShouterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CConnectShouterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CConnectShouterDlg::OnCbnSelchangeComboVideo()
{
	int index = m_ComboBoxVideo.GetCurSel();
	if (index > -1)
	{
		IMoniker* ptr = reinterpret_cast<IMoniker*>(m_ComboBoxVideo.GetItemDataPtr(index));
		if (dfh.ConnectShouterVideoGraph(ptr) == S_OK) {
			m_ButtonPlay.EnableWindow(TRUE);
			m_ComboBoxAudio.EnableWindow(FALSE);
		}
	}
}


void CConnectShouterDlg::OnCbnSelchangeComboAudio()
{
	int index = m_ComboBoxAudio.GetCurSel();
	if (index > -1)
	{
		IMoniker* ptr = reinterpret_cast<IMoniker*>(m_ComboBoxAudio.GetItemDataPtr(index));
		if (dfh.ConnectShouterAudioGraph(ptr) == S_OK) {
			m_ButtonPlay2.EnableWindow(TRUE);
			m_ComboBoxVideo.EnableWindow(FALSE);
		}
	}
}

void CConnectShouterDlg::OnBnClickedButtonPlay()
{
	if (dfh.MediaControlRun() == S_OK) {
		m_ButtonPlay.EnableWindow(FALSE);
		m_ButtonPause.EnableWindow(TRUE);
		m_ButtonStop.EnableWindow(TRUE);
	} else {
		DisableAllVideoButtons();
	}
}


void CConnectShouterDlg::OnBnClickedButtonPause()
{
	if (dfh.MediaControlPause() == S_OK) {
		m_ButtonPlay.EnableWindow(TRUE);
		m_ButtonPause.EnableWindow(FALSE);
		m_ButtonStop.EnableWindow(TRUE);
	} else {
		DisableAllVideoButtons();
	}
}


void CConnectShouterDlg::OnBnClickedButtonStop()
{
	dfh.MediaControlStop();
	DisableAllVideoButtons();
	m_ComboBoxAudio.EnableWindow(TRUE);
	m_ComboBoxVideo.SetCurSel(-1);
}

void CConnectShouterDlg::DisableAllVideoButtons()
{
	m_ButtonPlay.EnableWindow(FALSE);
	m_ButtonPause.EnableWindow(FALSE);
	m_ButtonStop.EnableWindow(FALSE);
}

void CConnectShouterDlg::DisableAllAudioButtons()
{
	m_ButtonPlay2.EnableWindow(FALSE);
	m_ButtonPause2.EnableWindow(FALSE);
	m_ButtonStop2.EnableWindow(FALSE);
}

void CConnectShouterDlg::LoadRegistryValues()
{
	CRegistryConfig reg(CRegistryConfig::OGGSHOUTSTREAM_PATH);

	CString host;
	reg.GetValueFromKey(CString("host"), &host);
	m_EditHost.SetWindowText(host);

	DWORD port = 0;
	reg.GetValueFromKey(CString("port"), &port);
	if (port >= 0) {
		CString port_str;
		port_str.Format(_T("%d"), (int)port);
		m_EditPort.SetWindowText(port_str);
	}

	CString mount;
	reg.GetValueFromKey(CString("mount"), &mount);
	m_EditMount.SetWindowTextW(mount);

	CString URL;
	reg.GetValueFromKey(CString("URL"), &URL);
	m_EditURL.SetWindowTextW(URL);

	CString password;
	reg.GetValueFromKey(CString("password"), &password);
	m_EditPassword.SetWindowText(password);

}

void CConnectShouterDlg::SaveRegistryValues()
{
	CRegistryConfig reg(CRegistryConfig::OGGSHOUTSTREAM_PATH);
	
	const CString HTTP = _T("http://");
	CString host;
	m_EditHost.GetWindowText(host);
	if (host.Left(7) != HTTP) {
		host = HTTP + host;
	}
	reg.SaveElementToRegistry(CString("host"), host);

	CString port_str;
	m_EditPort.GetWindowText(port_str);
	int port = _wtoi((LPCWSTR)port_str);
	reg.SaveElementToRegistry(CString("port"), port);	

	CString mount;
	m_EditMount.GetWindowText(mount);
	reg.SaveElementToRegistry(CString("mount"), mount);

	const CString DELIMITER = _T("/");
	CString delimiter;
	delimiter.Empty();
	if (mount.Left(1) != DELIMITER) {
		delimiter = DELIMITER;
	}

	CString URL;
	URL.Format(_T("%s:%s%s%s"), (LPCTSTR)host, (LPCTSTR)port_str, (LPCTSTR)delimiter, (LPCTSTR)mount);
	reg.SaveElementToRegistry(CString("URL"), URL);
	m_EditURL.SetWindowText(URL);

	CString password;
	m_EditPassword.GetWindowText(password);
	reg.SaveElementToRegistry(CString("password"), password);

}

void CConnectShouterDlg::OnBnClickedCancel()
{
	SaveRegistryValues();

	CDialogEx::OnCancel();
}


void CConnectShouterDlg::OnBnClickedSavebutton()
{
	SaveRegistryValues();
}


void CConnectShouterDlg::OnBnClickedClearbutton()
{
	CString temp;
	temp.Empty();

	m_EditHost.SetWindowText(temp);

	m_EditPort.SetWindowText(temp);

	m_EditMount.SetWindowTextW(temp);

	m_EditURL.SetWindowTextW(temp);

	m_EditPassword.SetWindowText(temp);

	SaveRegistryValues();
}


void CConnectShouterDlg::OnBnClickedButtonPlay2()
{
	if (dfh.MediaControlRun() == S_OK) {
		m_ButtonPlay2.EnableWindow(FALSE);
		m_ButtonPause2.EnableWindow(TRUE);
		m_ButtonStop2.EnableWindow(TRUE);
	} else {
		DisableAllAudioButtons();
	}
}


void CConnectShouterDlg::OnBnClickedButtonPause2()
{
	if (dfh.MediaControlPause() == S_OK) {
		m_ButtonPlay2.EnableWindow(TRUE);
		m_ButtonPause2.EnableWindow(FALSE);
		m_ButtonStop2.EnableWindow(TRUE);
	} else {
		DisableAllAudioButtons();
	}
}


void CConnectShouterDlg::OnBnClickedButtonStop2()
{
	dfh.MediaControlStop();
	DisableAllAudioButtons();
	m_ComboBoxVideo.EnableWindow(TRUE);
	m_ComboBoxAudio.SetCurSel(-1);
}

