# Hooking-DX9-Present

A simple console app to demonstrate how to use Deviare in C#, and connect to a native
C++ plugin, which is running as a CustomDLL in a target app.  

Deviare does not provide easy access to all DirectX calls because the only call supported
in the Deviare DB for DX9 is CreateDevice, so it's easier to directly use those APIs in 
C++ in a plugin.  This also allows the plugin to handle more complex tasks, without 
impacting performance like it would if it were done in the C# host side.

<br>

This sample is intended to demonstrate several things that are useful to DX9
or graphics programmers. This is only setup for DX9 and x86, but could be extended for either x64 or DX11.

 1) Setting up a mixed environment of C# and Native C++
 2) Using a Nektra CustomDLL for hooking.
 3) Mixed use of In-Proc hooking, and Deviare hooking.
 4) An easily copied technique to hook any vtable based DX9 calls.
 5) Deviare style hook for a normal DLL export function.
 6) How to use Nektra CallCustomAPI.

<br>

To test, download the repo and build.  It was built with VS2013, but should work with 
any version.  All required pieces are included here, including DLL binaries for Deviare.

Launch any DX9 x86 game by passing the game path on the command line.


<br>

Written by Bo3b Johnson, August 2017.  
 This code is MIT license, free for any use.
 Deviare and Nektra In-Proc are used here via their GPL 3 license.

This is loosely based upon the Nektra Invisible-Walls sample, but simplified.