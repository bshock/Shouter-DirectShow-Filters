#include "stdafx.h"
#include "RegistryConfig.h"

TCHAR* CRegistryConfig::MP3SHOUTSTREAM_PATH = _T("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM");
TCHAR* CRegistryConfig::OGGSHOUTSTREAM_PATH = _T("SOFTWARE\\DSFILTERS\\OGGSHOUTSTREAM");

CRegistryConfig::CRegistryConfig()
{
	HKEY hK = HKEY_CURRENT_USER;	
	m_hKey = NULL;
	m_strSubKey = PTLIII_REGISTRY_KEY;
	
	if (RegOpenKeyEx(hK,(LPCTSTR)m_strSubKey, 0, KEY_ALL_ACCESS, &m_hKey) != ERROR_SUCCESS) {
	}
}

CRegistryConfig::CRegistryConfig( const CString& key )
{
	HKEY hK = HKEY_CURRENT_USER;	
	m_hKey = NULL;
	m_strSubKey = key;
	if (RegOpenKeyEx(hK, (LPCTSTR)m_strSubKey, 0, KEY_ALL_ACCESS, &m_hKey) != ERROR_SUCCESS) {
	}
}

CRegistryConfig::~CRegistryConfig()
{
	if (m_hKey!=NULL) {
		RegCloseKey(m_hKey);
	}
}

BOOL CRegistryConfig::SaveElementToRegistry(const CString& Key, const CString& Value)
{

	HKEY hK=HKEY_CURRENT_USER;	
	LPDWORD lpdwDisposition=NULL;

	if (m_hKey==NULL) {
		if (RegCreateKeyEx(hK, (LPCWSTR)m_strSubKey, 0, _T("LPCWSTR"), 0, 0, NULL, &m_hKey, lpdwDisposition) != ERROR_SUCCESS) {
			return FALSE;
		}
	}

	int nBytes = Value.GetLength()*sizeof(wchar_t);
	if (RegSetValueEx(m_hKey, (LPCTSTR)Key, 0,REG_SZ, (unsigned char *)(LPCWSTR)Value, nBytes) != ERROR_SUCCESS) {
		return FALSE;					
	}

	return TRUE;
}

BOOL CRegistryConfig::SaveElementToRegistry(const CString& Key, DWORD Value)
{
	HKEY hK=HKEY_CURRENT_USER;	
	LPDWORD lpdwDisposition=NULL;

	if (m_hKey==NULL) {
		if (RegCreateKeyEx(hK, (LPCWSTR)m_strSubKey, 0, _T("DWORD"), 0, 0, NULL, &m_hKey, lpdwDisposition) != ERROR_SUCCESS) {
			return FALSE;
		}
	}

	if (RegSetValueEx(m_hKey, (LPCWSTR)Key, 0,REG_DWORD, (const BYTE*)&Value, sizeof(DWORD) )!=ERROR_SUCCESS) {
		return FALSE;					
	}

	return TRUE;
}

BOOL CRegistryConfig::GetValueFromKey(const CString& Key, CString* retval)
{
	if (m_hKey==NULL) {
		return FALSE;
	}

	//HKEY hK = HKEY_CURRENT_USER;	
	BYTE val[MAX_PATH];

	DWORD value_length = MAX_PATH - 1;
	DWORD type = REG_SZ;
	if (RegQueryValueEx(m_hKey, (LPCWSTR)Key, NULL, &type, (LPBYTE)val, &value_length) != ERROR_SUCCESS) {
		return FALSE;
	}

	val[value_length - 1] = 0;
	CString temp = (LPCWSTR)val;	//it's a wide char, so let CString know that (or else you'll get a 1-character string because of 0 termination).
	retval->SetString(temp);
	return TRUE;
}

BOOL CRegistryConfig::GetValueFromKey(const CString& Key, DWORD* retval)
{
	if (m_hKey == NULL) {
		return FALSE;
	}

	//HKEY hK = HKEY_CURRENT_USER;	
	DWORD value_length = sizeof(DWORD);
	DWORD type = REG_DWORD;
	if (RegQueryValueEx(m_hKey, (LPCWSTR)Key, NULL, &type, (LPBYTE)retval, &value_length) != ERROR_SUCCESS) {
		return FALSE;
	}

	return TRUE;
}
