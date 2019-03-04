#pragma once

#ifndef HOOK_IDT_H
#define HOOK_IDT_H

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>

// Maximum number of entries on the IDT (255)
#define MAX_NUMBER_OF_ENTRIES		0xFF
// Index of IDT to hook
#define SYSTEM_SERVICE_VECTOR		0x2e
#define NT_SYSTEM_SERVICE_INT		SYSTEM_SERVICE_VECTOR

// necessary to create an address with two shorts
#define MAKE_ADDRESS(low_word,high_word) ((unsigned long) (((unsigned short) (low_word)) | ((unsigned long) ((unsigned short)(high_word))) << 16))

/*
*	Structure for the IDTR (Interrupt descriptor table register
*	this structure saves the base address of the IDT (interrupt
*	descriptor table) in memory.
*	This is the structure that will be used to get the IDTR value
*	as we can see it shows the limit, and the address as two shorts
*/
typedef struct _IDTR
{
	unsigned short IDTLimit;
	unsigned short LowIDTBase;
	unsigned short HighIDTBase;
} IDTR;


/*
*	IDT table have 256 entries. Each entry from
*	the IDT contains a pointer to routine known
*	as ISR (Interrupt Service Routine).
*	This is the structure for each IDT entry.
*
*	We use pragma pack to keep the structure followed
*	in memory with alignment on a 1-unsigned char boundary.
*	In a Windows System the interrupt for a system
*	call will be the offset 0x2E in the IDT table.
*/
#pragma pack(1)
typedef struct _IDT_DESCRIPTOR
{
	unsigned short LowOffset;	// Bits[0 to 15] offset address
	unsigned short selector;  // Bits[16 to 31] segment selector (value moved to CS).

	unsigned char unused_lo;
	/*
	Above will be the same than:
		unsigned char unused : 5; // not used
		unsigned char zeroes : 3; // and shoud be 0

		So we will not care about this
	*/
	unsigned char segment_type : 4; // also known as gate type
	/*
	*	Values for segment_type:
	*		0b0101 	0x5 	5 	80386 32 bit task gate
	*		0b0110 	0x6 	6 	80286 16-bit interrupt gate
	*		0b0111 	0x7 	7 	80286 16-bit trap gate
	*		0b1110 	0xE 	14 	80386 32-bit interrupt gate
	*		0b1111 	0xF 	15 	80386 32-bit trap gate
	*/

	unsigned char system_segment_flag : 1; //  Set to 0 for interrupt and trap gates (see below). 
	unsigned char DPL : 2; // privilege level descriptor
	unsigned char P : 1; // present
	unsigned short HiOffset;
} IDT_ENTRY, *PIDT_ENTRY;
#pragma pack()

#endif // !HOOK_IDT_H
