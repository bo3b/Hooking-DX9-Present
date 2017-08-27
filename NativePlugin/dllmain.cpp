#include "NativePlugin.h"

// For this part of the DLL, it makes more sense to use Nektra In-Proc
// instead of Nektra Deviare.  The interface for DX9 is not well supported
// by the DB for Deviare, so getting access to the DX9 APIs is simpler
// using In-Proc.
#include "NktHookLib.h"



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



//-----------------------------------------------------------
// This is automatically instantiated by C++, so the hooking library
// is immediately available.
CNktHookLib nktInProc;


//-----------------------------------------------------------
// API chunks to allow us to hook the IDirect3DDevice9::Present

// This serves a dual purpose of defining the interface routine as required by
// DX9, and also is the storage for the original call, returned by nktInProc.Hook.

SIZE_T hook_id_Present;
STDMETHOD_(HRESULT, pOrigPresent)(IDirect3DDevice9* This,
	/* [in] */ const RECT    *pSourceRect,
	/* [in] */ const RECT    *pDestRect,
	/* [in] */       HWND    hDestWindowOverride,
	/* [in] */ const RGNDATA *pDirtyRegion
	) = nullptr;


STDMETHODIMP_(HRESULT) Hooked_Present(IDirect3DDevice9* This,
	CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
//	::OutputDebugStringA("Hooked_Present called\n");	// Called too often to log

	HRESULT hr = pOrigPresent(This, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

	return hr;
}

//-----------------------------------------------------------
// API chunks to allow us to hook the IDirect3D9::CreateDevice

// This serves a dual purpose of defining the interface routine as required by
// DX9, and also is the storage for the original call, returned by nktInProc.Hook.

SIZE_T hook_id_CreateDevice;
STDMETHOD_(HRESULT, pOrigCreateDevice)(IDirect3D9* This,
	/* [in] */          UINT                  Adapter,
	/* [in] */          D3DDEVTYPE            DeviceType,
	/* [in] */          HWND                  hFocusWindow,
	/* [in] */          DWORD                 BehaviorFlags,
	/* [in, out] */     D3DPRESENT_PARAMETERS *pPresentationParameters,
	/* [out, retval] */ IDirect3DDevice9      **ppReturnedDeviceInterface
	) = nullptr;


STDMETHODIMP_(HRESULT) Hooked_CreateDevice(IDirect3D9* This,
	UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface)
{
	::OutputDebugStringA("Hooked_CreateDevice called\n");

	HRESULT hr = pOrigCreateDevice(This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters,
		ppReturnedDeviceInterface);

	if (pOrigPresent == nullptr && SUCCEEDED(hr) && ppReturnedDeviceInterface != nullptr)
	{
		IDirect3DDevice9* game_Device = *ppReturnedDeviceInterface;

		DWORD dwOsErr = nktInProc.Hook(&hook_id_Present, (void**)&pOrigPresent,
			game_Device->lpVtbl->Present, Hooked_Present, 0);
		if (FAILED(dwOsErr))
			::OutputDebugStringA("Failed to hook IDirect3DDevice9::Present\n");
	}

	return hr;
}

//-----------------------------------------------------------
// API chunks to allow us to hook the Direct3DCreate9
//

SIZE_T hook_id_Direct3DCreate9;
STDMETHOD_(IDirect3D9*, pOrigDirect3DCreate9)(
	UINT SDKVersion
	) = nullptr;


STDMETHODIMP_(IDirect3D9*) Hooked_Direct3DCreate9(UINT SDKVersion)
{
	::OutputDebugStringA("Hooked_Direct3DCreate9 called\n");

	// Call original routine, and save the returned Direct3D9.  We only
	// want to keep the latest one, because the game might make several
	// as it tests system capabilities.
	IDirect3D9* game_Direct3D9;
	game_Direct3D9 = pOrigDirect3DCreate9(SDKVersion);

	// If we are here, we want to now hook the IDirect3D9::CreateDevice
	// routine, as that will be the next thing the game does, and we
	// need access to the Direct3DDevice9.
	// This can't be done directly, because this is a vtable based API
	// call, not an export from a DLL, so we need to directly hook the 
	// address of the CreateDevice function. Since we are using the 
	// CINTERFACE, we can just directly access it.

	if (pOrigCreateDevice == nullptr && game_Direct3D9 != nullptr)
	{
		DWORD dwOsErr = nktInProc.Hook(&hook_id_CreateDevice, (void**)&pOrigCreateDevice,
			game_Direct3D9->lpVtbl->CreateDevice, Hooked_CreateDevice, 0);
		if (FAILED(dwOsErr))
			::OutputDebugStringA("Failed to hook IDirect3D9::CreateDevice\n");
	}

	return game_Direct3D9;
}


//-----------------------------------------------------------

// At DLL launch, we are loaded by the Deviare C# master, and one of the
// very first pieces of code to be called in the game process.
// Here we want to start the daisy chain of hooking DX9 interfaces, to
// ultimately get access to IDirect3DDevice9::Present
//
// The sequence a game will use is:
//  IDirect3D9* Direct3DCreate9;
//  IDirect3D9::CreateDevice(return ppIDirect3DDevice9);
//  ppIDirect3DDevice9->Present

BOOL APIENTRY DllMain(__in HMODULE hModule, __in DWORD ulReasonForCall, __in LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			HINSTANCE hDX9;
			DWORD dwOsErr;
			void* fnOrigDirect3DCreate9;

			nktInProc.SetEnableDebugOutput(TRUE);
			::CoInitializeEx(NULL, COINIT_MULTITHREADED);

			// Start with the Direct3DCreate9 call.  This is a direct export from the
			// d3d9.dll, so we don't need the vtable to hook this.
			//
			//  IDirect3D9* Direct3DCreate9(UINT SDKVersion);

			hDX9 = NktHookLibHelpers::GetModuleBaseAddress(L"d3d9.dll");
			if (!hDX9)
				goto err;

			fnOrigDirect3DCreate9 = NktHookLibHelpers::GetProcedureAddress(hDX9, "Direct3DCreate9");
			if (fnOrigDirect3DCreate9 == NULL)
				goto err;

			dwOsErr = nktInProc.Hook(&hook_id_Direct3DCreate9, (void**)&pOrigDirect3DCreate9,
				fnOrigDirect3DCreate9, Hooked_Direct3DCreate9, 0);
			if (FAILED(dwOsErr))
				goto err;

			TCHAR name[255];
			DWORD ret;
			ret = ::GetCurrentDirectory(255, name);
			::OutputDebugStringA("Working directory: ");
			::OutputDebugStringW(name); 
			::OutputDebugStringA("\n");

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
err:
	::OutputDebugStringA("Failed creating Hook for Direct3DCreate9.\n");
	return FALSE;
}


// This routine will grab the address of the  IDirect3DDevice9::Present
// function.  Present is the primary routine for hooking games, because
// it is called every frame to Present what the game has drawn. 
//
// This routine is in this compilation unit, so that we can use the
// CINTERFACE trick to easily get the address.
//
// This should only be called when the game has setup it's window, so
// that we can do the sequence to create an IDirect3DDevice9 object.
// This object will be released, we just need to fetch the Present
// address, and it will be the same address for any caller in the 
// game process.
//
// The sequence a game will use is:
//  IDirect3D9* D3D9::Direct3DCreate9();
//  IDirect3D9::CreateDevice(return ppIDirect3DDevice9);
//  ppIDirect3DDevice9->Present


LPVOID __stdcall GetPresentAddr(HWND window, IDirect3D9* pD3D)
{
	IDirect3DDevice9* g_pDevice9;
	D3DPRESENT_PARAMETERS presParams = {};
	LPVOID addrPresent = nullptr;

	LPVOID addrCreateDevice = nullptr;
	LPVOID addrPresent = nullptr;
	presParams.BackBufferWidth = 1024;
	presParams.BackBufferHeight = 768;
	presParams.BackBufferFormat = D3DFMT_UNKNOWN;
	presParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presParams.Windowed = true;
	HRESULT hres = pD3D->lpVtbl->CreateDevice(pD3D, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
		window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presParams,
		&g_pDevice9);
	if (SUCCEEDED(hres))
		addrPresent = g_pDevice9->lpVtbl->Present;

	g_pDevice9->lpVtbl->Release(g_pDevice9);

	return addrPresent;
}

