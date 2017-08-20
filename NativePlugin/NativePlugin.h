#pragma once

//-----------------------------------------------------------

// Exclude rarely-used stuff from Windows headers, and use a header
// set that will be workable upon our base target OS of Win7.

#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#define _WIN32_WINDOWS 0x0410
#define _WIN32_IE 0x0700
#define WIN32_LEAN_AND_MEAN

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <SDKDDKVer.h>

#include <windows.h>

#include <stdlib.h>


//#include <objbase.h>




//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
//#include <atlbase.h>
//#include <atlstr.h>
//
//#define MY_EXPORT extern "C" __declspec(dllexport)

//-----------------------------------------------------------

