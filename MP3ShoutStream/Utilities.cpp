#include "stdafx.h"
#include "Utilities.h"

#include <new>
#include <cmath>
#include <atlcomcli.h>
#include <amfilter.h>
#include <Guiddef.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <wxdebug.h>

#include <time.h>

#include "FilterTypes.h"

using namespace std;

//////////////////////////////////////////////
//Utilities junk functions
//////////////////////////////////////////////
DOUBLE Utilities::k100NS_UNITS = 10000000.0;    

HRESULT Utilities::MakeBITMAPINFOHEADER(REFGUID subType,
                                        WORD bitsPerPixel,
                                        LONG frameWidth,
                                        LONG frameHeight,
                                        BITMAPINFOHEADER** ppresult)
{
    if (ppresult == 0)
        return E_POINTER;

    *ppresult = 0;

    // Get the format fourcc. For all formats except RGB,
    // the fourcc is in the Data1 field of the GUID.
    DWORD fourcc = subType.Data1;

    // Is this an RGB format GUID?
    if (IsEqualGUID(subType, MEDIASUBTYPE_ARGB32) ||
        !memcmp(&(MEDIASUBTYPE_RGB24.Data2), &(subType.Data2), sizeof(GUID) - sizeof(long)))
    {
        fourcc = BI_RGB;
    }
	
    try
    {
        *ppresult = new BITMAPINFOHEADER;
        memset(*ppresult, 0, sizeof(BITMAPINFOHEADER));
    }
    catch (bad_alloc)
    {
        delete *ppresult;
        *ppresult = 0;
    }

    // Exit if BITMAPINFO could not be allocated
    if (*ppresult == 0)
        return E_OUTOFMEMORY;

    (*ppresult)->biSize = sizeof(BITMAPINFOHEADER);
    (*ppresult)->biWidth = frameWidth;
    (*ppresult)->biHeight = frameHeight;
    (*ppresult)->biPlanes = 1;
    (*ppresult)->biBitCount = (bitsPerPixel != -1) ? bitsPerPixel : 0;
    (*ppresult)->biCompression = fourcc;

    if (frameWidth != 0 && frameHeight != 0 && bitsPerPixel != 0)
        (*ppresult)->biSizeImage = DIBSIZE(**ppresult);

    return S_OK;
}

DOUBLE Utilities::RTU_TO_FPS(REFERENCE_TIME units)
{
    DWORD ifps = static_cast<DWORD>((k100NS_UNITS / static_cast<DOUBLE>(units)) * 100.0);
    return (static_cast<DOUBLE>(ifps)) / 100.0;
}

REFERENCE_TIME Utilities::FPS_TO_RTU(DOUBLE fps)
{
    return static_cast<REFERENCE_TIME>(floor(k100NS_UNITS / fps));
}

BOOL Utilities::GetVideoFormatInfo(const AM_MEDIA_TYPE& mType,
                                   GUID& subtype,
                                   CMySize& dimension,
                                   WORD& bitsPerPixel,
                                   DOUBLE& framesPerSecond,
                                   BITMAPINFOHEADER& bmiHeader)
{
	//OutputDebugString(L"Utilities::GetVideoFormatInfo1");


    if (!IsEqualCLSID(mType.majortype, MEDIATYPE_Video))
	{
		//OutputDebugString(L"Utilities::GetVideoFormatInfo1a:FALSE");
        return FALSE;
	}
	//OutputDebugString(L"Utilities::GetVideoFormatInfo2");
    //if (mType.pbFormat == 0 || !IsEqualCLSID(mType.formattype, FORMAT_VideoInfo))
	if (mType.pbFormat == 0 || !IsEqualCLSID(mType.formattype, FORMAT_Theora))
	{
		//OutputDebugString(L"Utilities::GetVideoFormatInfo2a:FALSE");
        return FALSE;
	}
	//OutputDebugString(L"Utilities::GetVideoFormatInfo3");
    subtype = mType.subtype;
	//OutputDebugString(L"Utilities::GetVideoFormatInfo4");
    VIDEOINFOHEADER *pVIH = reinterpret_cast<VIDEOINFOHEADER*>(mType.pbFormat);
    if (!pVIH)
	{
		//OutputDebugString(L"Utilities::GetVideoFormatInfo4a:FALSE");
        return FALSE;
	}
	//OutputDebugString(L"Utilities::GetVideoFormatInfo5");
    dimension.cx = pVIH->bmiHeader.biWidth;
    dimension.cy = pVIH->bmiHeader.biHeight;
    bitsPerPixel = pVIH->bmiHeader.biBitCount;
    framesPerSecond = RTU_TO_FPS(pVIH->AvgTimePerFrame);
	//OutputDebugString(L"Utilities::GetVideoFormatInfo6");
    memmove(&bmiHeader, &pVIH->bmiHeader, sizeof(BITMAPINFOHEADER));
	//OutputDebugString(L"Utilities::GetVideoFormatInfo: successful end");
    return TRUE;
}

