#include "StdAfx.h"

#include "RenderFilter.h"

#include <atlcomcli.h>
#include <cmath>
#include <new>
#include <amfilter.h>
#include <Guiddef.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <wxdebug.h>

#include "FilterTypes.h"

using namespace std;


#define MP3_FORMAT			"MP3"
#define VORBIS_FORMAT		"VORBIS"
#define THEORA_FORMAT		"THEORA"

// Setup data
WCHAR *g_wszName_RenderFilter = L"MP3ShoutStream";
char *g_Name_RenderFilter = "MP3ShoutStream";

const AMOVIESETUP_MEDIATYPE sudPinTypes_RenderFilter[1] =
{
	{	&MEDIATYPE_Audio,	&MEDIASUBTYPE_MP3	}
};

const AMOVIESETUP_PIN sudPins_RenderFilter =
{
    L"Input",                       // Name of the pin
    TRUE,                           // Is pin rendered
    FALSE,                          // Is an output pin
    FALSE,                          // Ok for no pins
    FALSE,                          // Allowed many
    &CLSID_NULL,                    // Connects to filter
    L"Output",                      // Connects to pin
    1,                              // Number of pin types
    sudPinTypes_RenderFilter    // Details for pins
};

const AMOVIESETUP_FILTER sudSampVid_RenderFilter =
{
    &CLSID_RenderFilter,        // Filter CLSID
    g_wszName_RenderFilter,     // Filter name
    MERIT_DO_NOT_USE,               // Filter merit
    1,                              // Number pins
    &sudPins_RenderFilter       // Pin details
};


//TODO: determine if commented code is necessary, and if it isn't, delete it

//REGFILTER2 rf2FilterReg_RenderFilter =
//{
//    1,                              // Version 1 (no pin mediums or pin category).
//    MERIT_NORMAL,                   // Merit.
//    1,                              // Number of pins.
//    &sudPins_RenderFilter       // Pointer to pin information.
//};


//
//  Object creation stuff
//
CFactoryTemplate g_Templates[] =
{
    g_wszName_RenderFilter, &CLSID_RenderFilter, CRenderFilter::CreateInstance, NULL, &sudSampVid_RenderFilter
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    


// CreateInstance

 //This goes in the factory template table to create new filter instances

CUnknown * WINAPI CRenderFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	//OutputDebugString(L"CRenderFilter:CreateInstance1");
    ASSERT(phr);
	//OutputDebugString(L"CRenderFilter:CreateInstance2");    
    CRenderFilter *pNewObject = new CRenderFilter(g_wszName_RenderFilter, pUnk, phr);
	//OutputDebugString(L"CRenderFilter:CreateInstance3");    
    if (pNewObject == NULL) {
		//OutputDebugString(L"CRenderFilter:CreateInstance3a:out of memory failure");    
        if (phr)
		{
            *phr = E_OUTOFMEMORY;
		}
    }

	//OutputDebugString(L"CRenderFilter:CreateInstance4:successful end");    
    return pNewObject;
}

CRenderFilter::CRenderFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    CBaseRenderer(CLSID_RenderFilter, pName, pUnk, phr),
    m_MasterDim(0, 0),
    m_MasterRate(0.0)
{
	//OutputDebugString(L"CRenderFilter:ctor1");

    m_TargetFormat.InitMediaType();

	//OutputDebugString(L"CRenderFilter:ctor2");

    m_pInputPin = new (nothrow) CRenderFilterInputPin(this, phr, L"Input");

	//OutputDebugString(L"CRenderFilter:ctor3");

    if (m_pInputPin == 0)
    {
		//OutputDebugString(L"CRenderFilter:ctor3a:out of memory failure");

        *phr = E_OUTOFMEMORY;
        return;
    }

    if (FAILED(*phr))
    {
		//OutputDebugString(L"CRenderFilter:ctor3a:FAILED");

        delete m_pInputPin;
        m_pInputPin = 0;
        return;
    }

	//TODO: figure out a better place for the following:
	TCHAR szDebug[1024];
	OutputDebugString(L"ShoutStream:Ctor:GetConfigFromRegistry\n");
	GetConfigFromRegistry();
	shout_init();
	OutputDebugString(L"ShoutStream:Ctor:StreamSetup\n");
	StreamSetup(m_ezConfig.host, m_ezConfig.port, m_ezConfig.mount);

	WCHAR* wPWord = Utilities::C2WC(const_cast<char*>(shout_get_password(m_Shout)));
	StringCchPrintf(szDebug, 511, TEXT("%s: Password=*%s*\n"), g_wszName_RenderFilter, wPWord );
	OutputDebugString(szDebug);
	delete [] wPWord;

}

