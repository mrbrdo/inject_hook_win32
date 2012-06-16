/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include "mrbCodeHook.h"

__declspec( thread ) LPVOID CurrentInstructionHook = NULL;
__declspec( thread ) BYTE CurrentInstructionHookByte;

/***********************************************************************
 *                        HELPER FUNCTIONS                             *
 ***********************************************************************/

// Pauses all threads in this process except the current thread
__declspec( naked ) void WINAPI PauseOtherThreads()
{
	__asm {
		call DWORD PTR [GetCurrentThreadId]	// IAT lookup
		mov edi, eax
		mov esi, DWORD PTR [Threads]
LPAUSE_LOOP:
		cmp DWORD PTR [esi], edi
		je LPAUSE_NEXT
		push DWORD PTR [esi+4]
		call DWORD PTR [SuspendThread]
LPAUSE_NEXT:
		mov esi, DWORD PTR [esi+8]
		test esi, esi
		jne LPAUSE_LOOP
		ret
	};
/*	DoubleLinkedList ptl = Threads;
	while (ptl != NULL) {
		if ((DWORD)ptl->index != GetCurrentThreadId())
			SuspendThread((HANDLE) ptl->value);
		ptl = ptl->ptr;
    }*/
}

// Resumes all threads in this process except the current thread
__declspec( naked ) void WINAPI ResumeOtherThreads()
{
	__asm {
		call DWORD PTR [GetCurrentThreadId]	// IAT lookup
		mov edi, eax
		mov esi, DWORD PTR [Threads]
LRESUME_LOOP:
		cmp DWORD PTR [esi], edi
		je LRESUME_NEXT
		push DWORD PTR [esi+4]
		call DWORD PTR [ResumeThread]		// IAT lookup
LRESUME_NEXT:
		mov esi, DWORD PTR [esi+8]
		test esi, esi
		jne LRESUME_LOOP
		ret
	};
/*	DoubleLinkedList ptl = Threads;
	while (ptl != NULL) {
		ResumeThread((HANDLE) ptl->value);
		ptl = ptl->ptr;
    }*/
}

// In some cases (e.g. debug builds), an address of a function does
// not point to the start of its code, but rather to a JMP start_of_code.
// This function gets the address of such a jump, if there is one.
LPVOID ResolveFunctionJump(LPVOID funcPtr)
{
	if (*((LPBYTE) funcPtr) == 0xE9)
	{
		return (LPVOID) (*MakePtr(LPDWORD, funcPtr, 1) + (DWORD) funcPtr + 5);
	}
	return funcPtr;
}

// TLS helper functions (naked for extra speed)
LPVOID __declspec( naked ) GetCurrentInstructionHook()
{
	{
		LPVOID r = CurrentInstructionHook;
		_asm {
			mov eax, r
			ret
		}
	}
}

BYTE __declspec( naked ) GetCurrentInstructionHookByte()
{
	{
		BYTE r = CurrentInstructionHookByte;
		_asm {
			mov al, r
			ret
		}
	}
}

void __declspec( naked ) SetCurrentInstructionHook(LPVOID value)
{
	__asm lea ebp, [esp-4]
	CurrentInstructionHook = value;
	__asm ret 4
}

void __declspec( naked ) SetCurrentInstructionHookByte(BYTE value)
{
	__asm lea ebp, [esp-4]
	CurrentInstructionHookByte = value;
	__asm ret 4
}
/***********************************************************************
 *                      CALL ORIGINAL TEMPLATE                         *
 ***********************************************************************/

__declspec( naked ) void WINAPI SIHCallOriginalTemplate()
{
	__asm {
DUMMY_ADDR:
		pushad
		pushfd

		call DUMMY_ADDR		// PauseOtherThreads()

		nop					// push lpOldCode
		nop
		nop
		nop
		nop
		call SetCurrentInstructionHook

		nop					// push exCode (byte)
		nop
		call SetCurrentInstructionHookByte

		popfd
		popad

		nop					// Jump to original function (breakpoint will trigger)
		nop					// Cannot simply use "jmp" here with DUMMY_ADDR, because that will make a short jump (0xEB opcode)
		nop
		nop
		nop
	};
}

/***********************************************************************
 *                            CREATE SIH                               *
 ***********************************************************************/

