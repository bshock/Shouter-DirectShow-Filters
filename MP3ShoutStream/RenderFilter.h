#pragma once

#include <strmif.h>
#include <MMSystem.h>
#include <mtype.h>
#include <streams.h>
#include <combase.h>

#include "Utilities.h"

#include <shout/shout.h>

extern const GUID CLSID_RenderFilter;
extern const GUID IID_IRenderFilter;

//
// Global namespace functions
//


struct EZCONFIG {
	char host[MAXCHAR];
	int port;
	char mount[MAXCHAR];

	char URL[MAXCHAR];
	char* username;
	char password[MAXCHAR];
	char format[MAXCHAR];
	char* fileName;
	char* metadataProgram;
	char* metadataFormat;
	char* serverName;
	char* serverURL;
	char* serverGenre;
	char* serverDescription;
	char* serverBitrate;
	char* serverChannels;
	char* serverSamplerate;
	char* serverQuality;
	int	serverPublic;
	int fileNameIsProgram;
	unsigned int reconnectAttempts;
};

//struct EZCONFIG {
//	char host[MAXCHAR];
//	int port;
//	char mount[MAXCHAR];
//
//	char URL[MAXCHAR];
//	char username[MAXCHAR];
//	char password[MAXCHAR];
//	char format[MAXCHAR];
//	char fileName[MAXCHAR];
//	char metadataProgram[MAXCHAR];
//	char metadataFormat[MAXCHAR];
//	char serverName[MAXCHAR];
//	char serverURL[MAXCHAR];
//	char serverGenre[MAXCHAR];
//	char serverDescription[MAXCHAR];
//	char serverBitrate[MAXCHAR];
//	char serverChannels[MAXCHAR];
//	char serverSamplerate[MAXCHAR];
//	char serverQuality[MAXCHAR];
//	int	serverPublic;
//	int fileNameIsProgram;
//	unsigned int reconnectAttempts;
//};


// Duplicate of ATL's CComPtr class

extern IUnknown* MyAtlComPtrAssign(IUnknown** pp, IUnknown* lp);


DECLARE_INTERFACE_(IRenderFilter, IUnknown)
{
    // Called by the INSPECX to set the incomming media format.
    virtual HRESULT SetTargetFormat(CMySize& dim, DOUBLE rate) = 0;
    STDMETHOD(SetTargetFormatEx)(int dimX, int dimY, float rate) PURE;
	STDMETHOD(SetCompleteFormat)(int inX, int inY, int outX, int outY, float rate);
    STDMETHOD(GetTargetFormat)(CMediaType* pmtype) PURE;
};

extern WCHAR* g_wszName_RenderFilter;
extern const AMOVIESETUP_MEDIATYPE sudPinTypes_RenderFilter[1];
extern const AMOVIESETUP_FILTER sudSampVid_RenderFilter;
extern REGFILTER2 rf2FilterReg_RenderFilter;

class CRenderFilter :
    public CBaseRenderer,
    public IRenderFilter
{
public:
    DECLARE_IUNKNOWN

    CRenderFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CRenderFilter(void);

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // Override these from CBaseRenderer
	virtual HRESULT SetMediaType(const CMediaType *pmt);
    virtual HRESULT CheckMediaType(const CMediaType* pmtIn);
    virtual HRESULT DoRenderSample(IMediaSample* pMediaSample);
	virtual HRESULT StartStreaming();
	virtual HRESULT StopStreaming();

    // IRenderFilter Implementation
    HRESULT SetTargetFormat(CMySize& dim, DOUBLE rate);
    STDMETHODIMP SetTargetFormatEx(int dimX, int dimY, float rate);
    STDMETHODIMP SetCompleteFormat(int inX, int inY, int outX, int outY, float rate);
    STDMETHODIMP GetTargetFormat(CMediaType* pmtype);

private:
    // Targeted format for negotiation of input pin connection
    CMySize m_MasterDim;
    DOUBLE m_MasterRate;
    CMediaType m_TargetFormat;

	//Extra stuff (available, but not used right now)
	VIDEOINFOHEADER m_videoInfo;
	PixelFormat m_pixFmt;
	int m_stride;
    int m_bytesPerPixel;

	//New stuff
	EZCONFIG m_ezConfig;
	shout_t* m_Shout;

	HRESULT StreamSetup(const char *host, unsigned short port, const char *mount);
	HRESULT GetConfigFromRegistry();
	HRESULT CRenderFilter::ReconnectServer(shout_t *shout, int closeConn);

    // Disable these
    CRenderFilter(const CRenderFilter&);
    const CRenderFilter& operator = (const CRenderFilter&);
};


// The smart pointer type for this class
typedef CMyComPtr<IRenderFilter> RenderFilterPtr;


class CRenderFilterInputPin :
    public CRendererInputPin
{
public:
    CRenderFilterInputPin(CBaseRenderer *pRenderer,
                              HRESULT *phr,
                              LPCWSTR Name);
    virtual ~CRenderFilterInputPin(void);

    // Overidden to inform the filter graph of the desired format.
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
};
