// NativePlugin.cpp : Defines the exported functions for the DLL application.
//

#include "NativePlugin.h"

// We don't include this in NativePlugin.h, because we treat the d3d9.h differently
// when we are fetching the routine address, versus just using the interface.

#if defined _M_IX86
#import "DeviareCOM.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename
#elif defined _M_X64
#import "DeviareCOM64.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename
#else
#error Unsupported platform
#endif

#include <d3d9.h>



// The actual IDirect3DDevice9 that the game is using.
IDirect3DDevice9* pDeviceInterface;

CNktHookLib spy_in_proc;


using namespace Deviare2;

extern "C" HRESULT WINAPI OnLoad()
{
	::OutputDebugStringA("NativePlugin::OnLoad called\n");

	// Start the daisy chain of API calls when this code is loaded by the C# master.
	// We don't use DLLMain, because it's very restricted on what you can do.

	return S_OK;
}


extern "C" VOID WINAPI OnUnload()
{
	::OutputDebugStringA("NativePlugin::OnUnLoad called\n");
	return;
}


extern "C" HRESULT WINAPI OnHookAdded(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex,
	__in LPCWSTR szParametersW)
{
	::OutputDebugStringA("NativePlugin::OnHookAdded called\n");
	return S_OK;
}


extern "C" VOID WINAPI OnHookRemoved(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex)
{
	::OutputDebugStringA("NativePlugin::OnHookRemoved called\n");
	return;
}


// This is the primary call we are interested in.  It will be called after CreateDevice
// is called by the game.  We can then fetch the returned IDirect3DDevice9 object, which
// will be stored in pDeviceInterface.

// IDirect3D9* Direct3DCreate9(
//	UINT SDKVersion
// );

IDirect3D9* g_pD3D = nullptr;
HANDLE hPipe = nullptr;

extern "C" HRESULT WINAPI OnFunctionCall(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex,
	__in INktHookCallInfoPlugin *lpHookCallInfoPlugin)
{
	::OutputDebugString(L"NativePlugin::OnFunctionCall called for ");
	BSTR name;
	lpHookInfo->get_FunctionName(&name);
	::OutputDebugString(name);
	::OutputDebugString(L"\n");

	HRESULT hres;
	INktParamsEnum* paramsEnum;
	long paramCount;
	long pointeraddress;

	INktParam* nktResult;
	lpHookCallInfoPlugin->Result(&nktResult);
	nktResult->get_PointerVal((long*)(&g_pD3D));

	// Now send that address of the CreateDevice call back to C#
	LPVOID addrCreate = GetCreateAddr(g_pD3D);

	hPipe = ::CreateNamedPipe(L"\\\\.\\pipe\\HyperPipe32",
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
		PIPE_UNLIMITED_INSTANCES,
		4096,
		4096,
		0,
		NULL);

	ConnectNamedPipe(hPipe, NULL);

	DWORD bytesWritten = 0;
	WriteFile(hPipe, &addrCreate, sizeof(LPVOID), &bytesWritten, NULL);


	// If we have a valid window here, we can drive down through the DX9 vtables to 
	// find the address of the Present call.
	LPVOID pPresent = nullptr;
	HWND window = GetActiveWindow();
	if (window != nullptr)
	{
		pPresent = GetPresentAddr(g_pD3D);
		DWORD bytesWritten = 0;
		WriteFile(hPipe, &pPresent, sizeof(LPVOID), &bytesWritten, NULL);
	}



	//hres = lpHookCallInfoPlugin->Params(&paramsEnum);
	//if (FAILED(hres))
	//	return S_OK;

	//hres = paramsEnum->get_Count(&paramCount);
	//if (FAILED(hres))
	//	return S_OK;

	//INktParam* param = nullptr;
	//for (int i = 0; i < paramCount; i++)
	//{
	//	paramsEnum->GetAt(i, &param);  /// leaves it at the last one.
	//}

	//if (param != nullptr)
	//{
	//	param->get_PointerVal(&pointeraddress);

	//	IDirect3DDevice9** ppDeviceInterface = (IDirect3DDevice9**)pointeraddress;

	//	pDeviceInterface = *ppDeviceInterface;
	//}

	return S_OK;
}
