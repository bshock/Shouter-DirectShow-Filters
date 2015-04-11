#pragma once

#include <WTypes.h>
#include <strmif.h>
#include <MMSystem.h>
#include <mtype.h>
#include <streams.h>
#include <combase.h>
#include "gdiplus.h"
#include <string>

using namespace Gdiplus;


#define CHECK_POINTER(x) { if (!x) return E_POINTER; }
#define PIXEL_BIT_SIZE 32
#define MIN_HORIZ_FRAME_SIZE 32
#define MIN_VERT_FRAME_SIZE 32
#define MIN_FRAME_SIZE_GRANULARITY 4
#define PIXEL_BYTE_SIZE 4
#define DEFAULT_FPS 30.0
#define CHECK_POINTER(x) { if (!x) return E_POINTER; }

//replacement for CSize, which requires messy libraries I don't want to add for VS 2010 Debug builds
struct CMySize
{
	int cx;
	int cy;
	CMySize():cx(0), cy(0) {}
	CMySize(int x, int y):cx(x), cy(y) {}
	void SetSize(int x, int y) { cx = x; cy = x;}
};

template <class T>
class CMyComPtr
{
public:
    typedef T _PtrClass;
    CMyComPtr() {p=NULL;}
    CMyComPtr(T* lp)
    {
        if ((p = lp) != NULL)
            p->AddRef();
    }
    CMyComPtr(const CMyComPtr<T>& lp)
    {
        if ((p = lp.p) != NULL)
            p->AddRef();
    }
    ~CMyComPtr() {if (p) p->Release();}
    void Release() {if (p) p->Release(); p=NULL;}
    operator T*() {return (T*)p;}
    T& operator*() {ASSERT(p!=NULL); return *p; }
    //The assert on operator& usually indicates a bug.  If this is really
    //what is needed, however, take the address of the p member explicitly.
    T** operator&() { ASSERT(p==NULL); return &p; }

	
	T* operator->() { ASSERT(p!=NULL); return p; }
    T* operator=(T* lp){return (T*)MyAtlComPtrAssign((IUnknown**)&p, lp);}
    T* operator=(const CMyComPtr<T>& lp)
    {
        return (T*)MyAtlComPtrAssign((IUnknown**)&p, lp.p);
    }
#if _MSC_VER>1020
    bool operator!(){return (p == NULL);}
#else
    BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
#endif
    T* p;
};

class Utilities
{
static DOUBLE k100NS_UNITS;

public:
// # of 100ns units per second (to calc time per frame in 100ns units)

static HRESULT MakeBITMAPINFOHEADER(REFGUID subType,
                                        WORD bitsPerPixel,
                                        LONG frameWidth,
                                        LONG frameHeight,
                                        BITMAPINFOHEADER** ppresult);

static DOUBLE RTU_TO_FPS(REFERENCE_TIME units);

static REFERENCE_TIME FPS_TO_RTU(DOUBLE fps);

static BOOL GetVideoFormatInfo(const AM_MEDIA_TYPE& mType,
                                   GUID& subtype,
                                   CMySize& dimension,
                                   WORD& bitsPerPixel,
                                   DOUBLE& framesPerSecond,
                                   BITMAPINFOHEADER& bmiHeader);

static HRESULT MakeVideoMediaType(AM_MEDIA_TYPE** ppresult,
                                      REFGUID subType,
                                      WORD bitsPerPixel,
                                      LONG frameWidth,
                                      LONG frameHeight,
                                      DOUBLE framesPerSecond);

static HRESULT SetVideoFormatInfo(CMediaType &mtype,
                                      REFGUID subtype,
                                      CMySize& dimension,
                                      WORD bitsPerPixel,
                                      DOUBLE framesPerSecond);

static void Scale_32_Bit_Buffer(LONG out_width, LONG out_height,
			                             DWORD* out_buf,
			                             LONG in_width, LONG in_height,
			                             DWORD* in_buf);

static HRESULT GetPixelFormat(int bpp, PixelFormat* pixFmt);

static bool GetRegValueFromPath(TCHAR* path, TCHAR* valueName, TCHAR* value, int valsize);
static bool GetRegValueFromPath(TCHAR* path, TCHAR* valueName, DWORD* value);
static WCHAR * Utilities::C2WC(char* p);
static char * Utilities::WC2C(WCHAR* p);
static const std::string Utilities::CurrentDateTime();
};
