#include "StdAfx.h"
#include "DFH.h"
#include <string>
#include <new>

//building graphs with capture graph builder
//https://msdn.microsoft.com/en-us/library/windows/desktop/dd318240(v=vs.85).aspx
//https://msdn.microsoft.com/en-us/library/windows/desktop/dd373396(v=vs.85).aspx
//http://wenku.baidu.com/view/ebfa79687e21af45b307a8b8
//http://extras.springer.com/2000/978-1-893115-76-7/MSDN_VCB/SAMPLES/VC98/SDK/GRAPHICS/DIRECTANIMATION/help/ds/dssd0059.htm

//#pragma comment(lib, "shell32.lib")

const long TIMEOUT = 5000;

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

DFH::DFH():cpSource(NULL), cpEncoder(NULL), cpShouter(NULL), cpMC(NULL), cpGraph(NULL), cpBuilder(NULL)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

}

DFH::~DFH()
{
	cpSource = NULL;
	cpEncoder = NULL;
	cpShouter = NULL;
	cpMC = NULL;
	cpGraph = NULL;
	cpBuilder = NULL;

	std::map<std::wstring, CComPtr<IMoniker> >::iterator iter;
	for (iter = VideoDeviceMap.begin(); iter != VideoDeviceMap.end(); ++iter)
	{
		iter->second = NULL;
	}

	CoUninitialize();
}

void DFH::FillVideoComboBox(CComboBox& combobox)
{
	combobox.Clear();

	std::map<std::wstring, CComPtr<IMoniker> >::iterator iter;
	for (iter = VideoDeviceMap.begin(); iter != VideoDeviceMap.end(); ++iter)
	{
		int index = combobox.AddString(static_cast<LPCTSTR>(iter->first.c_str()));
		combobox.SetItemDataPtr(index, iter->second);
	}
}

void DFH::FillAudioComboBox(CComboBox& combobox)
{
	combobox.Clear();

	std::map<std::wstring, CComPtr<IMoniker> >::iterator iter;
	for (iter = AudioDeviceMap.begin(); iter != AudioDeviceMap.end(); ++iter)
	{
		int index = combobox.AddString(static_cast<LPCTSTR>(iter->first.c_str()));
		combobox.SetItemDataPtr(index, iter->second);
	}
}

void DFH::ShowError(const std::string msg, HRESULT hr)
{
	//TODO: add some ways to show errors!
}

HRESULT DFH::ConnectShouterVideoGraph(IMoniker* pSourceMoniker)
{
	//TODO: create one function for taking down the entire graph, and use it in dtor as well.
	cpSource = NULL;
	cpEncoder = NULL;
	cpShouter = NULL;
	cpMC = NULL;
	cpGraph = NULL;
	cpBuilder = NULL;

	HRESULT hr = S_OK;
	hr = CreateGraph();
	if (hr != S_OK)
	{
		ShowError("ConnectShouterVideoGraph:CreateGraph Failed.", hr);
		return hr;
	}

	CLSID clsid;

	hr = AddFilterByMoniker(cpGraph, pSourceMoniker, &cpSource, _T("Source"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterVideoGraph:Source filter failed to add.", hr);
		return hr;
	}

	//{083863F1-70DE-11D0-BD40-00A0C911CE86}\{5C769985-C3E1-4F95-BEE7-1101C465F5FC}
	//	-- Use the second part
	CLSIDFromString(_T("{5C769985-C3E1-4F95-BEE7-1101C465F5FC}"), &clsid);
	hr = AddFilterByCLSID(cpGraph, clsid, &cpEncoder, _T("Encoder"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterVideoGraph:Encoder filter failed to add.", hr);
		return hr;
	}

	hr = ConnectFilters(cpGraph, cpSource, cpEncoder);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterVideoGraph:Source and Encoder filters failed to connect.", hr);
		return hr;
	}

	//To get OggShoutStream.dll and MP3ShoutStream.dll to register, you need the VS2012 redistributable.
	//	-- Download it, restart (even though in a perfect world you shouldn't have to), and then regsvr32 these two files.
	
	//OggShoutStream
	//@device:sw:{083863F1-70DE-11D0-BD40-00A0C911CE86}\{F0A36589-94E4-41C8-B776-7F96E5F5DF85}
	CLSIDFromString(L"{F0A36589-94E4-41C8-B776-7F96E5F5DF85}", &clsid);	//correct one is the second
	hr = AddFilterByCLSID(cpGraph, clsid, &cpShouter, _T("OggShoutStream"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterVideoGraph:OggShoutStream filter failed to add.", hr);
		return hr;
	}

	hr = ConnectFilters(cpGraph, cpEncoder, cpShouter);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterGraph:Encoder and Shouter filters failed to connect.", hr);
		return hr;
	}

	hr = GetMediaControl(cpGraph, &cpMC);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterGraph:Failed to get media control.", hr);
		return hr;
	}

	return hr;
}

