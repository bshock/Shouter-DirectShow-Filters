//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
//          (C) 2013 Cristian Adam
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//- Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//- Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//- Neither the name of Zentaro Kavanagh nor the names of contributors 
//  may be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================
#include "Precompiled.h"
#include "oggmuxfilter.h"
#include "Utilities.h"
#include <strsafe.h>
#include "Constants.h"

WCHAR *g_wszName_Filter = L"Shouter";

CFactoryTemplate g_Templates[] = 
{
    { 
		g_wszName_Filter,			// Name
	    &CLSID_ShouterFilter,            // CLSID
	    OggMuxFilter::CreateInstance,	// Method to create an instance of MyComponent
        NULL,							// Initialization function
        NULL							// Set-up information (for filters)
    }
//	,
//   { 
//		L"Ogg Muxer Properties",		// Name
//	    &CLSID_PropsOggMux,             // CLSID
//	     PropsOggMux::CreateInstance,	// Method to create an instance of MyComponent
//       NULL,							// Initialization function
//       NULL							// Set-up information (for filters)
//   }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 


CUnknown* WINAPI OggMuxFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
    Log::ReportingLevel() = Log::FromString(TEXT("DEBUG"));
	std::wstring logFileName = TEXT("Shouter.log");
	Log::Stream(logFileName);

	OggMuxFilter *pNewObject = new OggMuxFilter();
    if (pNewObject == NULL) 
    {
        *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
} 

void OggMuxFilter::NotifyComplete() 
{
	HRESULT locHR = NotifyEvent(EC_COMPLETE, S_OK, NULL);
    UNREFERENCED_PARAMETER(locHR);
}

STDMETHODIMP OggMuxFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	//Note: if the class is derived from IFileSinkFilter or IFileSourceFilter and you return the interface here, a dialog box asking for a filename will pop up when the filter is loaded.
	//if (riid == IID_IFileSinkFilter) 
 //   {
 //       return GetInterface((IFileSinkFilter*)this, ppv);
	//} 
    if (riid == IID_IAMFilterMiscFlags) 
    {
		LOG(logDEBUG)<<"Queried for IAMMiscFlags"<<endl;
		return GetInterface((IAMFilterMiscFlags*)this, ppv);
	} 
    else if (riid == IID_IMediaSeeking) 
    {
		LOG(logDEBUG)<<"Queried for IMediaSeeking"<<endl;
		return GetInterface((IMediaSeeking*)this, ppv);
	} 
    else if (riid == IID_IOggMuxProgress) 
    {
		LOG(logDEBUG)<<"Queried for IOggMuxProgress"<<endl;
		return GetInterface((IOggMuxProgress*)this, ppv);
	} 
    else if (riid == IID_IOggMuxSettings) 
    {
		return GetInterface((IOggMuxSettings*)this, ppv);
	}
	//else if (riid == IID_ISpecifyPropertyPages) 
    //{
	//	return GetInterface((ISpecifyPropertyPages*)this, ppv);
	//}

	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv); 
}

LONGLONG __stdcall OggMuxFilter::getProgressTime() 
{
	if (mInterleaver != NULL) 
    {
		return mInterleaver->progressTime();
	} 
    else 
    {
		return -1;
	}
}

LONGLONG __stdcall OggMuxFilter::getBytesWritten() 
{
	if (mInterleaver != NULL) 
    {
		return mInterleaver->bytesWritten();
	} 
    else 
    {
		return -1;
	}
}

ULONG OggMuxFilter::GetMiscFlags() 
{
	LOG(logDEBUG)<<"GetMiscflags"<<endl;
	return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}


OggMuxFilter::OggMuxFilter()
	:	CBaseFilter(g_wszName_Filter, NULL, m_pLock, CLSID_ShouterFilter)
	,	mInterleaver(NULL)
{
	mInterleaver = new OggPageInterleaver(this, this);
	//LEAK CHECK:::Both get deleted in constructor.

	m_pLock = new CCritSec;
	mStreamLock = new CCritSec;
	mInputPins.push_back(new OggMuxInputPin(this, m_pLock, &mHR, mInterleaver->newStream()));

	//To avoid a circular reference... we do this without the addref.
	// This is safe because we control the lifetime of this pin, and it won't be deleted until we are.
	IMediaSeeking* locSeeker = (IMediaSeeking*)mInputPins[0];
	SetDelegate(locSeeker);

	//TODO: figure out a better place for the following:
	TCHAR szDebug[1024];
	OutputDebugString(L"ShoutStream:Ctor:GetConfigFromRegistry\n");
	GetConfigFromRegistry();
	shout_init();
	OutputDebugString(L"ShoutStream:Ctor:StreamSetup\n");
	StreamSetup(m_ezConfig.host, m_ezConfig.port, m_ezConfig.mount);

	WCHAR* wPWord = Utilities::C2WC(const_cast<char*>(shout_get_password(m_Shout)));
	StringCchPrintf(szDebug, 511, TEXT("%s: Password=*%s*\n"), g_wszName_Filter, wPWord );
	OutputDebugString(szDebug);
	delete [] wPWord;

}