HRESULT Utilities::MakeVideoMediaType(AM_MEDIA_TYPE** ppresult,
                                      REFGUID subType,
                                      WORD bitsPerPixel,
                                      LONG frameWidth,
                                      LONG frameHeight,
                                      DOUBLE framesPerSecond)
{
    CHECK_POINTER(ppresult)

    if (frameWidth <= 0 || frameHeight <= 0 || framesPerSecond <= 0.0)
        return E_INVALIDARG;

    *ppresult = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppresult == 0)
        return E_OUTOFMEMORY;
    memset(*ppresult, 0, sizeof(AM_MEDIA_TYPE));

    (*ppresult)->majortype = MEDIATYPE_Video;
    (*ppresult)->subtype = subType;
    (*ppresult)->bFixedSizeSamples = TRUE;
    (*ppresult)->bTemporalCompression = FALSE;
    (*ppresult)->formattype = FORMAT_VideoInfo;
    (*ppresult)->pUnk = 0;

    BITMAPINFOHEADER* pBMInfo = 0;
    DWORD headersize = 0;

    HRESULT hr = MakeBITMAPINFOHEADER(subType, bitsPerPixel, frameWidth, frameHeight, &pBMInfo);
    if (FAILED(hr))
        return hr;

    VIDEOINFOHEADER *pVIH = (VIDEOINFOHEADER*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    if (pVIH == 0)
    {
        CoTaskMemFree(*ppresult);
        *ppresult = 0;
        delete pBMInfo;
        return E_OUTOFMEMORY;
    }

    memset(pVIH, 0, sizeof(VIDEOINFOHEADER));
    (*ppresult)->cbFormat = sizeof(VIDEOINFOHEADER);
    (*ppresult)->pbFormat = (BYTE*)pVIH;

    // Copy the information from the generated BITMAPINFO structure
    memcpy(&pVIH->bmiHeader, pBMInfo, sizeof(BITMAPINFOHEADER));
    delete pBMInfo;

    pVIH->rcSource.left = 0;
    pVIH->rcSource.right = 0;
    pVIH->rcSource.top = 0;
    pVIH->rcSource.bottom = 0;

    pVIH->rcTarget.left = 0;
    pVIH->rcTarget.right = 0;
    pVIH->rcTarget.top = 0;
    pVIH->rcTarget.bottom = 0;

    pVIH->AvgTimePerFrame = FPS_TO_RTU(framesPerSecond);
    (*ppresult)->lSampleSize = pVIH->bmiHeader.biSizeImage;
    pVIH->dwBitRate = (DWORD) floor((DOUBLE)(*ppresult)->lSampleSize * framesPerSecond * 8.0); // # bits per second

    return S_OK;
}

HRESULT Utilities::SetVideoFormatInfo(CMediaType &mtype,
                                      REFGUID subtype,
                                      CMySize& dimension,
                                      WORD bitsPerPixel,
                                      DOUBLE framesPerSecond)
{
    AM_MEDIA_TYPE* pMediaType = 0;
    HRESULT hr = Utilities::MakeVideoMediaType(&pMediaType,
                                    subtype,
                                    bitsPerPixel,
                                    dimension.cx,
                                    dimension.cy,
                                    framesPerSecond);
    if (FAILED(hr))
        return hr;

    FreeMediaType(mtype);
    mtype.InitMediaType();
    hr = mtype.Set(*pMediaType);
    DeleteMediaType(pMediaType);
    return hr;
}