HRESULT DFH::ConnectShouterAudioGraph(IMoniker* pSourceMoniker)
{
	//TODO: create one function for taking down the entire graph, and use it in dtor as well.
	cpSource = NULL;
	cpEncoder = NULL;
	cpShouter = NULL;
	cpMC = NULL;
	cpGraph = NULL;
	cpBuilder = NULL;

	HRESULT hr = S_OK;
	hr = CreateGraph();
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:CreateGraph Failed.", hr);
		return hr;
	}

	hr = GetMediaControl(cpGraph, &cpMC);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:Failed to get media control.", hr);
		return hr;
	}

	hr = AddFilterByMoniker(cpGraph, pSourceMoniker, &cpSource, _T("Source"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:Source filter failed to add.", hr);
		return hr;
	}

	//Mono MP3 encoder
	//{083863F1-70DE-11D0-BD40-00A0C911CE86}\{470C932A-D017-4748-AC0B-73841FFCBB2D}
		//	-- Use the second part
	CLSID clsid;
	CLSIDFromString(_T("{470C932A-D017-4748-AC0B-73841FFCBB2D}"), &clsid);
	hr = AddFilterByCLSID(cpGraph, clsid, &cpEncoder, _T("MP3Encoder"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:MP3Encoder filter failed to add.", hr);
		return hr;
	}

	hr = ConnectFilters(cpGraph, cpSource, cpEncoder);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:Source and MP3Encoder filters failed to connect.", hr);
		return hr;
	}

	//To get Mp3ShoutStream.dll and MP3ShoutStream.dll to register, you need the VS2012 redistributable.
	//	-- Download it, restart (even though in a perfect world you shouldn't have to), and then regsvr32 these two files.
	
	//MP3ShoutStream
	//@device:sw:{083863F1-70DE-11D0-BD40-00A0C911CE86}\{87DE62FF-A0D4-4FC9-A073-316C6B2FFE10}

	CLSIDFromString(L"{87DE62FF-A0D4-4FC9-A073-316C6B2FFE10}", &clsid);	//correct part of the device code is the second part
	hr = AddFilterByCLSID(cpGraph, clsid, &cpShouter, _T("mp3ShoutStream"));
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:Shouter filter failed to add.", hr);
		return hr;
	}

	hr = ConnectFilters(cpGraph, cpEncoder, cpShouter);
	if (hr != S_OK)
	{
		ShowError("ConnectShouterAudioGraph:mp3Encoder and mp3ShoutStream filters failed to connect.", hr);
		return hr;
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT DFH::EnumerateVideoDevicesToMapSP()
{
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        CComPtr<IEnumMoniker> cpEnum;
        hr = EnumerateDevicesSP(CLSID_VideoInputDeviceCategory, &cpEnum);
        if (SUCCEEDED(hr))
        {
            SaveVideoDeviceInformationSP(cpEnum);
            cpEnum = 0;
        }
		else
		{
			ShowError("EnumerateVideoDevices failed.", hr);
		}
    }
	return hr;
}

HRESULT DFH::EnumerateAudioDevicesToMapSP()
{
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr))
    {
        CComPtr<IEnumMoniker> cpEnum;
        hr = EnumerateDevicesSP(CLSID_AudioInputDeviceCategory, &cpEnum);
        if (SUCCEEDED(hr))
        {
            SaveAudioDeviceInformationSP(cpEnum);
            cpEnum = 0;
        }
		else
		{
			ShowError("EnumerateAudioDevices failed.", hr);
		}
    }
	return hr;
}

