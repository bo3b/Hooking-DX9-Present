#include "NativePlugin.h"


// This is a bit weird.  By setting the CINTERFACE before including d3d9.h, we get access
// to the C style interface, which includes direct access to the vTable for the objects.
// That makes it possible to just reference the lpVtbl->CreateDevice, instead of having
// magic constants, and multiple casts to fetch the address of the CreateDevice routine.
//
// This can only be included where it's used to fetch those routine addresses, because
// it will make other C++ units fail to compile.  

#define CINTERFACE
#include <d3d9.h>
#undef CINTERFACE


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

//-----------------------------------------------------------

BOOL APIENTRY DllMain(__in HMODULE hModule, __in DWORD ulReasonForCall, __in LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			//HANDLE hPipe = ::CreateNamedPipe(L"\\\\.\\pipe\\HyperPipe32",
			//	PIPE_ACCESS_DUPLEX,
			//	PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
			//	PIPE_UNLIMITED_INSTANCES,
			//	4096,
			//	4096,
			//	0,
			//	NULL);

			//ConnectNamedPipe(hPipe, NULL);


			//IDirect3D9* g_pD3D = NULL;
			//g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);

			//IDirect3DDevice9* g_pDevice9;
			//D3DPRESENT_PARAMETERS presParams = {};
			//LPVOID addrCreateDevice = nullptr;
			//LPVOID addrPresent = nullptr;
			//presParams.BackBufferWidth = 1600;
			//presParams.BackBufferHeight = 1200;
			//presParams.BackBufferFormat = D3DFMT_UNKNOWN;
			//presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
			//presParams.Windowed = true;
			//HRESULT hres = g_pD3D->lpVtbl->CreateDevice(g_pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, 
			//	GetActiveWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &presParams, &g_pDevice9);
			//if (SUCCEEDED(hres))
			//{
			//	addrCreateDevice = g_pD3D->lpVtbl->CreateDevice;
			//	addrPresent= g_pDevice9->lpVtbl->Present;
			//}

			//DWORD bytesWritten = 0;
			//WriteFile(hPipe, &addrPresent, sizeof(LPVOID), &bytesWritten, NULL);

			//CreateThread(NULL, NULL, MyThread, NULL, NULL, NULL);

			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

LPVOID __stdcall GetPresentAddr(LPVOID pD3D)
{
	IDirect3D9* pD = (IDirect3D9*)pD3D;
	IDirect3DDevice9* g_pDevice9;
	D3DPRESENT_PARAMETERS presParams = {};
	LPVOID addrCreateDevice = nullptr;
	LPVOID addrPresent = nullptr;
	presParams.BackBufferWidth = 1600;
	presParams.BackBufferHeight = 1200;
	presParams.BackBufferFormat = D3DFMT_UNKNOWN;
	presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presParams.Windowed = true;
	HRESULT hres = pD->lpVtbl->CreateDevice(pD, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		GetActiveWindow(), D3DCREATE_HARDWARE_VERTEXPROCESSING, &presParams,
		&g_pDevice9);
	if (SUCCEEDED(hres))
		return g_pDevice9->lpVtbl->Present;
	else
		return nullptr;
}

LPVOID __stdcall GetCreateAddr(LPVOID pD3D)
{
	IDirect3D9* pD = (IDirect3D9*)pD3D;
	return pD->lpVtbl->CreateDevice;
}
