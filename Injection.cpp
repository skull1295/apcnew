#include "Injection.h"

#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <vector>
#include <conio.h> 
#include <psapi.h> 
#include <DbgHelp.h>
#include <map>

#include "Scheduler.hpp"



#pragma comment(lib, "Dbghelp.lib")

DWORD64 GetModuleBaseAddress(DWORD processID, const char* moduleName) {
	DWORD64 moduleBaseAddress = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processID);
	if (snapshot != INVALID_HANDLE_VALUE) {
		MODULEENTRY32 moduleEntry;
		moduleEntry.dwSize = sizeof(MODULEENTRY32);
		if (Module32First(snapshot, &moduleEntry)) {
			do {
				if (!_stricmp(moduleEntry.szModule, moduleName)) {
					moduleBaseAddress = (DWORD64)moduleEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(snapshot, &moduleEntry));
		}
		CloseHandle(snapshot);
	}
	return moduleBaseAddress;
}


void CustomPageGuardHandler() {

}

void InjectionICHook(ICStack* regs) {
	char printbuf[500] = {};
	static HANDLE iclogfile = NULL;
	static DWORD iclogfilecur = 0;
	if (iclogfile == NULL) {
		iclogfile = CreateFileW(L"ICLog.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
		SetFilePointer(iclogfile, 0, NULL, FILE_BEGIN);

	}



	//unhooking cool functions
	static void* KiUserCallbackDispatcher = NULL;
	static void* KiUserApcDispatcher = NULL;
	if (KiUserCallbackDispatcher == NULL) { KiUserCallbackDispatcher = GetProcAddress(NTDLL, "KiUserCallbackDispatcher"); }
	if (KiUserApcDispatcher == NULL) { KiUserApcDispatcher = GetProcAddress(NTDLL, "KiUserApcDispatcher"); }
	if (regs->r10 == (DWORD64)KiUserCallbackDispatcher) {
		regs->rcx = *(DWORD64*)(regs->rsp + 0x20);
		regs->returnaddr = regs->r10 + 5;
	}
	if (regs->r10 == (DWORD64)KiUserApcDispatcher) {
		regs->rcx = *(DWORD64*)(regs->rsp + 0x18);
		regs->returnaddr = regs->r10 + 5;
	}

	//hooking exceptions
	static void* KiUserExceptionDispatcher = NULL;
	static void (*RtlRestoreContext)(PCONTEXT ContextRecord, _EXCEPTION_RECORD * ExceptionRecord) = NULL;
	if (KiUserExceptionDispatcher == NULL) { KiUserExceptionDispatcher = GetProcAddress(NTDLL, "KiUserExceptionDispatcher"); }
	if (RtlRestoreContext == NULL) { RtlRestoreContext = (void (*)(PCONTEXT ContextRecord, _EXCEPTION_RECORD * ExceptionRecord))GetProcAddress(NTDLL, "RtlRestoreContext"); }
	if (regs->r10 == (DWORD64)KiUserExceptionDispatcher) {
		PEXCEPTION_RECORD ExceptionRecord = (PEXCEPTION_RECORD)(regs->rsp + 0x4f0);
		PCONTEXT Context = (PCONTEXT)regs->rsp;


		if ((ExceptionRecord->ExceptionCode == EXCEPTION_GUARD_PAGE)) {
			MEMORY_BASIC_INFORMATION binfo = {};
			NtQueryVirtualMemoryInline((HANDLE)0xffffffffffffffff, (void*)Context->Rip, MemoryBasicInformation, &binfo, sizeof(MEMORY_BASIC_INFORMATION), NULL);
			NtProtectVirtualMemoryInline((HANDLE)0xffffffffffffffff, &binfo.BaseAddress, &binfo.RegionSize, PAGE_EXECUTE_READ, NULL);
			RtlRestoreContext(Context, ExceptionRecord);
		}

	}


	//memory hider
	if (((regs->r10 > (ULONG_PTR)injection_vars.byfron.lpBaseOfDll) && (regs->r10 < (ULONG_PTR)injection_vars.byfron.lpBaseOfDll + injection_vars.byfron.SizeOfImage))) {
		MEMORY_BASIC_INFORMATION checkinfo = {};
		DWORD stat = NtQueryVirtualMemoryInline((HANDLE)0xffffffffffffffff, (void*)regs->rsp, MemoryBasicInformation, &checkinfo, sizeof(MEMORY_BASIC_INFORMATION), NULL);
		if ((checkinfo.Type != 0) && (stat == 0)) {
			ULONG_PTR cur = regs->rsp;
			ULONG_PTR end = (ULONG_PTR)checkinfo.BaseAddress + checkinfo.RegionSize - 0x100;
			while (cur < end) {
				PMEMORY_BASIC_INFORMATION checkbi = (PMEMORY_BASIC_INFORMATION)cur;
				DWORD checkcur = 0;
				while (checkcur < injection_vars.InjectionHiddenMemoryC) {
					PMEMORY_BASIC_INFORMATION compare = &injection_vars.InjectionHiddenMemory[checkcur];
					if ((checkbi->BaseAddress == compare->BaseAddress) && (checkbi->Protect == compare->Protect) && (checkbi->AllocationProtect == compare->AllocationProtect)) {
						checkbi->Protect = 1;
						checkbi->AllocationProtect = 1;
						checkbi->BaseAddress = (PVOID)0; //who wrote this byfron's rwx scanner ?
					}
					checkcur++;
				}
				cur++;
			}
		}
	}


	/*static bool test0 = 0;
	if (test0 == 0 && GetCurrentTID() == injection_vars.RobloxProcessInfo->ThreadInfos[0].Client_Id.UniqueThread) {
		test0 = 1;
	}*/
	regs->returnaddr = (DWORD64)injection_vars.ICAddr;
	return;
}
















int InjectionDllCaller() {


	HMODULE roblox = GetModuleHandleA("RobloxPlayerBeta.exe");
	void (*print)(int, const char*) = (void (*)(int, const char*))((BYTE*)roblox + 0x15977E0);



	
	
		print(1, "[ INFO ] Injected!");
		
	
	



	return 0;
	
}





typedef int (*InjectionDllCallerFunc)();


























bool InjectionFindByfron(HANDLE process, PInternalVars s) {
	MODULEINFO byfron = FindModuleByNameInProcess(process, L"RobloxPlayerBeta.dll");
	if (byfron.lpBaseOfDll != 0) {
		s->byfron = byfron;
		return 1;
	}
	return 0;
}



bool InjectionBeforeAllocSelf(HANDLE process, PVOID mem, PInternalVars s) {
	ICHookerLowLevelPartSetHooker(ConvertAddrByBase(&InjectionICHook, thismoduleinfo.lpBaseOfDll, mem));
	PVOID ic = (PVOID)PFindData(ByfronICPattern, 8, (ULONG_PTR)s->byfron.lpBaseOfDll, s->byfron.SizeOfImage, process, 100000);
	if (ic == (PVOID)-1) { return 0; }
	s->ICAddr = ic;
	return 1;
}
bool InjectionAllocSelf(HANDLE process, PInternalVars s, MODULEINFO thismodinfo) {

	PVOID selfmem = VirtualAllocEx(process, NULL, thismodinfo.SizeOfImage, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (InjectionBeforeAllocSelf(process, selfmem, s) == 0) { goto err; }
	if (selfmem == NULL) { return 0; }
	if (VirtualQueryEx(process, selfmem, &s->InjectionHiddenMemory[s->InjectionHiddenMemoryC], sizeof(MEMORY_BASIC_INFORMATION)) == 0) { goto err; };
	s->InjectionHiddenMemoryC++;
	if (WriteProcessMemory(process, selfmem, thismodinfo.lpBaseOfDll, thismodinfo.SizeOfImage, NULL) == 0) { goto err; };
	s->Injector = selfmem;
	return 1;

err:
	VirtualFreeEx(process, selfmem, 0, MEM_RELEASE);
	return 0;


}
bool InjectionAllocDll(HANDLE process, HANDLE dllfile, PInternalVars s) {
	SIZE_T dllsize = GetFileSize(dllfile, NULL);
	PVOID dllmem = VirtualAllocEx(process, NULL, dllsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	if (dllmem == NULL) { return 0; }

	void* buf = malloc(dllsize);
	SetFilePointer(dllfile, 0, NULL, FILE_BEGIN);
	if (ReadFile(dllfile, buf, dllsize, NULL, NULL) == 0) { return 0; };




	s->dllparams.BaseAddr = dllmem;

	PIMAGE_DOS_HEADER ImageDosHeader = (PIMAGE_DOS_HEADER)buf;
	PIMAGE_NT_HEADERS64 ImageNtHeaders = (PIMAGE_NT_HEADERS64)((unsigned char*)ImageDosHeader + ImageDosHeader->e_lfanew);
	PIMAGE_OPTIONAL_HEADER ImageOptionalHeader = (PIMAGE_OPTIONAL_HEADER)&ImageNtHeaders->OptionalHeader;


	s->dllparams.EntryPointOffset = ImageOptionalHeader->AddressOfEntryPoint - 0x1000 + 0x400;
	/*
	IMAGE_IMPORT_DESCRIPTOR* ImageImportDescr = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)ImageOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)buf);
	IMAGE_IMPORT_DESCRIPTOR* ImageImportDescrCur = ImageImportDescr;
	while (ImageImportDescrCur) {
		char* module = (char*)((BYTE*)buf+(ULONG_PTR)(ImageImportDescrCur->Name));
		std::cerr << module << "\n";

		skip:
		ImageImportDescrCur++;
	}
*/
//while (1) {}

	if (WriteProcessMemory(process, dllmem, buf, dllsize, NULL) == 0) { return 0; };

	free(buf);


	if (VirtualQueryEx(process, dllmem, &s->InjectionHiddenMemory[s->InjectionHiddenMemoryC], sizeof(MEMORY_BASIC_INFORMATION)) == 0) { return 0; };
	s->InjectionHiddenMemoryC++;
	return 1;

}
bool InjectionSetupInternalPart(HANDLE process, PInternalVars s) {
	//return CreateTPDirectThread(process, ConvertAddrByBase(&InjectionICHookSetuper,thismoduleinfo.lpBaseOfDll,s->Injector));
	return CreateAPCDirectThread(process, ConvertAddrByBase(&InjectionDllCaller, thismoduleinfo.lpBaseOfDll, s->Injector));

}
void InjectionICHookSetuper(HANDLE process, PInternalVars s) {
	PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION inscinfowb = {};
	memset(&inscinfowb, 0, sizeof(PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION));
	inscinfowb.Callback = ConvertAddrByBase(&ICHookerLowLevelPart, thismoduleinfo.lpBaseOfDll, s->Injector);
	while (1) {

		CallSyscall(0x001c, process, ProcessInstrumentationCallback, &inscinfowb, sizeof(PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION));
	}
}


bool GetThisModuleInfo() {
	HMODULE thismod = GetModuleHandleA(NULL);
	if (thismod == NULL) { return 0; }
	return GetModuleInformation(GetCurrentProcess(), thismod, &thismoduleinfo, sizeof(MODULEINFO));
}
ULONG_PTR ConvertAddrByBase(ULONG_PTR Addr, ULONG_PTR OldBase, ULONG_PTR NewBase) {
	return Addr - OldBase + NewBase;
}
void* ConvertAddrByBase(const void* Addr, const void* OldBase, const void* NewBase) {
	return (void*)((ULONG_PTR)Addr - (ULONG_PTR)OldBase + (ULONG_PTR)NewBase);
}



InternalVars injection_vars = {};
MODULEINFO thismoduleinfo = {};