OggMuxFilter::OggMuxFilter(REFCLSID inFilterGUID)
	:	CBaseFilter(NAME("OggMuxFilter"), NULL, m_pLock, inFilterGUID)
	,	mInterleaver(NULL)
{	
	m_pLock = new CCritSec;
	mStreamLock = new CCritSec;	
}

OggMuxFilter::~OggMuxFilter()
{
	//This is not a leak !! We just don't want it to be released... we never addreffed it.. see constructor.
	SetDelegate(NULL);
	
	delete mInterleaver;
	for (size_t i = 0; i < mInputPins.size(); i++) 
    {
		delete mInputPins[i];
	}

	delete m_pLock;
	delete mStreamLock;
}

HRESULT OggMuxFilter::addAnotherPin() 
{
	mInputPins.push_back(new OggMuxInputPin(this, m_pLock, &mHR, mInterleaver->newStream()));
	return S_OK;
}

//IFileSinkFilter Implementation
HRESULT OggMuxFilter::SetFileName(LPCOLESTR inFileName, const AM_MEDIA_TYPE* inMediaType) 
{
	CAutoLock locLock(m_pLock);
	mFileName = inFileName;

	SetupOutput();
	return S_OK;
}

HRESULT OggMuxFilter::GetCurFile(LPOLESTR* outFileName, AM_MEDIA_TYPE* outMediaType) 
{
	//Return the filename and mediatype of the raw data

    CheckPointer(outFileName, E_POINTER);
    *outFileName = NULL;

    if (!mFileName.empty()) 
    {
    	unsigned int size  = sizeof(WCHAR) * (mFileName.size() + 1);

        *outFileName = (LPOLESTR) CoTaskMemAlloc(size);
        if (*outFileName != NULL) 
        {
              CopyMemory(*outFileName, mFileName.c_str(), size);
        }
    }
	
	return S_OK;
}

