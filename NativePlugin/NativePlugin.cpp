// NativePlugin.cpp : Defines the exported functions for the DLL.
//  Including the special command API to change modes.

#include "NativePlugin.h"

// We need the Deviare interface though, to be able to provide the OnLoad,
// OnFunctionCalled interfaces, to be able to LoadCustomDLL this DLL.
#import "DeviareCOM.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename


// We don't include this in NativePlugin.h, because we treat the d3d9.h differently
// when we are fetching the routine address, versus just using the interface.

#include <d3d9.h>



// --------------------------------------------------------------------------------------------------
//IMPORTANT NOTES:
//---------------
//
//1) Regardless of the functionallity of the plugin, the dll must export: OnLoad, OnUnload, OnHookAdded,
//   OnHookRemoved and OnFunctionCall (Tip: add a .def file to avoid name mangling)
//
//2) Code inside methods should try/catch exceptions to avoid possible crashes in hooked application.
//
//3) Methods that returns an HRESULT value should return S_OK if success or an error value.
//
//   3.1) If a method returns a value less than zero, all hooks will be removed and agent will unload
//        from the process.
//
//   3.2) The recommended way to handle errors is to let the SpyMgr to decide what to do. For e.g. if
//        you hit an error in OnFunctionCall, probably, some custom parameter will not be added to the
//        CustomParams() collection. So, when in your app received the DNktSpyMgrEvents::OnFunctionCall
//        event, you will find the parameters is missing and at this point you can choose what to do.

using namespace Deviare2;

extern "C" HRESULT WINAPI OnLoad()
{
//	::OutputDebugStringA("NativePlugin::OnLoad called\n");

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
//	::OutputDebugString(L"NativePlugin::OnFunctionCall called for ");
	BSTR name;
	lpHookInfo->get_FunctionName(&name);
//	::OutputDebugString(name);
//	::OutputDebugString(L"\n");

	HRESULT hres;
	INktParamsEnum* paramsEnum;
	long paramCount;
	long pointeraddress;

	INktParam* nktResult;
	lpHookCallInfoPlugin->Result(&nktResult);
	nktResult->get_PointerVal((long*)(&g_pD3D));

	// Now send that address of the CreateDevice call back to C#
	LPVOID addrCreate = GetCreateAddr(g_pD3D);

	//hPipe = ::CreateNamedPipe(L"\\\\.\\pipe\\HyperPipe32",
	//	PIPE_ACCESS_DUPLEX,
	//	PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
	//	PIPE_UNLIMITED_INSTANCES,
	//	4096,
	//	4096,
	//	0,
	//	NULL);

	//ConnectNamedPipe(hPipe, NULL);

	//DWORD bytesWritten = 0;
	//WriteFile(hPipe, &addrCreate, sizeof(LPVOID), &bytesWritten, NULL);


	// If we have a valid window here, we can drive down through the DX9 vtables to 
	// find the address of the Present call.
	//LPVOID pPresent = nullptr;
	//HWND window = GetActiveWindow();
	//if (window != nullptr)
	//{
	//	pPresent = GetPresentAddr(g_pD3D);
	//	DWORD bytesWritten = 0;
	//	WriteFile(hPipe, &pPresent, sizeof(LPVOID), &bytesWritten, NULL);
	//}


	return S_OK;
}