void Utilities::Scale_32_Bit_Buffer(LONG out_width, LONG out_height,
			                             DWORD* out_buf,
			                             LONG in_width, LONG in_height,
			                             DWORD* in_buf)
{
	LONG h_dda_const;
	if (out_width >= in_width)
	{
		h_dda_const = static_cast<LONG>((1200.0f / (static_cast<FLOAT>(out_width) / static_cast<FLOAT>(in_width)) + 0.5f));
	}
	else
	{
		h_dda_const = static_cast<LONG>((1200.0f * (static_cast<FLOAT>(out_width) / static_cast<FLOAT>(in_width)) + 0.5f));
	}

	LONG v_dda_const;
	if (out_height >= in_height)
	{
		v_dda_const = static_cast<LONG>((1200.0f / (static_cast<FLOAT>(out_height) / static_cast<FLOAT>(in_height)) + 0.5f));
	}
	else
	{
		v_dda_const = static_cast<LONG>((1200.0f * (static_cast<FLOAT>(out_height) / static_cast<FLOAT>(in_height)) + 0.5f));
	}

	LONG h_acc;
	LONG src_index, dst_index;

	LONG v_acc = 0;
	LONG v_index = 0;

	LONG w_left;
	LONG w_right;
	LONG w_top;
	LONG w_bottom;

    BYTE* ThisPixel;
    BYTE* NextPixel;
    BYTE* PixelBelow;

    DWORD red;
    DWORD green;
    DWORD blue;

	if (out_width >= in_width && out_height >= in_height)
	{
		// Scale up both dimensions

		while (v_index < (in_height - 1))
		{
			w_top = 1200L - static_cast<LONG>(v_acc);
			w_bottom = static_cast<LONG>(v_acc);

			src_index = 0;
			dst_index = 0;
			h_acc = 0;

			while (src_index < (in_width - 1))
			{
				w_left = 1200L - static_cast<LONG>(h_acc);
				w_right = static_cast<LONG>(h_acc);

                ThisPixel = reinterpret_cast<BYTE*>(&in_buf[src_index]);
                NextPixel = reinterpret_cast<BYTE*>(&in_buf[src_index + 1]);
                PixelBelow = reinterpret_cast<BYTE*>(&in_buf[in_width + src_index]);

                red = static_cast<DWORD>((
					w_left * static_cast<LONG>(ThisPixel[0]) +
					w_right * static_cast<LONG>(NextPixel[0]) +
					w_top * static_cast<LONG>(ThisPixel[0]) +
					w_bottom * static_cast<LONG>(PixelBelow[0])
					) >> 10) & 0x000000FF;

                green = static_cast<DWORD>((
					w_left * static_cast<LONG>(ThisPixel[1]) +
					w_right * static_cast<LONG>(NextPixel[1]) +
					w_top * static_cast<LONG>(ThisPixel[1]) +
					w_bottom * static_cast<LONG>(PixelBelow[1])
					) >> 2) & 0x0000FF00;

                blue = static_cast<DWORD>((
					w_left * static_cast<LONG>(ThisPixel[2]) +
					w_right * static_cast<LONG>(NextPixel[2]) +
					w_top * static_cast<LONG>(ThisPixel[2]) +
					w_bottom * static_cast<LONG>(PixelBelow[2])
					) << 6) & 0x00FF0000;

				out_buf[dst_index++] = (blue | green | red);

				h_acc += h_dda_const;
				if (h_acc > 1200)
				{
					src_index++;
					h_acc -= 1200;
				}
			}

			// Get the last pixel in the row
            ThisPixel = reinterpret_cast<BYTE*>(&in_buf[src_index]);
            PixelBelow = reinterpret_cast<BYTE*>(&in_buf[in_width + src_index]);

            red = static_cast<DWORD>((
				w_top * static_cast<LONG>(ThisPixel[0]) +
				w_bottom * static_cast<LONG>(PixelBelow[0])
				) >> 10) & 0x000000FF;

            green = static_cast<DWORD>((
				w_top * static_cast<LONG>(ThisPixel[1]) +
				w_bottom * static_cast<LONG>(PixelBelow[1])
				) >> 2) & 0x0000FF00;

            blue = static_cast<DWORD>((
				w_top * static_cast<LONG>(ThisPixel[2]) +
				w_bottom * static_cast<LONG>(PixelBelow[2])
				) << 6) & 0x00FF0000;

		    out_buf[dst_index] = (blue | green | red);

			out_buf += out_width;

			v_acc += v_dda_const;
			if (v_acc > 1200)
			{
				v_index++;
				in_buf += in_width;
				v_acc -= 1200;
			}
		}

		// Get the last row in the bitmap
		src_index = 0;
		dst_index = 0;
		h_acc = 0;

		while (src_index < (in_width - 1))
		{
			w_left = 1200L - static_cast<LONG>(h_acc);
			w_right = static_cast<LONG>(h_acc);

            ThisPixel = reinterpret_cast<BYTE*>(&in_buf[src_index]);
            NextPixel = reinterpret_cast<BYTE*>(&in_buf[src_index + 1]);

            red = static_cast<DWORD>((
				w_left * static_cast<LONG>(ThisPixel[0]) +
				w_right * static_cast<LONG>(NextPixel[0])
				) >> 10) & 0x000000FF;

            green = static_cast<DWORD>((
				w_left * static_cast<LONG>(ThisPixel[1]) +
				w_right * static_cast<LONG>(NextPixel[1])
				) >> 2) & 0x0000FF00;

            blue = static_cast<DWORD>((
				w_left * static_cast<LONG>(ThisPixel[2]) +
				w_right * static_cast<LONG>(NextPixel[2])
				) << 6) & 0x00FF0000;

			out_buf[dst_index++] = (blue | green | red);

			h_acc += h_dda_const;
			if (h_acc > 1200)
			{
				src_index++;
				h_acc -= 1200;
			}
		}

		// Get the last pixel in the row
		out_buf[dst_index] = in_buf[src_index];

	}
	else if (out_width >= in_width && out_height < in_height)
	{
		// Scale up horizontal, scale down vertical

		while (v_index < in_height)
		{
			src_index = 0;
			dst_index = 0;
			h_acc = 0;

			while (src_index < (in_width - 1))
			{
				w_left = 1200L - static_cast<LONG>(h_acc);
				w_right = static_cast<LONG>(h_acc);

                ThisPixel = reinterpret_cast<BYTE*>(&in_buf[src_index]);
                NextPixel = reinterpret_cast<BYTE*>(&in_buf[src_index + 1]);

                red = static_cast<DWORD>((
				    w_left * static_cast<LONG>(ThisPixel[0]) +
				    w_right * static_cast<LONG>(NextPixel[0])
				    ) >> 10) & 0x000000FF;

                green = static_cast<DWORD>((
				    w_left * static_cast<LONG>(ThisPixel[1]) +
				    w_right * static_cast<LONG>(NextPixel[1])
				    ) >> 2) & 0x0000FF00;

                blue = static_cast<DWORD>((
				    w_left * static_cast<LONG>(ThisPixel[2]) +
				    w_right * static_cast<LONG>(NextPixel[2])
				    ) << 6) & 0x00FF0000;

				out_buf[dst_index++] = (blue | green | red);

				h_acc += h_dda_const;
				if (h_acc > 1200)
				{
					src_index++;
					h_acc -= 1200;
				}
			}

			// Get the last pixel in the row
			out_buf[dst_index] = in_buf[src_index];

			out_buf += out_width;

			while (v_acc < 1200)
			{
				v_index++;
				in_buf += in_width;
				v_acc += v_dda_const;
			}

			v_acc -= 1200;
		}
	}
	else if (out_width < in_width && out_height >= in_height)
	{
		// Scale down horizontal, scale up vertical

		while (v_index < (in_height - 1))
		{
			w_top = 1200L - static_cast<LONG>(v_acc);
			w_bottom = static_cast<LONG>(v_acc);

			src_index = 0;
			dst_index = 0;
			h_acc = 0;

			while(src_index < in_width)
			{
                ThisPixel = reinterpret_cast<BYTE*>(&in_buf[src_index]);
                PixelBelow = reinterpret_cast<BYTE*>(&in_buf[in_width + src_index]);

                red = static_cast<DWORD>((
				    w_top * static_cast<LONG>(ThisPixel[0]) +
				    w_bottom * static_cast<LONG>(PixelBelow[0])
				    ) >> 10) & 0x000000FF;

                green = static_cast<DWORD>((
				    w_top * static_cast<LONG>(ThisPixel[1]) +
				    w_bottom * static_cast<LONG>(PixelBelow[1])
				    ) >> 2) & 0x0000FF00;

                blue = static_cast<DWORD>((
				    w_top * static_cast<LONG>(ThisPixel[2]) +
				    w_bottom * static_cast<LONG>(PixelBelow[2])
				    ) << 6) & 0x00FF0000;

				out_buf[dst_index++] = (blue | green | red);

				while (h_acc < 1200)
				{
					src_index++;
					h_acc += h_dda_const;
				}

				h_acc -= 1200;
			}

			out_buf += out_width;

			v_acc += v_dda_const;
			if (v_acc > 1200)
			{
				v_index++;
				in_buf += in_width;
				v_acc -= 1200;
			}
		}

		// Get the last row in the bitmap
		src_index = 0;
		dst_index = 0;
		h_acc = 0;

		while(src_index < in_width)
		{
			out_buf[dst_index++] = in_buf[src_index];
			while (h_acc < 1200)
			{
				src_index++;
				h_acc += h_dda_const;
			}

			h_acc -= 1200;
		}

	}
	else if (out_width < in_width && out_height < in_height)
	{
		// Scale down both dimensions

		while (v_index < in_height)
		{
			src_index = 0;
			dst_index = 0;
			h_acc = 0;

			while(src_index < in_width)
			{
				DWORD test = in_buf[src_index];
				out_buf[dst_index++] = in_buf[src_index];
				while (h_acc < 1200)
				{
					src_index++;
					h_acc += h_dda_const;
				}

				h_acc -= 1200;
			}

			out_buf += out_width;

			while (v_acc < 1200)
			{
				v_index++;
				in_buf += in_width;
				v_acc += v_dda_const;
			}

			v_acc -= 1200;
		}
	}
}

