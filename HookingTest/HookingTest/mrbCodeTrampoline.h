/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBCODETRAMPOLINE
#define MRBCODETRAMPOLINE

#include <windows.h>

typedef struct CodeTrampoline *CodeTrampoline;

struct CodeTrampoline {
	void *lpOldCode;
	void *lpNewCode;
	void *lpTrampoline;
	unsigned int szTrampoline;
};

#endif