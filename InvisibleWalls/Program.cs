// A simple console app to demonstrate how to use Deviare in C#, to connect to a native
// C++ plugin, which is running as a CustomDLL in a target app.  
//
// This sample is intended to demonstrate several things that are useful to DX9
// or graphics programmers:
//
//  1) Setting up a mixed environment of C# and Native C++
//  2) Using a Nektra CustomDLL for hooking.
//  3) Mixed use of In-Proc hooking, and Deviare hooking.
//  4) A technique to easily hook vtable based DX9 calls.
//  5) Deviare style hook for a normal DLL export function.
//  6) How to use Nektra CallCustomAPI.
//  7) Easily copied approach for hooking any DX9 function.
//
//
// This is only setup for DX9 and x86, but could be extended for either x64 or DX11.
// To test, use it to launch any DX9 x86 game.
//
// Deviare does not provide easy access to all DirectX calls because the only call supported
// in the Deviare DB for DX9 is CreateDevice, so it's easier to directly use those APIs in 
// C++ in a plugin.  This also allows the plugin to handle more complex tasks, without 
// impacting performance like it would if it were done in the C# host side.
//
//
// Written by Bo3b Johnson, August 2017.  
//  This code is MIT license, free for any use.
//  Deviare and Nektra In-Proc are used here via their GPL 3 license.
//
// This is loosely based on the Invisible-Walls sample, but simplified.

using System;
using System.IO;

using Nektra.Deviare2;


namespace InvisibleWalls
{
    class Program
    {
        static NktSpyMgr _spyMgr;
        static NktProcess _gameProcess;

        static void Main(string[] args)
        {
            int hresult;
            object continueevent;
            string nativeDLLName = Environment.CurrentDirectory + @"\NativePlugin.dll";
            string game = @"G:\Games\The Ball\Binaries\Win32\theball.exe";

            // Default game can be overridden by any game path at command line.
            if (args.Length != 0)
                game = args[0];


            // Setup and Init the primary Deviare interface of the SpyMgr.  This is
            // declared static because we only ever want one.  
            // In order to successfully Init, we must have the DeviareCOM.dll,
            // deviare32.db, DvAgent.dll, and Nektra.Deviare2.dll in the app folder.

            Console.WriteLine("Initialize Deviare engine...");
            _spyMgr = new NktSpyMgr();
            hresult = _spyMgr.Initialize();
            if (hresult != 0)
                throw new Exception("Deviare initialization error.");

            // We must set the game directory specifically, otherwise it winds up being the 
            // C# app directory which can make the game crash.  This must be done before CreateProcess.
            // This also changes the working directory, which will break Deviare's ability to find
            // the NativePlugin, so we'll use full path descriptions for the DLL load.

            Directory.SetCurrentDirectory(Path.GetDirectoryName(game));

            // Launch the game, but suspended, so we can hook our first call and be certain to catch it.

            Console.WriteLine("Launching: " + game + "...");
            _gameProcess = _spyMgr.CreateProcess(game, true, out continueevent);
            if (_gameProcess == null)
                throw new Exception("Game launch failed.");

            // Load the NativePlugin for the C++ side.  The NativePlugin must be in this app folder.
            // The Agent supports the use of Deviare in the CustomDLL, but does not respond to hooks.

            Console.WriteLine("Load NativePlugin... " + nativeDLLName);
            _spyMgr.LoadAgent(_gameProcess);
            int result = _spyMgr.LoadCustomDll(_gameProcess, nativeDLLName, true, true);
            if (result < 0)
                throw new Exception("Could not load NativePlugin DLL.");

            // Hook the primary DX9 creation call of Direct3DCreate9, which is a direct export of 
            // the d3d9 DLL.  All DX9 games must call this interface.
            // We set this to flgOnlyPostCall, because we want to use the IDirect3D9 object it returns.

            Console.WriteLine("Hook the D3D9.DLL!Direct3DCreate9...");
            NktHook d3dHook = _spyMgr.CreateHook("D3D9.DLL!Direct3DCreate9", (int)eNktHookFlags.flgOnlyPostCall);
            if (d3dHook == null)
                throw new Exception("Failed to hook D3D9.DLL!Direct3DCreate9");

            // Make sure the CustomHandler in the NativePlugin at OnFunctionCall gets called when this 
            // object is created. At that point, the native code will take over.

            d3dHook.AddCustomHandler(nativeDLLName, 0, "");

            // Finally attach and activate the hook in the still suspended game process.

            d3dHook.Attach(_gameProcess, true);
            d3dHook.Hook(true);


            // Ready to go.  Let the game startup.  When it calls Direct3DCreate9, we'll be
            // called in the NativePlugin::OnFunctionCall

            Console.WriteLine("Continue game launch...");
            _spyMgr.ResumeProcess(_gameProcess, continueevent);


            // Go into the wait loop for exiting this app.  If this exits before the game is quit,
            // it will cleanly remove all the hooks and leave the game running.
            
            bool done = false;
            while (!done)
            {
                // This will block the app thread until a key is pressed.
                // This will only be active when this app is in the foreground.  
                ConsoleKey key = Console.ReadKey().Key;

                if (key == ConsoleKey.Insert)
                {
                    _spyMgr.CallCustomApi(_gameProcess, nativeDLLName, "ShowWireFrame", null, true);
                    Console.WriteLine("Wireframe mode on...");
                }
                if (key == ConsoleKey.Delete)
                {
                    _spyMgr.CallCustomApi(_gameProcess, nativeDLLName, "ShowWalls", null, true);
                    Console.WriteLine("Solid mode on...");
                }

                if (key == ConsoleKey.Escape)
                    done = true;
            }
        }
    }
}