BOOL WINAPI CreateSIH(LPVOID *trampoline, LPVOID lpNewCode)
{
	DWORD oldProtect;
	BYTE exCode;
	LPBYTE callOriginalCodePtr;
	LPVOID funcPtr;
	LPVOID lpOldCode;
#define callOriginalCodeSize 0x1F

	if (trampoline == NULL || *trampoline == NULL) return FALSE;
	lpOldCode = *trampoline;

	// 1. Cache the first byte and replace it with an "int 3" instruction.
	memcpy(&exCode, lpOldCode, 1);
	if (!VirtualProtect(lpOldCode, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) // Note: for performance purposes only, the old protection
		return FALSE;														// is never restored (we allow full memory access - read, write, execute),
																			// because when original code needs to be called, write access is always required.
																			// This could easily be modified by changing the call original code, however,
																			// performance would decrease.
	*((BYTE *) lpOldCode) = 0xCC; // int 3

	// 2. Generate a trampoline through which the original code can be called
	// allocate and change protection
	callOriginalCodePtr = (LPBYTE) malloc(callOriginalCodeSize);
	VirtualProtect(callOriginalCodePtr, callOriginalCodeSize, PAGE_EXECUTE_READWRITE, (PDWORD) &oldProtect);

	// copy code template
	funcPtr = ResolveFunctionJump((LPVOID) &SIHCallOriginalTemplate);
	memcpy(callOriginalCodePtr, funcPtr, callOriginalCodeSize);

	// insert addresses, modify code appropriately
	JmpRelativeAddress(callOriginalCodePtr, 3, &PauseOtherThreads);
	callOriginalCodePtr[7] = 0x68; // push dword
	JmpAbsoluteAddress(callOriginalCodePtr, 8, lpOldCode);
	JmpRelativeAddress(callOriginalCodePtr, 13, &SetCurrentInstructionHook);

	callOriginalCodePtr[17] = 0x6A; // push byte
	callOriginalCodePtr[18] = exCode;
	JmpRelativeAddress(callOriginalCodePtr, 20, &SetCurrentInstructionHookByte);

	callOriginalCodePtr[26] = 0xE9;
	JmpRelativeAddress(callOriginalCodePtr, 27, lpOldCode);

	// Add to list of SIHs
	IHLAdd(&InstructionHookList, lpOldCode, lpNewCode, exCode, callOriginalCodePtr);
	if (trampoline != NULL)
		*trampoline = (LPVOID) callOriginalCodePtr;
	return TRUE;
}

BOOL WINAPI RemoveSIH(LPVOID *trampoline)
{
	IHList item;

	if (trampoline == NULL) return FALSE;

	item = InstructionHookList;
	while (item != NULL && item->trampoline != *trampoline) {
		item = item->next;
	}

	if (item == NULL) return FALSE;

	*((BYTE *) item->address) = item->value;
	if (trampoline != NULL)
		*trampoline = item->address;
	free(item->trampoline);
	IHLRemove(&InstructionHookList, item->address);

	return TRUE;
}

/***********************************************************************
 *                  KiUserExceptionDispatcher hook                     *
 ***********************************************************************/

// Note: When a debugger is present, RtlDispatchException (which is not exported)
// would have to be called instead of KiUserExceptionDispatcher.

// Note: KiUserExceptionDispatcher has an unconventional calling convention. It is similar to Stdcall,
// but does not have a return address (so it needs not to be pushed onto the stack)
static VOID (NTAPI *KiUserExceptionDispatcherOriginal)(/*PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context*/) = NULL;

__declspec( naked ) VOID NTAPI KiUserExceptionDispatcherHook(/*PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context*/)
{
	__asm {

		// Check if original code is being called
		call GetCurrentInstructionHook
		mov esi, eax
		mov edi, DWORD PTR [esp+4]			// Context parameter
		cmp DWORD PTR[edi+184], esi			// Context.Eip

		je RESTORE_CALL_ORIGINAL_CODE

		// Check if this is a return from the original code
		test esi, esi
		jne RESTORE_BREAKPOINT_RESUME

		/********************************************************************
		 *                      Code redirection                            *
		 ********************************************************************/

		// See if this is a normal call to code, which we need to redirect
		mov eax, DWORD PTR [edi+184]		// Context.Eip

		mov esi, InstructionHookList
IHCHECK_LOOP:
		cmp esi, 0
		je IHCHECK_END
		cmp DWORD PTR [esi], eax
		jne IHCHECK_NEXT

		mov eax, DWORD PTR [esi+4]			// Redirect address
		mov DWORD PTR [edi+184], eax

		push edi
		call DWORD PTR [GetCurrentThread]
		push eax
		call DWORD PTR [SetThreadContext]	// Redirect by changing context

IHCHECK_NEXT:
		mov esi, DWORD PTR [esi+16]
		jmp IHCHECK_LOOP

IHCHECK_END:
		// The exception does not concern us.
		jmp KiUserExceptionDispatcherOriginal // special calling convention, no return address

		/********************************************************************
		 *            Call original - restoring original byte               *
		 ********************************************************************/
RESTORE_CALL_ORIGINAL_CODE:
		call GetCurrentInstructionHookByte
		mov cl, al
		mov BYTE PTR [esi], cl

		or DWORD PTR [edi+192], 0x100		// set one-step TF flag in Context.EFlags

		push edi
		call DWORD PTR [GetCurrentThread]
		push eax
		call DWORD PTR [SetThreadContext]

		/********************************************************************
		 *            Call original - restoring the breakpoint              *
		 ********************************************************************/
RESTORE_BREAKPOINT_RESUME:
		mov BYTE PTR [esi], 0xCC
		push 0b
		call SetCurrentInstructionHook

		push edi							// Context, for SetThreadContext

		call ResumeOtherThreads
		call DWORD PTR [GetCurrentThread]
		push eax
		call DWORD PTR [SetThreadContext]
	}
}

/***********************************************************************
 *                        SIH initialization                           *
 ***********************************************************************/

void InitSIH()
{
	InitThreadMonitor();
	HookAPI(L"ntdll.dll", "KiUserExceptionDispatcher", (LPVOID*)&KiUserExceptionDispatcherOriginal, &KiUserExceptionDispatcherHook);
}

void DestroySIH()
{
	IHList item = InstructionHookList;
	LPVOID trampoline;
	while (item != NULL) {
		trampoline = item->trampoline;
		item = item->next;
		RemoveSIH(&trampoline);
	}
	DestroyThreadMonitor();
	UnhookAPI((LPVOID*)&KiUserExceptionDispatcherOriginal);
}