bool OggMuxFilter::acceptOggPage(OggPage* inOggPage) 
{			
    //Deletes Page correctly.
	//LOG(logDEBUG)<<"Page accepted... writing..."<<endl;
	unsigned char* locPageData = inOggPage->createRawPageData();

	//mOutputFile.write((char*)locPageData, inOggPage->pageSize());

	if (shout_get_connected(m_Shout) != SHOUTERR_CONNECTED &&
		ReconnectServer(m_Shout, 0) == 0) {
		OutputDebugString(L"CRenderFilter:DoRenderSample:STREAM_SERVERR");
		return E_FAIL;
	}
	else
	{
		OutputDebugString(L"CRenderFilter:DoRenderSample:shout connected");
	}

	TCHAR szDebug[1024];
	WCHAR* time = Utilities::C2WC(const_cast<char*>(Utilities::CurrentDateTime().c_str()));
	StringCchPrintf(szDebug, 511, TEXT("%s:%s:CRenderFilter:DoRenderSample:sample size=%hu\n"), g_wszName_Filter, time, inOggPage->pageSize() );
	OutputDebugString(szDebug);
	delete [] time;

	shout_sync(m_Shout);

	//if (shout_send(m_Shout, pOutFrame, dl) != SHOUTERR_SUCCESS)
	if (shout_send(m_Shout, locPageData, inOggPage->pageSize()) != SHOUTERR_SUCCESS)
	{
		TCHAR szDebug[1024];
		WCHAR* error = Utilities::C2WC(const_cast<char*>(shout_get_error(m_Shout)));
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_send(): %s"), g_wszName_Filter, error );
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

	delete inOggPage;
	delete[] locPageData;
	return true;
}

bool OggMuxFilter::SetupOutput() 
{
	mOutputFile.open(StringHelper::toNarrowStr(mFileName).c_str(), ios_base::out | ios_base::binary);
	return mOutputFile.is_open();
	return true;
}

bool OggMuxFilter::CloseOutput() 
{
	mOutputFile.close();
	return true;
}

//BaseFilter Interface
int OggMuxFilter::GetPinCount() 
{
	//TO DO::: Change this for multiple streams
	return (int)mInputPins.size();
}

CBasePin* OggMuxFilter::GetPin(int inPinNo) 
{
	if ((inPinNo >= 0) && ((size_t)inPinNo < mInputPins.size()) ) 
    {
		return mInputPins[inPinNo];
	} 
    
    return NULL;
}


//IMEdiaStreaming
HRESULT __stdcall OggMuxFilter::Run(REFERENCE_TIME tStart) 
{
	CAutoLock locLock(m_pLock);

	OutputDebugString(L"ShoutStream:StartStreaming\n");
	TCHAR szDebug[1024];
	OutputDebugString(L"ShoutStream:StartStreaming:shout_open attempt\n");
	if (shout_open(m_Shout) == SHOUTERR_SUCCESS) {
		OutputDebugString(L"ShoutStream:StartStreaming:shout_open success\n");
		WCHAR* wHost = Utilities::C2WC(m_ezConfig.host);
		WCHAR* wMount = Utilities::C2WC(m_ezConfig.mount);
		WCHAR* time = Utilities::C2WC(const_cast<char*>(Utilities::CurrentDateTime().c_str()));
		StringCchPrintf(szDebug, 511, TEXT("%s:%s:Connected to http://%s:%hu%s\n"), g_wszName_Filter, time, wHost, m_ezConfig.port, wMount );
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

	return CBaseFilter::Run(tStart);
}

HRESULT __stdcall OggMuxFilter::Pause(void) 
{
	CAutoLock locLock(m_pLock);
	
	HRESULT locHR = CBaseFilter::Pause();	
	return locHR;
}

HRESULT __stdcall OggMuxFilter::Stop() 
{
	CAutoLock locLock(m_pLock);

	OutputDebugString(L"ShoutStream:StopStreaming\n");
	if (m_Shout != NULL) {
		shout_close(m_Shout);
	}

	return CBaseFilter::Stop();
}

STDMETHODIMP OggMuxFilter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop) 
{
	HRESULT locHR = BasicSeekPassThrough::GetPositions(pCurrent, pStop);
	LOG(logDEBUG)<<"GetPos Before : "<<*pCurrent<<" - "<<*pStop<<endl;
	*pCurrent = mInterleaver->progressTime();
	LOG(logDEBUG)<<"GetPos After : "<<*pCurrent<<" - "<<*pStop<<endl;
	return locHR;
}

STDMETHODIMP OggMuxFilter::GetCurrentPosition(LONGLONG *pCurrent) 
{
	*pCurrent = mInterleaver->progressTime();
	LOG(logDEBUG)<<"GetCurrentPos : "<<*pCurrent<<endl;
	return S_OK;
}

bool __stdcall OggMuxFilter::setMaxPacketsPerPage(unsigned long inMaxPacketsPerPage) 
{
	for (std::vector<OggMuxInputPin*>::iterator locPinIterator = mInputPins.begin();
		 locPinIterator != mInputPins.end();
		 locPinIterator++) 
    {
		OggMuxInputPin* locPin = *locPinIterator;
		locPin->SetPaginatorMaximumPacketsPerPage(inMaxPacketsPerPage);
	}

	return true;
}

unsigned long __stdcall OggMuxFilter::maxPacketsPerPage() 
{
	unsigned long locCurrentMaximumPacketsPerPage = 0;

	for (std::vector<OggMuxInputPin*>::iterator locPinIterator = mInputPins.begin();
		 locPinIterator != mInputPins.end();
		 locPinIterator++) 
    {		
		OggMuxInputPin* locPin = *locPinIterator;

		unsigned long locMaximumPacketsPerPageForThisPin =
			locPin->PaginatorMaximumPacketsPerPage();

		if (locMaximumPacketsPerPageForThisPin > locCurrentMaximumPacketsPerPage) 
        {
			locCurrentMaximumPacketsPerPage = locMaximumPacketsPerPageForThisPin;
		}
	}

	return locCurrentMaximumPacketsPerPage;
}

HRESULT OggMuxFilter::GetConfigFromRegistry()
{
	//static bool GetRegValueFromPath(TCHAR* path, TCHAR* valueName, TCHAR* value, int valsize);
	//m_ezConfig

	//TODO: load m_ezConfig from Registry, instead of hard-coding it.

	strcpy(m_ezConfig.host, "10.0.0.160");
	m_ezConfig.port = 9001;
	strcpy(m_ezConfig.mount, "/videolist");
	strcpy(m_ezConfig.URL, "http://10.0.0.160:9001/videolist");
	m_ezConfig.username = NULL;
	memset(m_ezConfig.password, 0, MAXCHAR);
	strcpy(m_ezConfig.password, "globalmed123");
	strcpy(m_ezConfig.format, "THEORA");
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

	return S_OK;
}

HRESULT OggMuxFilter::StreamSetup(const char *host, unsigned short port, const char *mount)
{
	//TODO: look for a non-NULL m_Shout, shut it down, and delete it?	Otherwise, you might have a leak.
	m_Shout = NULL;
	TCHAR szDebug[1024];

	if ((m_Shout = shout_new()) == NULL) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_new(): %s"), g_wszName_Filter, strerror(ENOMEM) );
		OutputDebugString(szDebug);
		return E_FAIL;
	}

	if (shout_set_host(m_Shout, host) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_host(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_protocol(m_Shout, SHOUT_PROTOCOL_HTTP) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_protocol(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_port(m_Shout, port) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_port(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_password(m_Shout, m_ezConfig.password) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_password(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_mount(m_Shout, mount) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_mount(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (shout_set_user(m_Shout, "source") != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_user(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (!strcmp(m_ezConfig.format, MP3_FORMAT) &&
	    shout_set_format(m_Shout, SHOUT_FORMAT_MP3) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_format(MP3): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if ((!strcmp(m_ezConfig.format, VORBIS_FORMAT) ||
	     !strcmp(m_ezConfig.format, THEORA_FORMAT)) &&
	    shout_set_format(m_Shout, SHOUT_FORMAT_OGG) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_format(OGG): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (m_ezConfig.username &&
	    shout_set_user(m_Shout, m_ezConfig.username) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_user(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverName &&
	    shout_set_name(m_Shout, m_ezConfig.serverName) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_name(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverURL &&
	    shout_set_url(m_Shout, m_ezConfig.serverURL) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_url(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverGenre &&
	    shout_set_genre(m_Shout, m_ezConfig.serverGenre) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_genre(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverDescription &&
	    shout_set_description(m_Shout, m_ezConfig.serverDescription) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_description(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverBitrate &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_BITRATE, m_ezConfig.serverBitrate) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_BITRATE): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverChannels &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_CHANNELS, m_ezConfig.serverChannels) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_CHANNELS): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverSamplerate &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_SAMPLERATE, m_ezConfig.serverSamplerate) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_SAMPLERATE): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}
	if (m_ezConfig.serverQuality &&
	    shout_set_audio_info(m_Shout, SHOUT_AI_QUALITY, m_ezConfig.serverQuality) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_audio_info(AI_QUALITY): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	if (shout_set_public(m_Shout, (unsigned int)m_ezConfig.serverPublic) != SHOUTERR_SUCCESS) {
		StringCchPrintf(szDebug, 511, TEXT("%s: shout_set_public(): %s\n"), g_wszName_Filter, shout_get_error(m_Shout) );
		OutputDebugString(szDebug);
		shout_free(m_Shout);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT OggMuxFilter::ReconnectServer(shout_t *shout, int closeConn)
{
	unsigned int i = 0;
	int close_conn = closeConn;
	TCHAR szDebug[1024];

	StringCchPrintf(szDebug, 511, TEXT("%s: Connection to %s lost\n"), g_wszName_Filter, m_ezConfig.URL );
	OutputDebugString(szDebug);

	while (++i)
	{
		StringCchPrintf(szDebug, 511, TEXT("%s: Attempting reconnection #"), g_wszName_Filter);
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
			StringCchPrintf(szDebug, 511, TEXT("OK\n%s: Reconnect to %s successful\n"), g_wszName_Filter, m_ezConfig.URL );
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

		StringCchPrintf(szDebug, 511, TEXT("%s: Waiting 5s for %s to come back ...\n"), g_wszName_Filter, m_ezConfig.URL );
		OutputDebugString(szDebug);

		Sleep(5000);
	};

	StringCchPrintf(szDebug, 511, TEXT("%s: Giving up\n"), g_wszName_Filter);
	OutputDebugString(szDebug);

	return E_FAIL;
}

