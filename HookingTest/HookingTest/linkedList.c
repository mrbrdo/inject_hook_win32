/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#include "linkedList.h"
#include <stdlib.h>

#ifndef NULL
#define NULL 0
#endif

LinkedList LinkedList_Append(LinkedList *_head, void *value)
{
	LinkedList head = *_head;
	LinkedList newItem = (LinkedList) malloc(sizeof(struct _LinkedList));

	if (head == NULL)
		*_head = newItem;
	else {
		while (head->next != NULL) {
			head = head->next;
		}
		head->next = newItem;
	}
	newItem->next = NULL;
	newItem->value = value;
	return newItem;
}

// returns next item
LinkedList LinkedList_RemoveByValue(LinkedList *_head, void *value)
{
	LinkedList head = *_head;
	LinkedList previous = NULL;

	if (head->value == value) {
		*_head = head->next;
		free(head);
		return *_head;
	}

	previous = head;
	while ((head = head->next) != NULL) {
		if (head->value == value) {
			previous->next = head->next;
			free(head);
			return previous->next;
		}
		previous = head;
	}
	return NULL;
}

ThreadList ThreadList_Append(ThreadList *_head, DWORD threadId, HANDLE threadHandle)
{
	ThreadList head = *_head;
	ThreadList newItem = (ThreadList) malloc(sizeof(struct _ThreadList));

	if (head == NULL)
		*_head = newItem;
	else {
		while (head->next != NULL) {
			head = head->next;
		}
		head->next = newItem;
	}
	newItem->next = NULL;
	newItem->threadId = threadId;
	newItem->threadHandle = threadHandle;
	return newItem;
}

// returns next item
ThreadList ThreadList_Remove(ThreadList *_head, ThreadList item)
{
	ThreadList head = *_head;
	ThreadList previous = NULL;

	if (head == item) {
		*_head = head->next;
		CloseHandle(head->threadHandle);
		free(head);
		return *_head;
	}

	previous = head;
	while ((head = head->next) != NULL) {
		if (head == item) {
			previous->next = head->next;
			CloseHandle(head->threadHandle);
			free(head);
			return previous->next;
		}
		previous = head;
	}
	return NULL;
}

void IHLAdd(IHList *_head, LPVOID address, LPVOID redirect, BYTE value, LPVOID trampoline)
{
	IHList head = *_head;
	IHList newItem = (IHList) malloc(sizeof(struct _IHList));

	if (head == NULL)
		*_head = newItem;
	else {
		while (head->next != NULL) {
			head = head->next;
		}
		head->next = newItem;
	}

	newItem->next = NULL;
	newItem->address = address;
	newItem->redirect = redirect;
	newItem->value = value;
	newItem->trampoline = trampoline;
}

void IHLRemove(IHList *ptl_p, LPVOID address)
{
    IHList ptl = *ptl_p;
	IHList prev;

    if (ptl != NULL) {
        while ( ptl != NULL && ptl->address == address ) {
            *ptl_p = ptl->next;
            free(ptl);
			ptl = *ptl_p;
        }

		if (ptl != NULL) {
	        prev = ptl;
			ptl = ptl->next;

	        while (ptl != NULL) {
	            if (ptl->address == address) {
	                prev->next = ptl->next;
	                free(ptl);
					ptl = prev;
	            }
	            prev = ptl;
	            ptl = ptl->next;
	        }
		}
    }
}