CRenderFilter::~CRenderFilter(void)
{
	OutputDebugString(L"CRenderFilter:dtor");
    m_TargetFormat.InitMediaType();

	if (m_Shout != NULL) {
		shout_close(m_Shout);
		delete m_Shout;
	}

	//OutputDebugString(L"CRenderFilter:dtor end");
}

STDMETHODIMP CRenderFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv, E_POINTER);

    if (riid == IID_IRenderFilter)
    {
        return GetInterface(static_cast<IRenderFilter*>(this), ppv);
    }

    return CBaseRenderer::NonDelegatingQueryInterface(riid, ppv);

}

HRESULT CRenderFilter::SetMediaType(const CMediaType *pmt)
{
	return S_OK;
}

HRESULT CRenderFilter::CheckMediaType(const CMediaType* pmtIn)
{
	OutputDebugString(L"CRenderFilter:CheckMediaType1");
    if (!pmtIn)
        return E_POINTER;
	OutputDebugString(L"CRenderFilter:CheckMediaType2");

	if (pmtIn->majortype != MEDIATYPE_Audio)
	{
        return VFW_E_TYPE_NOT_ACCEPTED;
	}
	if (pmtIn->subtype != MEDIASUBTYPE_MP3)
	{
        return VFW_E_TYPE_NOT_ACCEPTED;
	}

	OutputDebugString(L"CRenderFilter:CheckMediaType2b:Successful end");
    return S_OK;
}

HRESULT CRenderFilter::StartStreaming()
{
	OutputDebugString(L"ShoutStream:StartStreaming\n");
	TCHAR szDebug[1024];
	OutputDebugString(L"ShoutStream:StartStreaming:shout_open attempt\n");
	if (shout_open(m_Shout) == SHOUTERR_SUCCESS) {
		OutputDebugString(L"ShoutStream:StartStreaming:shout_open success\n");
		WCHAR* wHost = Utilities::C2WC(m_ezConfig.host);
		WCHAR* wMount = Utilities::C2WC(m_ezConfig.mount);
		WCHAR* time = Utilities::C2WC(const_cast<char*>(Utilities::CurrentDateTime().c_str()));
		StringCchPrintf(szDebug, 511, TEXT("%s:%s:Connected to http://%s:%hu%s\n"), g_wszName_RenderFilter, time, wHost, m_ezConfig.port, wMount );
		OutputDebugString(szDebug);
		delete [] wHost;
		delete [] wMount;
		delete [] time;
	}
	else
	{
		WCHAR* wError = Utilities::C2WC(const_cast<char*>(shout_get_error(m_Shout)));
		StringCchPrintf(szDebug, 511, TEXT("FAILED: %s\n"), wError);
		OutputDebugString(szDebug);
		delete [] wError;
	}

	return CBaseRenderer::StartStreaming();
}

HRESULT CRenderFilter::StopStreaming()
{
	OutputDebugString(L"ShoutStream:StopStreaming\n");
	if (m_Shout != NULL) {
		shout_close(m_Shout);
	}
	return CBaseRenderer::StopStreaming();
}

