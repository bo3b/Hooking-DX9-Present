// NativePlugin.cpp : Defines the exported functions for the DLL.
//  Including the special command API to change modes.

#include "NativePlugin.h"

// We need the Deviare interface though, to be able to provide the OnLoad,
// OnFunctionCalled interfaces, to be able to LoadCustomDLL this DLL.
#import "DeviareCOM.dll" raw_interfaces_only, named_guids, raw_dispinterfaces, auto_rename



// --------------------------------------------------------------------------------------------------
//IMPORTANT NOTES:
//---------------
//
//1) Regardless of the functionality of the plugin, the dll must export: OnLoad, OnUnload, OnHookAdded,
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
	::OutputDebugStringA("NativePlugin::OnLoad called\n");

	// This is running inside the game itself, so make sure we can use
	// COM here.
	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

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
// is called by the game.  We can then fetch the returned IDirect3D9 object, and
// use that to hook the next level.
// 
// We can use the Deviare side to hook this function, because Direct3DCreate9 is
// a direct export from the d3d9 DLL, and is also directly supported in the 
// Deviare DB.

// Original API:
//	IDirect3D9* Direct3DCreate9(
//		UINT SDKVersion
//	);


extern "C" HRESULT WINAPI OnFunctionCall(__in INktHookInfo *lpHookInfo, __in DWORD dwChainIndex,
	__in INktHookCallInfoPlugin *lpHookCallInfoPlugin)
{
	BSTR name;
	INktParam* nktResult;
	IDirect3D9* pDX9 = nullptr;

	lpHookInfo->get_FunctionName(&name);

	::OutputDebugString(L"NativePlugin::OnFunctionCall called for ");
	::OutputDebugString(name);
	::OutputDebugString(L"\n");

	// We only expect this to be called for D3D9.DLL!Direct3DCreate9. We want to daisy chain
	// through the call sequence to ultimately get the Present routine.
	//
	// The result of the Direct3DCreate9 function is the IDirect3D9 object, which you can think
	// of as DX9 itself. 

	lpHookCallInfoPlugin->Result(&nktResult);
	nktResult->get_PointerVal((long*)(&pDX9));

	// At this point, we are going to switch from using Deviare style calls
	// to In-Proc style calls, because the routines we need to hook are not
	// found in the Deviare database.  It would be possible to add them 
	// and then rebuilding it, but In-Proc works alongside Deviare so this
	// approach is simpler.

	HookCreateDevice(pDX9);

	return S_OK;
}