HRESULT Utilities::GetPixelFormat(int bpp, PixelFormat* pixFmt)
{
	switch(bpp)
	{
		case 15:
			*pixFmt = PixelFormat16bppRGB555;
			return S_OK;

		case 16:
			*pixFmt = PixelFormat16bppRGB565;
			return S_OK;

		case 24:
			*pixFmt = PixelFormat24bppRGB;
			return S_OK;

		case 32:
			*pixFmt = PixelFormat32bppRGB;
			return S_OK;

		default:
			return E_FAIL;
	}
}

bool Utilities::GetRegValueFromPath(TCHAR* path, TCHAR* valueName, TCHAR* value, int valsize)
{
	HKEY key;
	if (RegOpenKey(HKEY_CURRENT_USER, path, &key) == ERROR_SUCCESS)
	{
		DWORD value_length = valsize-1;
		DWORD type = REG_SZ;
		RegQueryValueEx(key, valueName, NULL, &type, (LPBYTE)value, &value_length);
		value[value_length-1] = 0;
		RegCloseKey(key);
		return true;
	}
	RegCloseKey(key);
	return false;
}

bool Utilities::GetRegValueFromPath(TCHAR* path, TCHAR* valueName, DWORD* value)
{
	HKEY key;
	if (RegOpenKey(HKEY_CURRENT_USER, path, &key) == ERROR_SUCCESS)
	{
		DWORD value_length = sizeof ( DWORD);
		DWORD type = REG_DWORD;
		RegQueryValueEx(key, valueName, NULL, &type, (unsigned char *)value, &value_length);
		value[value_length-1] = 0;
		RegCloseKey(key);
		return true;
	}
	RegCloseKey(key);
	return false;
}


WCHAR * Utilities::C2WC(char* p)
{
	WCHAR *pwcsName;
	// required size
	int nChars = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
	pwcsName = new WCHAR[nChars];
	MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR)pwcsName, nChars);
	return pwcsName;
}

char * Utilities::WC2C(WCHAR* p)
{
	//WCHAR *pwcsName;
	//// required size
	//int nChars = MultiByteToWideChar(CP_ACP, 0, p, -1, NULL, 0);
	//pwcsName = new WCHAR[nChars];
	//MultiByteToWideChar(CP_ACP, 0, p, -1, (LPWSTR)pwcsName, nChars);
	//return pwcsName;
	char* pcName;
	int nChars = WideCharToMultiByte(CP_ACP, 0, p, -1, NULL, 0, NULL, NULL);
	pcName = new char[nChars];
	WideCharToMultiByte(CP_ACP, 0, p, -1, (LPSTR)pcName, nChars, NULL, NULL);
	return pcName;
}

const std::string Utilities::CurrentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}


///////////////////////////////
//end Utilities functions
///////////////////////////////