HRESULT CRenderFilter::DoRenderSample(IMediaSample* pMediaSample)
{
	OutputDebugString(L"CRenderFilter:DoRenderSample");

    CHECK_POINTER(pMediaSample);
	//OutputDebugString(L"CRenderFilter:DoRenderSample2");

	if (shout_get_connected(m_Shout) != SHOUTERR_CONNECTED &&
		ReconnectServer(m_Shout, 0) == 0) {
		OutputDebugString(L"CRenderFilter:DoRenderSample:STREAM_SERVERR");
		return E_FAIL;
	}
	else
	{
		OutputDebugString(L"CRenderFilter:DoRenderSample:shout connected");
	}

    BYTE* pOutFrame = 0;
    HRESULT hr = pMediaSample->GetPointer(&pOutFrame);
	size_t dl = pMediaSample->GetActualDataLength();
	if (pMediaSample->GetActualDataLength() > 0 && (pOutFrame[0] & 128) != 0) 
    {
		OutputDebugString(L"CRenderFilter:DoRenderSample:THIS IS A HEADER!!!");
	}
    if (FAILED(hr))
	{
		OutputDebugString(L"CRenderFilter:DoRenderSample:ERROR:pMediaSample pointer invalid.");
        return hr;
	}

	TCHAR szDebug[1024];
	WCHAR* time = Utilities::C2WC(const_cast<char*>(Utilities::CurrentDateTime().c_str()));
	StringCchPrintf(szDebug, 511, TEXT("%s:%s:CRenderFilter:DoRenderSample:sample size=%hu\n"), g_wszName_RenderFilter, time, dl );
	OutputDebugString(szDebug);
	delete [] time;

	shout_sync(m_Shout);

	if (shout_send(m_Shout, pOutFrame, dl) != SHOUTERR_SUCCESS)
	//if (shout_send_raw(m_Shout, pOutFrame, dl) != SHOUTERR_SUCCESS)
	{
		TCHAR szDebug[1024];
		WCHAR* error = Utilities::C2WC(const_cast<char*>(shout_get_error(m_Shout)));
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_send(): %s"), g_wszName_RenderFilter, error );
		OutputDebugString(szDebug);
		delete [] error;
		if (!ReconnectServer(m_Shout, 1))
		{
			OutputDebugString(L"CRenderFilter:STREAM_SERVERR");
			return E_FAIL;
		}
	}
	else
	{
		OutputDebugString(L"CRenderFilter:DoRenderSample:shout_send:SUCCESS");
	}
    return S_OK;
}

HRESULT CRenderFilter::SetTargetFormat(CMySize& dim, DOUBLE rate)
{
    return S_OK;
}

HRESULT CRenderFilter::SetTargetFormatEx(int dimX, int dimY, float rate)
{
	return S_OK;
}

HRESULT CRenderFilter::SetCompleteFormat(int inX, int inY, int outX, int outY, float rate)
{
	CMySize sizeOut(outX, outY);
	//m_VideoServer.SetOutDimensions(sizeOut);
	CMySize sizeIn(inX, inY);
	return SetTargetFormat(sizeIn, rate);
}

HRESULT CRenderFilter::GetTargetFormat(CMediaType* pmtype)
{
	OutputDebugString(L"CRenderFilter::GetTargetFormat");
    CHECK_POINTER(pmtype)
    *pmtype = m_TargetFormat;
    return S_OK;
}

