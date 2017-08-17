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


            Console.WriteLine("Connect native plugin...");
            int result = _spyMgr.LoadCustomDll(_process, @".\CRegistryPlugin.dll", false, false);


            pipe = new NamedPipeClientStream(".", "HyperPipe32", PipeDirection.InOut);
            pipe.Connect();

            byte[] data = new byte[4];
            pipe.Read(data, 0, 4);
            Int32 address = BitConverter.ToInt32(data, 0);


            Console.WriteLine("Hook the d3d9.dll!CreatDevice...");
            NktHook hook = _spyMgr.CreateHookForAddress((IntPtr)address, "D3D9.DLL!CreateDevice", (int)(eNktHookFlags.flgRestrictAutoHookToSameExecutable | eNktHookFlags.flgOnlyPostCall | eNktHookFlags.flgDontCheckAddress));
            // hook.AddCustomHandler(@"..\..\..\Plugin\bin\Plugins\CRegistryPlugin.dll", 0, "");
            hook.AddCustomHandler(@".\CRegistryPlugin.dll", 0, "");

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

