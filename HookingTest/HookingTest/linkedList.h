/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef LINKEDLIST
#define LINKEDLIST

#include <windows.h>

typedef struct _LinkedList {
	void *value;
	struct _LinkedList *next;
} *LinkedList;

LinkedList LinkedList_RemoveByValue(LinkedList *_head, void *value);
LinkedList LinkedList_Append(LinkedList *_head, void *value);

#pragma pack(push, ALIGN_FOUR, 4)
typedef struct _ThreadList {
	DWORD threadId;
	HANDLE threadHandle;
	struct _ThreadList *next;
} *ThreadList;
#pragma pack(pop, ALIGN_FOUR)

ThreadList ThreadList_Append(ThreadList *_head, DWORD threadId, HANDLE threadHandle);
ThreadList ThreadList_Remove(ThreadList *_head, ThreadList item);

// Instruction Hook List
#pragma pack(push, ALIGN_FOUR, 4)
typedef struct _IHList {
    LPVOID address;
	LPVOID redirect;
	BYTE value;
	LPVOID trampoline;
    struct _IHList *next;
} *IHList;
#pragma pack(pop, ALIGN_FOUR)
void IHLAdd(IHList *ptl_p, LPVOID address, LPVOID redirect, BYTE value, LPVOID trampoline);
void IHLRemove(IHList *ptl_p, LPVOID address);

#endif