HRESULT CRenderFilter::GetConfigFromRegistry()
{
	strcpy(m_ezConfig.host, "10.0.0.160");
	m_ezConfig.port = 9001;
	strcpy(m_ezConfig.mount, "/mp3list");
	strcpy(m_ezConfig.URL, "http://10.0.0.160:9001/mp3list");
	m_ezConfig.username = NULL;
	memset(m_ezConfig.password, 0, MAXCHAR);
	strcpy(m_ezConfig.password, "globalmed123");
	strcpy(m_ezConfig.format, "MP3");
	m_ezConfig.fileName = NULL;

	m_ezConfig.metadataProgram = NULL;
	m_ezConfig.metadataFormat = NULL;
	m_ezConfig.serverName = NULL;
	m_ezConfig.serverURL = NULL;
	m_ezConfig.serverGenre = NULL;
	m_ezConfig.serverDescription = NULL;
	m_ezConfig.serverBitrate = NULL;
	m_ezConfig.serverChannels = NULL;
	m_ezConfig.serverSamplerate = NULL;
	m_ezConfig.serverQuality = NULL;

	m_ezConfig.serverPublic = 1;
	m_ezConfig.fileNameIsProgram = 0;
	m_ezConfig.reconnectAttempts = 3;

	TCHAR val[MAX_PATH];

	memset(val, 0, MAX_PATH*sizeof(TCHAR));
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("host"), val, MAX_PATH)) {
		char* host = Utilities::WC2C(val);
		strcpy(m_ezConfig.host, host);
		delete [] host;
	}

	DWORD dwVal = 0;
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("port"), &dwVal)) {
		m_ezConfig.port = (int)dwVal;
	}

	memset(val, 0, MAX_PATH*sizeof(TCHAR));
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("mount"), val, MAX_PATH)) {
		char* mount = Utilities::WC2C(val);
		strcpy(m_ezConfig.mount, mount);
		delete [] mount;
	}

	memset(val, 0, MAX_PATH*sizeof(TCHAR));
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("URL"), val, MAX_PATH)) {
		char* url = Utilities::WC2C(val);
		strcpy(m_ezConfig.URL, url);
		delete [] url;
	}

	memset(val, 0, MAX_PATH*sizeof(TCHAR));
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("password"), val, MAX_PATH)) {
		char* password = Utilities::WC2C(val);
		strcpy(m_ezConfig.password, password);
		delete [] password;
	}

	//Note: GetRegValueFromPath doesn't like to reuse DWORDs sent into it. You could see a crash when it goes into StreamSetup
	DWORD dwVal2 = 0;
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("serverpublic"), &dwVal2)) {
		m_ezConfig.serverPublic = (int)dwVal2;
	}

	DWORD dwVal3 = 0;
	if (Utilities::GetRegValueFromPath(TEXT("SOFTWARE\\DSFILTERS\\MP3SHOUTSTREAM"), TEXT("reconnectattempts"), &dwVal3)) {
		m_ezConfig.reconnectAttempts = (int)dwVal3;
	}

	return S_OK;
}

