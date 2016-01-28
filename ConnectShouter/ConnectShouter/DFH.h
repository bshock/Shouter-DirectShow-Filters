#pragma once

#include <Windows.h>
#include <DShow.h>
#include <map>

class DFH
{
	//CComPtr is too much like auto_ptr -- you can't safely put it in a collection because of its reference counting.
	std::map<std::wstring, CComPtr<IMoniker> > VideoDeviceMap;
	std::map<std::wstring, CComPtr<IMoniker> > AudioDeviceMap;

public:
	DFH();
	~DFH();

	HRESULT EnumerateVideoDevicesToMapSP();
	HRESULT EnumerateAudioDevicesToMapSP();
	void FillVideoComboBox(CComboBox& combobox);
	void FillAudioComboBox(CComboBox& combobox);

	HRESULT ConnectShouterVideoGraph(IMoniker* pSourceMoniker);
	HRESULT ConnectShouterAudioGraph(IMoniker* pSourceMoniker);
	HRESULT MediaControlRun();
	HRESULT MediaControlPause();
	HRESULT MediaControlStop();

private:
	HRESULT EnumerateDevicesSP(REFGUID category, IEnumMoniker **ppEnumMoniker);
	HRESULT SaveVideoDeviceInformationSP(IEnumMoniker *pEnumMoniker);
	HRESULT SaveAudioDeviceInformationSP(IEnumMoniker *pEnumMoniker);

	HRESULT CreateGraph();
	HRESULT GetMediaControl(IGraphBuilder *pGraph, IMediaControl **ppMC);

	HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName);
	HRESULT GetFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF);
	HRESULT AddFilterByMoniker(IGraphBuilder *pGraph, IMoniker* pMoniker, IBaseFilter **ppF, LPCWSTR wszName);
	HRESULT AddSourceFilterForMoniker(IGraphBuilder *pGraph, IMoniker* pMoniker, IBaseFilter *pSource, LPCWSTR wszName);
	HRESULT IsPinConnected(IPin *pPin, BOOL *pResult);
	HRESULT IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult);
	HRESULT MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult);
	HRESULT FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn);
	HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);

	void ShowError(const std::string msg, HRESULT hr);

private:
    CComPtr<IMediaControl> cpMC;
	CComPtr<IGraphBuilder> cpGraph;
	CComPtr<ICaptureGraphBuilder2> cpBuilder;

	CComPtr<IBaseFilter> cpSource;
	CComPtr<IBaseFilter> cpEncoder;
	CComPtr<IBaseFilter> cpShouter;

};

