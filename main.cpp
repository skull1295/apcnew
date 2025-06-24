#include "main.h"

int wmain(int argc, wchar_t* argv[]) {

    std::cerr << "[0x0001] Initializing NTDLL function table...\n";
    SetupNTDLL();
    if (NTDLL == NULL) {
        std::cerr << "[0x0002][FAIL] NTDLL initialization failed. Code: 0xE0010001\n";
        exit(1);
    }
    std::cerr << "[0x0003][OK] NTDLL function table initialized at 0x" << std::hex << (uintptr_t)NTDLL << "\n";

    std::cerr << "[0x0004] Retrieving injector module info...\n";
    if (GetThisModuleInfo() == 0) {
        std::cerr << "[0x0005][FAIL] Could not get self module base. Code: 0xE0020002\n";
        exit(1);
    }
    std::cerr << "[0x0006][OK] Injector base @ 0x" << std::hex << (uintptr_t)thismoduleinfo.lpBaseOfDll
        << ", size: 0x" << thismoduleinfo.SizeOfImage << "\n";

    std::cerr << "[0x0007] Scanning for Roblox process (RobloxPlayerBeta.exe)...\n";
    SYSTEM_PROCESS_INFORMATION* RobloxProcessInformation = FindProcessByModuleName(L"RobloxPlayerBeta.exe");
    if (RobloxProcessInformation && RobloxProcessInformation->ProcessId != 0) {
        std::cerr << "[0x0008][OK] Roblox PID: 0x" << std::hex << RobloxProcessInformation->ProcessId << "\n";

        std::cerr << "[0x0009] Attempting handle acquisition for PID 0x" << std::hex << RobloxProcessInformation->ProcessId << "...\n";
        HANDLE RobloxProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, RobloxProcessInformation->ProcessId);
        if (RobloxProcess == INVALID_HANDLE_VALUE) {
            std::cerr << "[0x000A][FAIL] OpenProcess failed. Code: 0xE0030003\n";
            exit(1);
        }
        std::cerr << "[0x000B][OK] Process handle: 0x" << std::hex << (uintptr_t)RobloxProcess << "\n";
        injection_vars.RobloxProcessInfo = RobloxProcessInformation;

        std::cerr << "[0x000C] Loading 'Surge.dll' for staged injection...\n";
        HANDLE dllfile = CreateFile("Surge.dll", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (dllfile == INVALID_HANDLE_VALUE) {
            std::cerr << "[0x000D][FAIL] Surge.dll not found or access denied. Code: 0xE0040004\n";
            exit(1);
        }
        std::cerr << "[0x000E][OK] DLL handle: 0x" << std::hex << (uintptr_t)dllfile << "\n";

        std::cerr << "[0x000F] Scanning remote memory for Byfron module signature...\n";
        if (InjectionFindByfron(RobloxProcess, &injection_vars) == 0) {
            std::cerr << "[0x0010][FAIL] Byfron module not located in remote process. Code: 0xE0050005\n";
            exit(1);
        }
        std::cerr << "[0x0011][OK] Byfron base: 0x" << std::hex << (uintptr_t)injection_vars.byfron.lpBaseOfDll
            << ", size: 0x" << injection_vars.byfron.SizeOfImage << "\n";

        std::cerr << "[0x0012] Enumerating and suspending threads inside Byfron region...\n";
        for (DWORD t = 0; t < RobloxProcessInformation->ThreadCount; t++) {
            HANDLE thread = OpenThread(THREAD_ALL_ACCESS, 0, RobloxProcessInformation->ThreadInfos[t].Client_Id.UniqueThread);
            ULONG_PTR saddr = 0;
            DWORD stat = (DWORD)CallSyscall(0x0025, thread, ThreadQuerySetWin32StartAddress, &saddr, sizeof(ULONG_PTR), NULL);
            if (stat == 0) {
                if (saddr >= (ULONG_PTR)injection_vars.byfron.lpBaseOfDll &&
                    saddr < (ULONG_PTR)injection_vars.byfron.lpBaseOfDll + injection_vars.byfron.SizeOfImage) {
                    SuspendThread(thread);
                    std::cerr << "[0x0013][PAUSE] Suspended TID: 0x" << std::hex << RobloxProcessInformation->ThreadInfos[t].Client_Id.UniqueThread
                        << " | StartAddress: 0x" << saddr << "\n";
                }
            }
            CloseHandle(thread);
        }

        std::cerr << "[0x0014] Mapping Surge.dll into remote process memory...\n";
        if (InjectionAllocDll(RobloxProcess, dllfile, &injection_vars) == 0) {
            std::cerr << "[0x0015][FAIL] DLL injection failure. Code: 0xE0060006\n";
            exit(1);
        }
        std::cerr << "[0x0016][OK] DLL injection completed.\n";

        std::cerr << "[0x0017] Injecting current module into remote memory space...\n";
        if (InjectionAllocSelf(RobloxProcess, &injection_vars, thismoduleinfo) == 0) {
            std::cerr << "[0x0018][FAIL] Self injection failed. Code: 0xE0070007\n";
            exit(1);
        }
        std::cerr << "[0x0019][OK] Self module mapped and executing.\n";

        std::cerr << "[0x001A] Executing internal bootstrapper setup in remote context...\n";
        if (InjectionSetupInternalPart(RobloxProcess, &injection_vars) == 0) {
            std::cerr << "[0x001B][FAIL] Remote internal setup failed. Code: 0xE0080008\n";
            exit(1);
        }
        std::cerr << "[0x001C][OK] Internal logic bootstrap initialized.\n";

        std::cerr << "[0x001D] Finalizing: setting inline hooks...\n";
        InjectionICHookSetuper(RobloxProcess, &injection_vars);
        std::cerr << "[0x001E][OK] Hook setup complete. Injection lifecycle complete.\n";

    }
    else {
        std::cerr << "[0xFFFF][ABORT] RobloxPlayerBeta.exe not found. Code: 0xE0090009\n";
        exit(1);
    }
}