HRESULT CRenderFilter::StreamSetup(const char *host, unsigned short port, const char *mount)
{
	//TODO: look for a non-NULL m_Shout, shut it down, and delete it?	Otherwise, you might have a leak.
	m_Shout = NULL;
	TCHAR szDebug[1024];

	if ((m_Shout = shout_new()) == NULL) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_new(): %s"), g_wszName_RenderFilter, strerror(ENOMEM) );
		OutputDebugString(szDebug);
		return E_FAIL;
	}

	if (shout_set_host(m_Shout, host) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_host(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_protocol(m_Shout, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_protocol(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_port(m_Shout, port) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_port(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_password(m_Shout, m_ezConfig.password) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_password(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_mount(m_Shout, mount) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_mount(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_user(m_Shout, "source") != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_user(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (!strcmp(m_ezConfig.format, MP3_FORMAT) &&
	    shout_set_format(m_Shout, SHOUT_FORMAT_MP3) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_format(MP3): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if ((!strcmp(m_ezConfig.format, VORBIS_FORMAT) ||
	     !strcmp(m_ezConfig.format, THEORA_FORMAT)) &&
	    shout_set_format(m_Shout, SHOUT_FORMAT_OGG) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_format(OGG): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (m_ezConfig.username &&
	    shout_set_user(m_Shout, m_ezConfig.username) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_user(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverName &&
	    shout_set_name(m_Shout, m_ezConfig.serverName) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_name(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverURL &&
	    shout_set_url(m_Shout, m_ezConfig.serverURL) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_url(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverGenre &&
	    shout_set_genre(m_Shout, m_ezConfig.serverGenre) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_genre(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverDescription &&
	    shout_set_description(m_Shout, m_ezConfig.serverDescription) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_description(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverBitrate &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_BITRATE, m_ezConfig.serverBitrate) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_BITRATE): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverChannels &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_CHANNELS, m_ezConfig.serverChannels) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_CHANNELS): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverSamplerate &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_SAMPLERATE, m_ezConfig.serverSamplerate) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_SAMPLERATE): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverQuality &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_QUALITY, m_ezConfig.serverQuality) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_QUALITY): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (shout_set_public(m_Shout, (unsigned int)m_ezConfig.serverPublic) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_public(): %s\n"), g_wszName_RenderFilter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CRenderFilter::ReconnectServer(shout_t *shout, int closeConn)
{
	unsigned int i = 0;
	int close_conn = closeConn;
	TCHAR szDebug[1024];

	StringCchPrintf(szDebug, 511, TEXT("%s: Connection to %s lost\n"), g_wszName_RenderFilter, m_ezConfig.URL );
	OutputDebugString(szDebug);

	while (++i)
	{
		StringCchPrintf(szDebug, 511, TEXT("%s: Attempting reconnection #"), g_wszName_RenderFilter);
		OutputDebugString(szDebug);

		if (m_ezConfig.reconnectAttempts > 0)
		{
			StringCchPrintf(szDebug, 511, TEXT("%u/%u: "), i, m_ezConfig.reconnectAttempts );
			OutputDebugString(szDebug);
		}
		else
		{
			StringCchPrintf(szDebug, 511, TEXT("%u: "), i );
			OutputDebugString(szDebug);
		}
		if (close_conn == 0)
		{
			close_conn = 1;
		}
		else
		{
			shout_close(shout);
		}
		if (shout_open(shout) == SHOUTERR_SUCCESS)
		{
			StringCchPrintf(szDebug, 511, TEXT("OK\n%s: Reconnect to %s successful\n"), g_wszName_RenderFilter, m_ezConfig.URL );
			OutputDebugString(szDebug);
			return S_OK;
		}

		StringCchPrintf(szDebug, 511, TEXT("FAILED: %s\n"), shout_get_error(shout) );
		OutputDebugString(szDebug);

		if (m_ezConfig.reconnectAttempts > 0 &&
		    i >= m_ezConfig.reconnectAttempts)
		{
			break;
		}

		StringCchPrintf(szDebug, 511, TEXT("%s: Waiting 5s for %s to come back ...\n"), g_wszName_RenderFilter, m_ezConfig.URL );
		OutputDebugString(szDebug);

		Sleep(5000);
	};

	StringCchPrintf(szDebug, 511, TEXT("%s: Giving up\n"), g_wszName_RenderFilter);
	OutputDebugString(szDebug);

	return E_FAIL;
}




//
// CRenderFilterInputPin
//

CRenderFilterInputPin::CRenderFilterInputPin(CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name) :
    CRendererInputPin(pRenderer,phr, Name)
{
	OutputDebugString(L"CRenderFilterInputPin:ctor");
}

CRenderFilterInputPin::~CRenderFilterInputPin(void)
{
	OutputDebugString(L"CRenderFilterInputPin:dtor");
}

HRESULT CRenderFilterInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	OutputDebugString(L"CRenderFilterInputPin:CheckMediaType1");
    CHECK_POINTER(pMediaType)
	//OutputDebugString(L"CRenderFilterInputPin:CheckMediaType2");
    if (iPosition > 0)
	{
		OutputDebugString(L"CRenderFilterInputPin:CheckMediaType2a:VFW S NO MORE ITEMS");
        return VFW_S_NO_MORE_ITEMS;
	}

    if (iPosition < 0)
	{
		OutputDebugString(L"CRenderFilterInputPin:CheckMediaType2b:INVALIDARG");
        return E_INVALIDARG;
	}

	//OutputDebugString(L"CRenderFilterInputPin:CheckMediaType3");
    HRESULT hr = S_OK;
    CComPtr<IRenderFilter> spRenderFilter;
    hr = m_pRenderer->QueryInterface(IID_IRenderFilter, reinterpret_cast<void**>(&spRenderFilter));
    if (SUCCEEDED(hr))
	{
		OutputDebugString(L"CRenderFilterInputPin:CheckMediaType4");
        spRenderFilter->GetTargetFormat(pMediaType);
	}
	OutputDebugString(L"CRenderFilterInputPin:CheckMediaType:successful end");
    return hr;
}

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
