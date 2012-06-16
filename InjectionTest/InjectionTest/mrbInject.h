/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBINJECT
#define MRBINJECT

#include <windows.h>
#include <tchar.h>

BOOL WINAPI StartProcessInject(
   LPCTSTR lpApplicationName,
   LPCSTR lpDllFullPath
);

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
);

BOOL WINAPI ProcessInject(
   DWORD dwProcessId,
   LPCSTR lpDllFullPath
);

#endif