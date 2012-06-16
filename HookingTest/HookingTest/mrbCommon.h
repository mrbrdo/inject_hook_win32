/*
  This file is licensed under Creative Commons Attribution-Non-Commercial-Share Alike 2.5 Slovenia.
  http://creativecommons.org/licenses/by-nc-sa/2.5/si/deed.en_GB
*/

#ifndef MRBCOMMON
#define MRBCOMMON

#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))
#define JmpRelativeAddress( ptr, offset, addr ) *((LPVOID *) &ptr[offset]) = (LPVOID)( (DWORD_PTR)(addr) - (DWORD_PTR)(ptr) - (DWORD_PTR) (offset + 4) )
#define JmpAbsoluteAddress( ptr, offset, addr ) *((LPVOID *) &ptr[offset]) = (LPVOID)( (DWORD_PTR)(addr) )
#define JmpRelativeAddressBased( ptr, offset, addr, base, add_subtract ) *((LPVOID *) &ptr[offset]) = (LPVOID)( (DWORD_PTR)(addr) - (DWORD_PTR)(base) - (DWORD_PTR) (offset + 4 + add_subtract) )
#ifndef _MSC_VER
	#define bool byte
	#define true 1
	#define false 0
#endif

#endif