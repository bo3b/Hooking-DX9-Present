// NativePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "NativePlugin.h"

// We don't include this in NativePlugin.h, because we treat the d3d9.h differently
// when we are fetching the routine address, versus just using the interface.

#include <d3d9.h>


#if defined _M_IX86
#import "DeviareCOM.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename
#elif defined _M_X64
#import "DeviareCOM64.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename
#else
#error Unsupported platform
#endif


// The actual IDirect3DDevice9 that the game is using.
IDirect3DDevice9* pDeviceInterface;

using namespace Deviare2;

extern "C" HRESULT WINAPI OnLoad()
{
	DebugBreak();
	::OutputDebugStringA("NativePlugin::OnLoad called");
	return S_OK;
}


extern "C" VOID WINAPI OnUnload()
{
	::OutputDebugStringA("NativePlugin::OnUnLoad called");
	return;
}


extern "C" HRESULT WINAPI OnHookAdded(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex,
	__in LPCWSTR szParametersW)
{
	::OutputDebugStringA("NativePlugin::OnHookAdded called");
	return S_OK;
}


extern "C" VOID WINAPI OnHookRemoved(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex)
{
	::OutputDebugStringA("NativePlugin::OnHookRemoved called");
	return;
}


// This is the primary call we are interested in.  It will be called after CreateDevice
// is called by the game.  We can then fetch the returned IDirect3DDevice9 object, which
// will be stored in pDeviceInterface.

extern "C" HRESULT WINAPI OnFunctionCall(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex,
	__in INktHookCallInfoPlugin *lpHookCallInfoPlugin)
{
	::OutputDebugStringA("NativePlugin::OnFunctionCall called");

	HRESULT hres;
	INktParamsEnum* paramsEnum;
	long paramCount;
	long pointeraddress;

	hres = lpHookCallInfoPlugin->Params(&paramsEnum);
	if (FAILED(hres))
		return S_OK;

	hres = paramsEnum->get_Count(&paramCount);
	if (FAILED(hres))
		return S_OK;

	INktParam* param = nullptr;
	for (int i = 0; i < paramCount; i++)
	{
		paramsEnum->GetAt(i, &param);
	}

	param->get_PointerVal(&pointeraddress);

	IDirect3DDevice9** ppDeviceInterface = (IDirect3DDevice9**)pointeraddress;

	pDeviceInterface = *ppDeviceInterface;

	return S_OK;
}
