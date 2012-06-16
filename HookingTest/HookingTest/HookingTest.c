/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include <stdio.h>
#include <windows.h>
#include "mrbCodeHook.h"

// function hook example

// compiler optimizations may inline very short functions, therefore we need
// to tell the compiler explicitly not to inline these short example functions
__declspec(noinline) int ExampleFunc(int i)
{
	printf("F1\n");
	return 2*i;
}

static int (*RealExampleFunc)(int);

__declspec(noinline) int ExampleFuncHook(int i)
{
	printf("F3\n");
	return RealExampleFunc(i) + 1;
}

void HookFuncExample()
{
	RealExampleFunc = &ExampleFunc;
	CreateSIH((LPVOID *) &RealExampleFunc, (LPVOID) &ExampleFuncHook);
	printf("Result: %d\n", ExampleFunc(10));
	RemoveSIH((LPVOID *) &RealExampleFunc);
}

// API hook example

static VOID (WINAPI *RealSleep)(DWORD);

VOID WINAPI SleepHook(DWORD dwMilliseconds)
{
	printf("Sleeping for %dms\n", dwMilliseconds);
	RealSleep(dwMilliseconds);
	printf("Done\n");
}

void HookAPIExample()
{
	RealSleep = &Sleep;
	CreateSIH((LPVOID *) &RealSleep, (LPVOID) &SleepHook);
	Sleep(1000);
	RemoveSIH((LPVOID *) &RealSleep);
}

int wmain()
{
	InitSIH();

	HookFuncExample();
	HookAPIExample();

	DestroySIH();
	system("PAUSE");
	return 0;
}