HRESULT DFH::EnumerateDevicesSP(REFGUID category, IEnumMoniker **ppEnumMoniker)
{
    // Create the System Device Enumerator.
    CComPtr<ICreateDevEnum> cpDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&cpDevEnum));

    if (SUCCEEDED(hr))
    {
        // Create an enumerator for the category.
		hr = cpDevEnum->CreateClassEnumerator(category, ppEnumMoniker, 0);
        if (hr == S_FALSE)
        {
            hr = VFW_E_NOT_FOUND;  // The category is empty. Treat as an error.
        }
		else
		{
			ShowError("EnumerateDevices failed.", hr);
		}
    }
    return hr;
}

HRESULT DFH::SaveVideoDeviceInformationSP(IEnumMoniker *pEnumMoniker)
{
	HRESULT hr = S_OK;
	VideoDeviceMap.clear();
	try
	{
		CComPtr<IMoniker> cpMoniker;
		while (pEnumMoniker->Next(1, &cpMoniker, NULL) == S_OK)
		{
			//Can we get a filter out of this moniker?
			CComPtr<IBaseFilter> cpCaptureFilter;
			if (FAILED(cpMoniker->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<void**>(&cpCaptureFilter))))
			{
				ShowError("SaveVideoDeviceInformation:BindingToObject failed.", hr);
				cpMoniker = 0;
				cpCaptureFilter = 0;
				continue;
			}
			cpCaptureFilter = 0;

			//Check to see if we can connect with an output pin
			//CComPtr<IPin> cpCapturePin;
			//if (FAILED(GetCaptureOutputPin(cpCaptureFilter, &cpCapturePin)))
			//{
			//	cpMoniker = 0;
			//	continue;
			//}

			CComPtr<IPropertyBag> cpPropertyBag;
			if (FAILED(cpMoniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&cpPropertyBag))))
			{
				ShowError("SaveVideoDeviceInformation:BindToStorage failed.", hr);
				cpPropertyBag = 0;
				cpMoniker = 0;
				continue;
			}

			VARIANT variantName;
			variantName.vt = VT_BSTR;
			variantName.bstrVal = 0;
			if (FAILED(cpPropertyBag->Read(L"FriendlyName", &variantName, 0)))
			{
				ShowError("SaveVideoDeviceInformation:Read FriendlyName failed.", hr);
				cpPropertyBag = 0;
				cpMoniker = 0;
				VariantClear(&variantName);
				continue;
			}

			if (variantName.bstrVal)
			{
				VideoDeviceMap.insert(std::pair<std::wstring, CComPtr<IMoniker> >(variantName.bstrVal, cpMoniker.Detach()));
				VariantClear(&variantName);
			}

			cpPropertyBag = 0;
			cpMoniker = 0;
		}
	}
	catch(std::bad_alloc)
	{
		hr = E_OUTOFMEMORY;
		ShowError("SaveVideoDeviceInformation:Out of memory exception.", hr);
	}

	return hr;
}

