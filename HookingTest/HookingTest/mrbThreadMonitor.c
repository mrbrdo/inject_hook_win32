/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include "mrbThreadMonitor.h"

/***********************************************************************
 *                       ZwCreateThread hook                           *
 ***********************************************************************/
static NTSTATUS
(NTAPI *ZwCreateThreadOriginal)(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    /*POBJECT_ATTRIBUTES*/LPVOID ObjectAttributes,
    HANDLE ProcessHandle,
    /*PCLIENT_ID*/LPVOID ClientId,
    PCONTEXT ThreadContext,
    /*PINITIAL_TEB*/LPVOID UserStack,
    BOOLEAN CreateSuspended
) = NULL;

NTSTATUS
NTAPI ZwCreateThreadHook(
    PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    /*POBJECT_ATTRIBUTES*/LPVOID ObjectAttributes,
    HANDLE ProcessHandle,
    PCLIENT_ID ClientId,
    PCONTEXT ThreadContext,
    /*PINITIAL_TEB*/LPVOID UserStack,
    BOOLEAN CreateSuspended
)
{
	LONG ret = ZwCreateThreadOriginal(ThreadHandle, DesiredAccess, ObjectAttributes,
		ProcessHandle, ClientId, ThreadContext, UserStack, CreateSuspended);
	if (ret == STATUS_SUCCESS && ThreadHandle != NULL)
		ThreadList_Append(&Threads, (DWORD) ClientId->UniqueThread,
			OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, (DWORD) ClientId->UniqueThread));
	return ret;
}

/***********************************************************************
 *                      ZwTerminateThread hook                         *
 ***********************************************************************/

static NTSTATUS
(NTAPI
*ZwTerminateThreadOriginal)(
    HANDLE ThreadHandle, NTSTATUS ExitStatus
) = NULL;

NTSTATUS
NTAPI
ZwTerminateThreadHook(
    HANDLE ThreadHandle, NTSTATUS ExitStatus
)
{
	// No way to retrieve threadID from handle...
	DWORD thread_return;
	LONG ret = ZwTerminateThreadOriginal(ThreadHandle, ExitStatus);
	ThreadList cur = Threads;

	// remove terminated threads from thread list
	if (ret == STATUS_SUCCESS && ThreadHandle != NULL) {
		while (cur != NULL) {
			if (GetExitCodeThread(cur->threadHandle, &thread_return) && thread_return != STILL_ACTIVE)
				cur = ThreadList_Remove(&Threads, cur);
			else
				cur = cur->next;
		}
	}

	return ret;
}

/***********************************************************************
 *                    Thread monitor initialization                    *
 ***********************************************************************/

void InitThreadMonitor()
{
	DWORD dwCurrentThread = GetCurrentThreadId();
	DWORD dwCurrentProcess = GetCurrentProcessId();
	HANDLE h;
	HookAPI(L"ntdll.dll", "ZwCreateThread", (LPVOID*)&ZwCreateThreadOriginal, &ZwCreateThreadHook);
	HookAPI(L"ntdll.dll", "ZwTerminateThread", (LPVOID*)&ZwTerminateThreadOriginal, &ZwTerminateThreadHook);
	h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (h != INVALID_HANDLE_VALUE) {
		THREADENTRY32 te;
		te.dwSize = sizeof(te);
		if (Thread32First(h, &te)) {
			do {
				if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) +
					sizeof(te.th32OwnerProcessID)) {
					if (te.th32OwnerProcessID == dwCurrentProcess) {
						ThreadList_Append(&Threads, te.th32ThreadID,
							OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID));
					}
				}
				te.dwSize = sizeof(te);
			} while (Thread32Next(h, &te));
		}
		CloseHandle(h);
	}
}

void DestroyThreadMonitor()
{
	UnhookAPI((LPVOID*)&ZwCreateThreadOriginal);
	UnhookAPI((LPVOID*)&ZwTerminateThreadOriginal);

	while (Threads != NULL)
		ThreadList_Remove(&Threads, Threads);
}