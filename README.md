# Hooking-DX9-Present

A simple console app to demonstrate how to use Deviare in C#, and connect to a native
C++ plugin, which is running as a CustomDLL in a target app.  

Deviare does not provide easy access to all DirectX calls because the only call supported
in the Deviare DB for DX9 is CreateDevice, so it's easier to directly use those APIs in 
C++ in a plugin.  This also allows the plugin to handle more complex tasks, without 
impacting performance like it would if it were done in the C# host side.

The goal of the sample is just to demonstrate and clarify how to use Nektra's awesome
Deviare and In-Proc hooking libraries.  The DX9 Present call is often desirable as a
spot to hook, especially for overlays.  This demonstrates how to hook that call.

The actual functionality here will change the drawing mode from Solid to WireFrame.  It's
not particularly useful or interesting, but shows how to hook DX9 SetRenderState and 
override a parameter.

<br>

This sample is intended to demonstrate several things that are useful to DX9
or graphics programmers. This is only setup for DX9 and x86, but could be extended for either x64 or DX11.

 1) Setting up a mixed environment of C# and Native C++
 2) Using a Nektra CustomDLL for hooking.
 3) Mixed use of In-Proc hooking, and Deviare hooking.
 4) An easily copied technique to hook any vtable based DX9 calls.
 5) Deviare style hook for a normal DLL export function.
 6) How to use Nektra CallCustomAPI.
 7) How to step through DX9 objects, to create a Present hook.

<br>

To test:
1. Download the repo and build.  It was built with VS2013, but should work with 
any version.  
1. All required pieces are included here, including DLL binaries for Deviare in the Dependencies folder.
1. Register the DeviareCOM.DLL with the OS.  
  1. Navigate to the Dependencies folder with an elevated cmd line window.
  1. Execute *regsvr32 DeviareCOM.DLL*
1. Launch any DX9 x86 game by passing a full game path on the command line.
1. Use Insert key to enable WireFrame, Delete key to disable.

<br>

Written by Bo3b Johnson, August 2017.  
 This code is MIT license, free for any use.
 Deviare and Nektra In-Proc are used here via their GPL 3 license.

This is loosely based upon the Nektra Invisible-Walls sample, but simplified.
