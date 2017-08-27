// A simple console app to demonstrate how to use Deviare in C#, to connect to a native
// C++ plugin, running in a target app.  
//
// This is only setup for DX9 and x86, but could be extended for either x64 or DX11.
// To test, use it to launch any DX9 x86 game.
//
// Deviare does not provide easy access to all DirectX calls because the only call supported
// in the Deviare DB for DX9 is CreateDevice, so it's easier to directly use those APIs in 
// C++ in a plugin.  This also allows the plugin to handle more complex tasks, without 
// impacting performance like it would if it were done in the C# host side.
//
// This is loosely based on the Invisible-Walls sample, but simplified.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.IO;

using System.IO.Pipes;

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
            string game;

            if (args.Length == 0)
                game = @"G:\Games\The Ball\Binaries\Win32\theball.exe";
            else
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

            // For any hook that is called using this SpyMgr, let us know here.
            // This is not nec

            _spyMgr.OnFunctionCalled += new DNktSpyMgrEvents_OnFunctionCalledEventHandler(OnFunctionCalled);

            // We must set the game directory specifically, otherwise it winds up being the 
            // C# app directory which can make the game crash.  This must be done before CreateProcess.

            Directory.SetCurrentDirectory(Path.GetDirectoryName(game));


            // Launch the game, but suspended, so we can hook our first call and be certain to catch it.

            Console.WriteLine("Launching: " + game + "...");
            _gameProcess = _spyMgr.CreateProcess(game, true, out continueevent);
            if (_gameProcess == null)
                throw new Exception("Game launch failed.");

            // Load the NativePlugin for the C++ side.  The NativePlugin must be in this app folder.

            Console.WriteLine("Load NativePlugin...");
            int result = _spyMgr.LoadCustomDll(_gameProcess, @".\NativePlugin.dll", true, true);
            if (result < 0)
                throw new Exception("Could not load NativePlugin DLL.");

            // Hook the primary DX9 creation call of Direct3DCreate9, which is a direct export of 
            // the d3d9 DLL.  All DX9 games must call this interface.
            // We set this to flgOnlyPostCall, because we want to use the IDirect3D9 object it returns.

            Console.WriteLine("Hook the D3D9.DLL!Direct3DCreate9...");
            NktHook d3dHook = _spyMgr.CreateHook("D3D9.DLL!Direct3DCreate9",
                (int)(eNktHookFlags.flgRestrictAutoHookToSameExecutable | 
                eNktHookFlags.flgOnlyPostCall | 
                eNktHookFlags.flgDontCheckAddress));
            if (d3dHook == null)
                throw new Exception("Failed to hook D3D9.DLL!Direct3DCreate9");

            // Make sure the CustomHandler in the NativePlugin at OnFunctionCall gets called when this 
            // object is created. At that point, the native code will take over.

            d3dHook.AddCustomHandler(@".\NativePlugin.dll", (int)eNktHookCustomHandlerFlags.flgChDontCallIfLoaderLocked, "");

            // Finally attach and activate the hook in the game process.

            d3dHook.Attach(_gameProcess, true);
            d3dHook.Hook(true);


            // Ready to go.  Let the game startup.  When it calls Direct3DCreate9, we'll be
            // called in the NativePlugin::OnFunctionCall

            Console.WriteLine("Continue game launch...");
            _spyMgr.ResumeProcess(_gameProcess, continueevent);


            while (true)
            {
                System.Threading.Thread.Sleep(1000);
            }
        }

        // This OnFunctionCalled is not necessary for the sample, but demonstrates how the hook can 
        // simultaneously call here in C#, and in C++ in the NativePlugin.
        static void OnFunctionCalled(NktHook hook, NktProcess process, NktHookCallInfo hookCallInfo)
        {

            string strOnFunctionCalled = hook.FunctionName + "\n";

            if (hook.FunctionName.CompareTo("D3D9.DLL!CreateDevice") == 0)
            {
                INktParamsEnum paramsEnum = hookCallInfo.Params();

                INktParam param = paramsEnum.First();

                INktParam tempParam = null;

                while (param != null)
                {
                    tempParam = param;

                    param = paramsEnum.Next();
                }

                strOnFunctionCalled += " " + tempParam.PointerVal.ToString() + "\n";

            }

            Console.WriteLine(strOnFunctionCalled);
        }
    }
}