HRESULT DFH::SaveAudioDeviceInformationSP(IEnumMoniker *pEnumMoniker)
{
	HRESULT hr = S_OK;
	AudioDeviceMap.clear();
	try
	{
		CComPtr<IMoniker> cpMoniker;
		while (pEnumMoniker->Next(1, &cpMoniker, NULL) == S_OK)
		{
			//Can we get a filter out of this moniker?
			CComPtr<IBaseFilter> cpCaptureFilter;
			if (FAILED(cpMoniker->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<void**>(&cpCaptureFilter))))
			{
				ShowError("SaveAudioDeviceInformation:BindToObject failed.", hr);
				cpMoniker = 0;
				cpCaptureFilter = 0;
				continue;
			}
			cpCaptureFilter = 0;

			//Check to see if we can connect with an output pin
			//CComPtr<IPin> cpCapturePin;
			//if (FAILED(GetCaptureOutputPin(cpCaptureFilter, &cpCapturePin)))
			//{
			//	cpMoniker = 0;
			//	continue;
			//}

			CComPtr<IPropertyBag> cpPropertyBag;
			if (FAILED(cpMoniker->BindToStorage(0, 0, IID_IPropertyBag, reinterpret_cast<void**>(&cpPropertyBag))))
			{
				ShowError("SaveAudioDeviceInformation:BindToStorage failed.", hr);
				cpPropertyBag = 0;
				cpMoniker = 0;
				continue;
			}

			VARIANT variantName;
			variantName.vt = VT_BSTR;
			variantName.bstrVal = 0;
			if (FAILED(cpPropertyBag->Read(L"FriendlyName", &variantName, 0)))
			{
				ShowError("SaveAudioDeviceInformation:Read FriendlyName failed.", hr);
				cpPropertyBag = 0;
				cpMoniker = 0;
				VariantClear(&variantName);
				continue;
			}

			if (variantName.bstrVal)
			{
				AudioDeviceMap.insert(std::pair<std::wstring, CComPtr<IMoniker> >(variantName.bstrVal, cpMoniker.Detach()));
				VariantClear(&variantName);
			}

			cpPropertyBag = 0;
			cpMoniker = 0;
		}
	}
	catch(std::bad_alloc)
	{
		hr = E_OUTOFMEMORY;
		ShowError("SaveAudioDeviceInformation:Out of memory exception.", hr);
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////

HRESULT DFH::CreateGraph()
{
	HRESULT hr = S_OK;

	// Create the Filter Graph Manager.
	hr =  CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&cpGraph);

	if (SUCCEEDED(hr))
	{
		// Create the Capture Graph Builder.
		hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&cpBuilder);
		if (SUCCEEDED(hr))
		{
			cpBuilder->SetFiltergraph(cpGraph);
		}
		else
		{
			ShowError("CreateGraph:Get CaptureGraphBuilder2 failed.", hr);
		}
	}
	else
	{
		ShowError("CreateGraph:Get GraphBuild failed.", hr);
	}
	return hr;
}

HRESULT DFH::GetMediaControl(IGraphBuilder *pGraph, IMediaControl **ppMC)
{
	if (pGraph == NULL) {
		return FWP_E_NULL_POINTER;
	}
	HRESULT hr = S_OK;
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)ppMC);
	return hr;
}

HRESULT DFH::MediaControlRun()
{
	//https://msdn.microsoft.com/en-us/library/windows/desktop/dd390177(v=vs.85).aspx

	if (cpMC == NULL) {
		return FWP_E_NULL_POINTER;
	}
	CWaitCursor wait;
	HRESULT hr = cpMC->Run();
	FILTER_STATE FS;
	if (hr == S_FALSE) {	//will return S_FALSE if some of the filters in the graph are still in transition
		hr = cpMC->GetState(TIMEOUT, (OAFilterState*)&FS);
	}
	if (FAILED(hr)) {
		cpMC->Stop();
		ShowError("MediaControlRun:Graph failed to start.", hr);
	}
	return hr;
}

HRESULT DFH::MediaControlPause()
{
	if (cpMC == NULL) {
		return FWP_E_NULL_POINTER;
	}
	CWaitCursor wait;
	HRESULT hr = cpMC->Pause();
	if (FAILED(hr)) {
		cpMC->Stop();
	}
	return hr;
}

HRESULT DFH::MediaControlStop()
{
	if (cpMC == NULL) {
		return FWP_E_NULL_POINTER;
	}
	CWaitCursor wait;
	HRESULT hr = cpMC->Stop();
	return hr;
}

HRESULT DFH::AddFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF, LPCWSTR wszName)
{
    *ppF = 0;

    IBaseFilter *pFilter = NULL;
    
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGraph->AddFilter(pFilter, wszName);
    if (FAILED(hr))
    {
        goto done;
    }

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
	SafeRelease(&pFilter);

    return hr;
}

HRESULT DFH::GetFilterByCLSID(IGraphBuilder *pGraph, REFGUID clsid, IBaseFilter **ppF)
{
    *ppF = 0;

    IBaseFilter *pFilter = NULL;
    
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFilter));
    if (FAILED(hr))

    *ppF = pFilter;
    (*ppF)->AddRef();

	SafeRelease(&pFilter);

    return hr;
}

