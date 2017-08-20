// A simple console app to demonstrate how to use Deviare in C#, to connect to a native
// C++ plugin, running in a target app.  
//
// Deviare does not provide easy access to all DirectX calls, so it's easier to directly
// use those APIs in C++ in a plugin.  This also allows the plugin to handle more complex
// tasks, without impacting performance like it would if it were done in the C# host side.
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
        static NktProcess _process;

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


            Console.WriteLine("Launch game...");
            _process = _spyMgr.CreateProcess(@"G:\Games\The Ball\Binaries\Win32\TheBall.exe", true, out continueevent);
            if (_process == null)
                throw new Exception("Game launch failed.");


            Console.WriteLine("Load native plugin...");
            int result = _spyMgr.LoadCustomDll(_process, @".\NativePlugin.dll", false, false);
            if (result < 0)         // This returns result=1/S_FALSE, which I think means that the Agent was loaded.
                throw new Exception("Could not load NativePlugin DLL.");
                

            // Connect via IPC using a named pipe, and fetch the address of the CreateDevice routine from 
            // the native plugin.  That address is fetched from the running game, during the DLLMain of the plugin.
            // ToDo: What happens in 64 bit?
            Console.WriteLine("Fetch address from Pipe...");
            byte[] byteAddress = new byte[4];
            pipe = new NamedPipeClientStream(".", "HyperPipe32", PipeDirection.InOut);
            pipe.Connect();
            pipe.Read(byteAddress, 0, 4);

            Console.WriteLine("Hook the d3d9.dll!CreateDevice...");
            Int32 addrCreateDevice = BitConverter.ToInt32(byteAddress, 0);
            NktHook hook = _spyMgr.CreateHookForAddress((IntPtr)addrCreateDevice, "D3D9.DLL!CreateDevice", 
                (int)(eNktHookFlags.flgRestrictAutoHookToSameExecutable | eNktHookFlags.flgOnlyPostCall | eNktHookFlags.flgDontCheckAddress));
            hook.AddCustomHandler(@".\NativePlugin.dll", 0, "");

            hook.Attach(_process, true);
            hook.Hook(true);

            Console.WriteLine("Continue game launch...");
            _spyMgr.ResumeProcess(_process, continueevent);

            while (true)
            {
                
            }
        }

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

