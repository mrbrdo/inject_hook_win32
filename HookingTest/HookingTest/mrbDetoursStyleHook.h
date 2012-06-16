/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBDETOURSSTYLEHOOK
#define MRBDETOURSSTYLEHOOK

#include <windows.h>
#include "mrbCommon.h"
#include "mrbCodeTrampoline.h"
#include "linkedList.h"
#include "disasm.h"

LinkedList Trampolines;

void HookFunction(LPVOID *lpFunctionTrampoline, LPVOID lpNewFunction);
void HookAPI(LPCTSTR lpLibName, LPCSTR lpProcName, LPVOID *lpFunctionTrampoline, LPVOID lpNewFunction);
void UnhookFunction(LPVOID *lpFunctionTrampoline);
void UnhookAPI(LPVOID *lpFunctionTrampoline);

#endif