HRESULT DFH::AddFilterByMoniker(IGraphBuilder *pGraph, IMoniker* pMoniker, IBaseFilter **ppF, LPCWSTR wszName)
{
	if (pMoniker == NULL) return FWP_E_NULL_POINTER;

    *ppF = 0;

    IBaseFilter *pFilter = NULL;
    
	HRESULT hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, reinterpret_cast<void**>(&pFilter));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pGraph->AddFilter(pFilter, wszName);
    if (FAILED(hr))
    {
        goto done;
    }

    *ppF = pFilter;
    (*ppF)->AddRef();

done:
	SafeRelease(&pFilter);

    return hr;
}

HRESULT DFH::AddSourceFilterForMoniker(IGraphBuilder *pGraph, IMoniker* pMoniker, IBaseFilter *pSource, LPCWSTR wszName)
{
	HRESULT hr = S_OK;
	IBindCtx *pContext = 0;
	hr = CreateBindCtx(0, &pContext);
	if (SUCCEEDED(hr))
	{
		IFilterGraph2 *pFG2 = NULL;
		hr = pGraph->QueryInterface(IID_IFilterGraph2, (void**)&pFG2);
		if (SUCCEEDED(hr))
		{
			hr = pFG2->AddSourceFilterForMoniker(pMoniker, pContext, wszName, &pSource);
			pFG2->Release();
		}
		pContext->Release();
	}
	pMoniker->Release();
	return hr;
}

HRESULT DFH::IsPinConnected(IPin *pPin, BOOL *pResult)
{
    IPin *pTmp = NULL;
    HRESULT hr = pPin->ConnectedTo(&pTmp);
    if (SUCCEEDED(hr))
    {
        *pResult = TRUE;
    }
    else if (hr == VFW_E_NOT_CONNECTED)
    {
        // The pin is not connected. This is not an error for our purposes.
        *pResult = FALSE;
        hr = S_OK;
    }
	SafeRelease(&pTmp);

	return hr;
}

HRESULT DFH::IsPinDirection(IPin *pPin, PIN_DIRECTION dir, BOOL *pResult)
{
    PIN_DIRECTION pinDir;
    HRESULT hr = pPin->QueryDirection(&pinDir);
    if (SUCCEEDED(hr))
    {
        *pResult = (pinDir == dir);
    }
    return hr;
}

HRESULT DFH::MatchPin(IPin *pPin, PIN_DIRECTION direction, BOOL bShouldBeConnected, BOOL *pResult)
{
    BOOL bMatch = FALSE;
    BOOL bIsConnected = FALSE;

    HRESULT hr = IsPinConnected(pPin, &bIsConnected);
    if (SUCCEEDED(hr))
    {
        if (bIsConnected == bShouldBeConnected)
        {
            hr = IsPinDirection(pPin, direction, &bMatch);
        }
    }

    if (SUCCEEDED(hr))
    {
        *pResult = bMatch;
    }
    return hr;
}

HRESULT DFH::FindUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    IEnumPins *pEnum = NULL;
    IPin *pPin = NULL;
    BOOL bFound = FALSE;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
    {
        goto done;
    }

    while (S_OK == pEnum->Next(1, &pPin, NULL))
    {
        hr = MatchPin(pPin, PinDir, FALSE, &bFound);
        if (FAILED(hr))
        {
            goto done;
        }
        if (bFound)
        {
            *ppPin = pPin;
            (*ppPin)->AddRef();
            break;
        }
		SafeRelease(&pPin);
    }
    if (!bFound)
    {
        hr = VFW_E_NOT_FOUND;
    }

done:
    SafeRelease(&pPin);
    SafeRelease(&pEnum);
    return hr;
}

//Connect filter to output pin
HRESULT DFH::ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest)
{
    IPin *pIn = NULL;
        
    HRESULT hr = FindUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
    if (SUCCEEDED(hr))
    {
        hr = pGraph->Connect(pOut, pIn);
        pIn->Release();
    }
    return hr;
}

//Connect filter to input pin
HRESULT DFH::ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IPin *pIn)
{
    IPin *pOut = NULL;
        
    HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        hr = pGraph->Connect(pOut, pIn);
        pOut->Release();
    }
    return hr;
}

//Connect two filters
HRESULT DFH::ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest)
{
    IPin *pOut = NULL;

    HRESULT hr = FindUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
    if (SUCCEEDED(hr))
    {
        hr = ConnectFilters(pGraph, pOut, pDest);
        pOut->Release();
    }
    return hr;
}
