#include "Tools.h"
PVOID AOBScan(PVOID start, SIZE_T size, const int* data1, SIZE_T datasize) {

	SIZE_T skip = 0;
	while (data1[skip] == -1 & skip < datasize) { skip++; }
	int* data = (int*)data1 + skip;

	PVOID cur = start;
	SIZE_T s = 0;
	SIZE_T es = size - datasize;
	while (s < es) {
		SIZE_T datacur = 0;
		while (((data[datacur] == *((BYTE*)cur + datacur)) | data[datacur] == -1) & datacur < datasize) { datacur++; }
		if (datacur == datasize) { return cur; }
		s++;
		cur = (PVOID)((BYTE*)cur + 1);
	}
	return (PVOID)-1;
}
bool is_string(char* str) {
	DWORD l = strlen(str);
	if (l < 3) { return 0; }
	DWORD sl = strlen(basic_chars);
	DWORD cur = 0;
	while (cur < l) {

		DWORD subcur = 0;
		while (subcur < sl) {
			if (str[cur] == basic_chars[subcur]) {
				goto strskip;
			}
			subcur++;
		}
		return 0;
	strskip:
		cur++;
	}
	return 1;
}

int hex2int(char hex) {
	if (hex >= '0' & hex <= '9') {
		return hex - '0';
	}
	if (hex >= 'a' & hex <= 'f') {
		return hex - 'a' + 0xa;
	}
	if (hex >= 'A' & hex <= 'F') {
		return hex - 'F' + 0xa;
	}
}
char int2hexB(int i) {
	if (i >= 0 & i <= 9) {
		return '0' + i;
	}
	if (i >= 10 & i <= 15) {
		return 'A' + i - 10;
	}
}
char int2hexS(int i) {
	if (i >= 0 & i <= 9) {
		return '0' + i;
	}
	if (i >= 10 & i <= 15) {
		return 'a' + i - 10;
	}
}
void hex2bin(char* hex, BYTE* bin, SIZE_T s) {
	SIZE_T c = 0;
	while (c < s) {
		bin[c] = hex2int(hex[c * 2]) << 4;
		bin[c] |= hex2int(hex[(c * 2) + 1]);
		c++;
	}
}
void bin2hex(BYTE* bin, char* hex, SIZE_T s) {
	SIZE_T c = 0;
	while (c < s) {
		hex[c * 2] = int2hexB((bin[c] >> 4) & 0xf);
		hex[(c * 2) + 1] = int2hexB(bin[c] & 0xf);
		c++;
	}
}

void hex2DWORD64(char* hex, DWORD64* d64) {
	BYTE* bin = (BYTE*)d64;
	SIZE_T c = 0;
	while (c < 8) {
		bin[7 - c] = hex2int(hex[c * 2]) << 4;
		bin[7 - c] |= hex2int(hex[(c * 2) + 1]);
		c++;
	}
}
void DWORD642hex(DWORD64* d64, char* hex) {
	BYTE* bin = (BYTE*)d64;
	SIZE_T c = 0;
	while (c < 8) {
		hex[c * 2] = int2hexB((bin[7 - c] >> 4) & 0xf);
		hex[(c * 2) + 1] = int2hexB(bin[7 - c] & 0xf);
		c++;
	}
}


SYSTEM_PROCESS_INFORMATION* FindProcessByModuleName(const wchar_t* modname) {
	SYSTEM_PROCESS_INFORMATION* Processes = (SYSTEM_PROCESS_INFORMATION*)malloc(0x400000);
	SYSTEM_PROCESS_INFORMATION* ret = NULL;
	SYSTEM_PROCESS_INFORMATION* ProcessCur;
	goto s;
end:
	return ret;
s:

	if ((DWORD)NtF("NtQuerySystemInformation")(SystemProcessInformation, Processes, 0x400000, NULL) != 0) { goto end; };

	ProcessCur = Processes;
	while (ProcessCur->NextOffset) {
		if (ProcessCur->ImageName.Buffer != 0) {
			if (wcscmp(ProcessCur->ImageName.Buffer, modname) == 0) {
				ret = ProcessCur;
				goto end;
			}
		}
		ProcessCur = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)ProcessCur + ProcessCur->NextOffset);
	}
	goto end;
}
MODULEINFO FindModuleByNameInProcess(HANDLE process, const wchar_t* modname) {
	wchar_t cmpbuf[256];
	void* cur = 0;
	void* bcur = (void*)-1;
	MEMORY_BASIC_INFORMATION bi;
	while (cur != bcur) {
		VirtualQueryEx(process, cur, &bi, sizeof(MEMORY_BASIC_INFORMATION));
		GetModuleBaseNameW(process, (HMODULE)cur, cmpbuf, 256);
		if (wcscmp(cmpbuf, modname) == 0) {
			MODULEINFO info;
			GetModuleInformation(process, (HMODULE)cur, &info, sizeof(MODULEINFO));
			return info;
		}
		bcur = cur;
		cur = (void*)((ULONG_PTR)cur + (ULONG_PTR)bi.RegionSize);
	}

	MODULEINFO  info;
	info.lpBaseOfDll = 0;
	info.SizeOfImage = 0;
	info.EntryPoint = 0;
	return info;
}
ULONG_PTR boffset(SIZE_T size, ULONG_PTR start, ULONG_PTR cur) { return (cur - start) % size; }
ULONG_PTR PFindData(const BYTE* data, ULONG_PTR dsize, ULONG_PTR startpos, SIZE_T fsize, HANDLE hProcess, SIZE_T bufsize) {
	BYTE* buf = (BYTE*)VirtualAlloc(NULL, bufsize, MEM_COMMIT, PAGE_READWRITE);
	BYTE* subbuf = (BYTE*)VirtualAlloc(NULL, dsize, MEM_COMMIT, PAGE_READWRITE);
	ULONG_PTR cur = startpos;
	while (cur - startpos < fsize) {
		if (boffset(bufsize, startpos, cur) == 0) {
			ReadProcessMemory(hProcess, (LPVOID)cur, buf, bufsize, NULL);
		}
		if (buf[boffset(bufsize, startpos, cur)] == data[0]) {
			ReadProcessMemory(hProcess, (LPVOID)cur, subbuf, dsize, NULL);
			ULONG_PTR subcur = 1;
			while (subcur < dsize & cur + subcur - startpos < fsize) {
				if (subbuf[subcur] != data[subcur]) {
					break;
				}
				subcur++;
			}

			if (subcur == dsize) {
				VirtualFree(buf, 0, MEM_RELEASE);
				VirtualFree(subbuf, 0, MEM_RELEASE);
				return  cur;
			}
		}
		cur++;
	}
	VirtualFree(buf, 0, MEM_RELEASE);
	VirtualFree(subbuf, 0, MEM_RELEASE);
	return -1;
}
BOOL SetPriv(const wchar_t* privname, DWORD attrs) {
	HANDLE token = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &token) == 0) { return 0; };
	TOKEN_PRIVILEGES priv;
	if (LookupPrivilegeValueW(NULL, privname, &priv.Privileges[0].Luid) == 0) { return 0; }
	priv.Privileges[0].Attributes = attrs;
	priv.PrivilegeCount = 1;
	if (AdjustTokenPrivileges(token, 0, &priv, sizeof(TOKEN_PRIVILEGES), NULL, NULL) == 0) { return 0; };
	if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) { return 0; }
	return 1;
}