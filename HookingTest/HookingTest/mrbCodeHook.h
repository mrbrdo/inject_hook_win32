/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBCODEHOOK
#define MRBCODEHOOK

#include <windows.h>
#include "linkedList.h"
#include "mrbThreadMonitor.h"

IHList InstructionHookList;

BOOL WINAPI CreateSIH(LPVOID *trampoline, LPVOID lpNewCode);
BOOL WINAPI RemoveSIH(LPVOID *trampoline);

void WINAPI PauseOtherThreads();

void InitSIH();
void DestroySIH();

#endif