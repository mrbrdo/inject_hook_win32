/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include "mrbDetoursStyleHook.h"
#include <stdio.h>

BOOL WINAPI CreateCodeTrampoline(LPVOID lpOldCode, LPVOID lpNewCode, CodeTrampoline result)
{
	SIZE_T count;
	int jmpCodeSz = 5;
	int exCodeSz = 64;
	int iBufferSz = 0;
	int rawSize;
	byte jmpCode[5];
	int returnCodeSz = 5;
	byte returnCode[5];
	void *codePtr;
	DWORD old_protect;
	t_disasm da;
	char *lpOldCodeChars = (char *)lpOldCode;

	// see how many instructions we need to copy
	rawSize = Disasm(lpOldCodeChars, exCodeSz, 0x400000, &da, DISASM_SIZE);
	while (TRUE)
	{
		if (rawSize > 0) {
			#define OP_JMP_SHORT_REL 0xEB
			#define OP_JMP_NEAR_REL 0xE9
			#define OP_JMP_NEAR_ABS 0xFF
			#define OP_RET_NEAR 0xC3
			#define OP_RET_FAR 0xCB
			#define OP_RET_NEAR_POP 0xC2
			#define OP_RET_FAR_POP 0xCA
			byte b = lpOldCodeChars[iBufferSz];
			if (b == OP_JMP_SHORT_REL ||
				b == OP_JMP_NEAR_REL ||
				b == OP_JMP_NEAR_ABS ||
				b == OP_RET_NEAR ||
				b == OP_RET_FAR ||
				b == OP_RET_NEAR_POP || // ret far we could use if it's the last instruction
				b == OP_RET_FAR_POP) {
					break;
			}
		}
		iBufferSz += rawSize;
		if (iBufferSz >= jmpCodeSz) break;
		rawSize = Disasm(&lpOldCodeChars[iBufferSz], exCodeSz-iBufferSz, 0x400000, &da, DISASM_SIZE);
	}
	if (iBufferSz < jmpCodeSz) return FALSE;
	// prepare jump code
	jmpCode[0] = 0xE9; // jmp relative
	// prepare return code
	returnCode[0] = 0xE9; // jmp relative
	// save code in target process
	codePtr = malloc(iBufferSz+returnCodeSz);
	if (!codePtr) return FALSE;
	if (!VirtualProtect(codePtr, iBufferSz+returnCodeSz, PAGE_EXECUTE_READWRITE, &count)) return FALSE;
	// insert addresses
	JmpRelativeAddressBased(jmpCode, 1, lpNewCode, lpOldCode, 0); // jump to new code
	JmpRelativeAddressBased(returnCode, 1, lpOldCode, codePtr, 0); // - iBufferSz + iBufferSz
									// overflow can happen here but that's correct (negative offset is ok)
	// write changes
	if (!memcpy(codePtr, lpOldCode, iBufferSz)) return FALSE;
	if (!memcpy(MakePtr(LPVOID, codePtr, iBufferSz), returnCode, returnCodeSz)) return FALSE;
	// rewrite old address
	if (!VirtualProtect(lpOldCode, jmpCodeSz, PAGE_EXECUTE_READWRITE, (PDWORD) &old_protect))
		return FALSE;
	if (!memcpy(lpOldCode, jmpCode, jmpCodeSz)) return FALSE;
	if (!VirtualProtect(lpOldCode, jmpCodeSz, old_protect, (PDWORD) &count))
		return FALSE;
	result->lpOldCode = lpOldCode;
	result->lpNewCode = lpNewCode;
	result->lpTrampoline = codePtr;
	result->szTrampoline = iBufferSz;
	return TRUE;
}

BOOL WINAPI DestroyCodeTrampoline(CodeTrampoline result)
{
	SIZE_T count;
	DWORD old_protect;

	if (!VirtualProtect(result->lpOldCode, result->szTrampoline, PAGE_EXECUTE_READWRITE,
		(PDWORD) &old_protect)) goto DestroyCodeTrampolineUnsuccessful;
	if (!memcpy(result->lpOldCode, result->lpTrampoline, result->szTrampoline)) goto DestroyCodeTrampolineUnsuccessful;
	if (!VirtualProtect(result->lpOldCode, result->szTrampoline, old_protect,
		(PDWORD) &count)) goto DestroyCodeTrampolineUnsuccessful;
	free(result->lpTrampoline);
	goto DestroyCodeTrampolineFreeBufferSucc;
DestroyCodeTrampolineUnsuccessful:
	return FALSE;
DestroyCodeTrampolineFreeBufferSucc:
	return TRUE;
}

void HookFunction(LPVOID *lpFunctionTrampoline, LPVOID lpNewFunction)
{
	CodeTrampoline result = (CodeTrampoline) malloc(sizeof(struct CodeTrampoline));
	if (CreateCodeTrampoline(*lpFunctionTrampoline, lpNewFunction, result))
		LinkedList_Append(&Trampolines, (void *) result);
	*lpFunctionTrampoline = result->lpTrampoline;
}

void HookAPI(LPCTSTR lpLibName, LPCSTR lpProcName, LPVOID *lpFunctionTrampoline, LPVOID lpNewFunction)
{
	HMODULE hLib = LoadLibrary(lpLibName);
	*lpFunctionTrampoline = (LPVOID) GetProcAddress(hLib, lpProcName);
	FreeLibrary(hLib);
	HookFunction(lpFunctionTrampoline, lpNewFunction);
}

void UnhookFunction(LPVOID *lpFunctionTrampoline)
{
	LinkedList ptl = Trampolines;
    while (ptl != NULL) {
		CodeTrampoline ct = (CodeTrampoline) ptl->value;
        if (ct->lpTrampoline == *lpFunctionTrampoline) {
			*lpFunctionTrampoline = ct->lpOldCode;
			LinkedList_RemoveByValue(&Trampolines, (void *)ct);
			DestroyCodeTrampoline(ct);
			break;
        }
        ptl = ptl->next;
    }
}

void UnhookAPI(LPVOID *lpFunctionTrampoline)
{
	UnhookFunction(lpFunctionTrampoline);
}