/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include "mrbCommon.h"
#include "mrbInject.h"

BOOL EnablePriv(LPTSTR lpszPriv)
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tkprivs;
    BOOL bRet;
    
    ZeroMemory(&tkprivs, sizeof(tkprivs));
    
    if(!OpenProcessToken(GetCurrentProcess(), (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY), &hToken))
        return FALSE;
    
    if(!LookupPrivilegeValue(NULL, lpszPriv, &luid)){
        CloseHandle(hToken); return FALSE;
    }
    
    tkprivs.PrivilegeCount = 1;
    tkprivs.Privileges[0].Luid = luid;
    tkprivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    bRet = AdjustTokenPrivileges(hToken, FALSE, &tkprivs, sizeof(tkprivs), NULL, NULL);
    CloseHandle(hToken);
    return bRet;
}

BOOL CreateInjectTrampoline(HANDLE hProcess, HANDLE hMainThread, LPVOID dllStrPtr)
{
	SIZE_T count;
	// prepare return code
	DWORD returnCodeSz = 26;
	byte returnCode[] = {
		0x9C, // pushfd
		0x60, // pushad
		0x68, 0x00, 0x00, 0x00, 0x00, // push dllName
		0xE8, 0x00, 0x00, 0x00, 0x00, // call LoadLibraryA
		0xC6, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, // mov BYTE PTR[returnCode], 0
		0x61, // popad
		0x9D, // popfd
		0xE9, 0x00, 0x00, 0x00, 0x00 // jmp originalEip
		};
	LPVOID codePtr;
	CONTEXT context, tmpContext;
	DWORD originalEip;
	BYTE bBuffer;
	SIZE_T numBytes;


	// save code in target process
	codePtr = VirtualAllocEx(hProcess, NULL, returnCodeSz, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!codePtr) return FALSE;

	// Cache thread context
	context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
	GetThreadContext(hMainThread, &context);
	originalEip = context.Eip;
	
	// Insert addresses into assembly code
	// dll path string address
	JmpAbsoluteAddress(returnCode, 3, dllStrPtr);
	// LoadLibraryA address
	JmpRelativeAddressBased(returnCode, 8, &LoadLibraryA, codePtr, 0);
	// returnCode address
	JmpAbsoluteAddress(returnCode, 14, codePtr);
	// original EIP address
	JmpRelativeAddressBased(returnCode, 22, originalEip, codePtr, 0);

	// Write assembly code
	if (!WriteProcessMemory(hProcess, codePtr, (LPVOID) returnCode,
		returnCodeSz, &count)) return FALSE;

	// "Jump" to our assembly code in target thread
	context.Eip = (DWORD) codePtr;
	SetThreadContext(hMainThread, &context);
	// Resume thread
	ResumeThread(hMainThread);

	// Poll the thread until our code has finished execution to clean up afterwards
	tmpContext.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;

	Sleep(1); // The thread has just been resumed, Sleep before suspending it first
	while (SuspendThread(hMainThread) != (DWORD) -1) {
			GetThreadContext(hMainThread, &tmpContext);
			ReadProcessMemory(hProcess, codePtr, (LPVOID) &bBuffer, 1, &numBytes); // guard byte
			ResumeThread(hMainThread);
			if (
				!(tmpContext.Eip >= (DWORD) codePtr && tmpContext.Eip <= ((DWORD) codePtr + returnCodeSz)) && // our code is not running
				(bBuffer == 0)	// our code overwrites itself to let us know when it's finished
								// this is needed because our code calls APIs which reside outside of our code
				) 
				break;
			Sleep(1);
	}
	
	// Free memory for our code
	VirtualFreeEx(hProcess, codePtr, 0, MEM_RELEASE);

	return TRUE;
}

BOOL WINAPI ProcessUninitializedInject(
   DWORD dwProcessId,
   HANDLE hMainThread,
   LPCSTR lpDllFullPath
)
{
	HANDLE hProcess;
	LPVOID dllStrPtr;
	SIZE_T written;
	
	// Enable SE_DEBUG_NAME privilege
	if (!EnablePriv(SE_DEBUG_NAME)) return FALSE;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (!hProcess) return FALSE;

	// Write dll path string to process memory
	dllStrPtr = VirtualAllocEx(hProcess, NULL, strlen(lpDllFullPath) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!dllStrPtr) return FALSE;

	if (!WriteProcessMemory(hProcess, dllStrPtr, (LPVOID) lpDllFullPath, strlen(lpDllFullPath) + 1, &written)) return FALSE;

	if (!CreateInjectTrampoline(hProcess, hMainThread, dllStrPtr)) return FALSE;

	VirtualFreeEx(hProcess, dllStrPtr, 0, MEM_RELEASE);

	CloseHandle(hProcess);
	return TRUE;
}

BOOL WINAPI CreateProcessInject(
	LPCTSTR lpApplicationName,
	LPTSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCTSTR lpCurrentDirectory,
	LPSTARTUPINFO lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation,
	LPCSTR lpDllFullPath
)
{
	if( CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes,
			lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment,
			lpCurrentDirectory, lpStartupInfo, lpProcessInformation) &&
		ProcessUninitializedInject(lpProcessInformation->dwProcessId, lpProcessInformation->hThread, lpDllFullPath) )
		return TRUE;
	else
		return FALSE;
}

BOOL WINAPI StartProcessInject(
   LPCTSTR lpApplicationName,
   LPCSTR lpDllFullPath
)
{
    STARTUPINFO lpStartupInfo;
    PROCESS_INFORMATION lpProcessInformation;
	LPTSTR appFolder;
	int slash = _tcslen(lpApplicationName);
	BOOL ret;

	while (--slash > 0) {
		if (lpApplicationName[slash] == '\\' || lpApplicationName[slash] == '/')
			break;
	}
	if (!slash)
		appFolder = NULL;
	else {
		appFolder = (LPTSTR) malloc((slash+1) * sizeof(TCHAR));
		appFolder[slash] = '\0';
		while (--slash >= 0) {
			appFolder[slash] = lpApplicationName[slash];
		}
	}
    memset(&lpStartupInfo, 0, sizeof(lpStartupInfo));
    memset(&lpProcessInformation, 0, sizeof(lpProcessInformation));
    lpStartupInfo.cb = sizeof(lpStartupInfo);
	ret = CreateProcessInject(lpApplicationName, NULL, NULL,
		NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL,
		NULL, &lpStartupInfo, &lpProcessInformation, lpDllFullPath);
	CloseHandle(lpProcessInformation.hProcess);
    CloseHandle(lpProcessInformation.hThread);
	if (appFolder)
		free(appFolder);
	return ret;
}