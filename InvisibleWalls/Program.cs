// A simple console app to demonstrate how to use Deviare in C#, to connect to a native
// C++ plugin, running in a target app.  
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
            NamedPipeClientStream pipe;

            Console.WriteLine("Initialize Deviare engine...");

            // Setup and Init the primary Deviare interface of the SpyMgr.  This is
            // declared static because we only ever want one.
            _spyMgr = new NktSpyMgr();
            hresult = _spyMgr.Initialize();
            if (hresult != 0)
                throw new Exception("Deviare initialization error.");

            // For any hook that is called using this SpyMgr, let us know.
            _spyMgr.OnFunctionCalled += new DNktSpyMgrEvents_OnFunctionCalledEventHandler(OnFunctionCalled);


            // Launch the game, but suspended, so we can hook our first call and be certain to catch it.
            Console.WriteLine("Launch game...");
            _gameProcess = _spyMgr.CreateProcess(@"G:\Games\The Ball\Binaries\Win32\TheBall.exe", true, out continueevent);
            if (_gameProcess == null)
                throw new Exception("Game launch failed.");

            Console.WriteLine("Load native plugin...");
            int result = _spyMgr.LoadCustomDll(_gameProcess, @".\NativePlugin.dll", false, false);
            if (result < 0)         // This returns result=1/S_FALSE, which I think means that the Agent was loaded.
                throw new Exception("Could not load NativePlugin DLL.");


            // Hook the primary DX9 creation call of Direct3DCreate9, which is a direct export of the DLL
            Console.WriteLine("Hook the D3D9.DLL!Direct3DCreate9...");
            NktHook d3dHook = _spyMgr.CreateHook("D3D9.DLL!Direct3DCreate9",
                (int)(eNktHookFlags.flgRestrictAutoHookToSameExecutable | eNktHookFlags.flgOnlyPostCall | eNktHookFlags.flgDontCheckAddress));
            if (d3dHook == null)
                throw new Exception("Failed to hook D3D9.DLL!Direct3DCreate9");

            // Make sure the CustomHandler in the NativePlugin gets called when this object is created.
            // At that point, the native code will take over.
            d3dHook.AddCustomHandler(@".\NativePlugin.dll", (int)eNktHookCustomHandlerFlags.flgChDontCallIfLoaderLocked, "");

            d3dHook.Attach(_gameProcess, true);
            d3dHook.Hook(true);


            Console.WriteLine("Continue game launch...");
            _spyMgr.ResumeProcess(_gameProcess, continueevent);

            // Connect via IPC using a named pipe, and fetch the address of the CreateDevice routine from 
            // the native plugin.  That address is fetched from the running game, during the DLLMain of the plugin.
            // This needs to be done after resuming the app, otherwise the pipe is blocked.
            // ToDo: What happens in 64 bit?
            Console.WriteLine("Fetch address from Pipe...");
            byte[] byteAddress = new byte[4];
            pipe = new NamedPipeClientStream(".", "HyperPipe32", PipeDirection.InOut);
            pipe.Connect();
            pipe.Read(byteAddress, 0, 4);

            Console.WriteLine("Hook the d3d9.dll!CreateDevice...");
            Int32 addrCreate = BitConverter.ToInt32(byteAddress, 0);
            if (addrCreate != 0)
            {
                NktHook hook = _spyMgr.CreateHookForAddress((IntPtr)addrCreate, "D3D9.DLL!CreateDevice",
                    (int)(eNktHookFlags.flgRestrictAutoHookToSameExecutable | eNktHookFlags.flgOnlyPostCall | eNktHookFlags.flgDontCheckAddress));
                hook.AddCustomHandler(@".\NativePlugin.dll", 0, "");

                hook.Attach(_gameProcess, true);
                hook.Hook(true);
            }

            Console.WriteLine("Fetch address of Present from Pipe...");
            pipe.Read(byteAddress, 0, 4);
            Int32 addrPresent = BitConverter.ToInt32(byteAddress, 0);


            while (true)
            {
                
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

