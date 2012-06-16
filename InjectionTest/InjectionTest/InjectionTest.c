/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include <stdio.h>
#include "mrbInject.h"

int wmain()
{
	// Change path to application and DLL. Note that path to DLL must be an ANSI C string (char *).
	StartProcessInject(L"C:\\WINDOWS\\notepad.exe", "TestDll.dll");
	return 0;
}