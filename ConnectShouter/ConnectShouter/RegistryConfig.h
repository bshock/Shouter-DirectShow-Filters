#pragma once

#define PTLIII_REGISTRY_KEY "SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"

class CRegistryConfig
{
protected: 
	CString m_strSubKey;
	HKEY m_hKey;

public:
	CRegistryConfig();
	CRegistryConfig( const CString& key );
	~CRegistryConfig();

	BOOL SaveElementToRegistry(const CString& Key, const CString& Value);
	BOOL SaveElementToRegistry(const CString& Key, DWORD value);
	BOOL GetValueFromKey(const CString& Key, CString* retval);
	BOOL GetValueFromKey(const CString& Key, DWORD* retval);

	static TCHAR* MP3SHOUTSTREAM_PATH;
	static TCHAR* OGGSHOUTSTREAM_PATH;
};

