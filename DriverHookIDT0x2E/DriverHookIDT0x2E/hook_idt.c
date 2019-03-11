#include "hook_idt.h"

/*
*	Real handler of the interruption 0x2E (also called Interrupt Service Routine)
*/
DWORD KiRealSystemServiceISR_Ptr;


// Functions that we will use
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);
int HookInterrupts();
int UnhookInterrupts();
VOID LogSystemCall(DWORD dispatchID, DWORD stackPtr);
/*
*	Here we have the function which will
*	be the executed function instead of
*	the one once the INT0x2E is executed.
*
*	We will declare the function naked
*	so we will avoid any windows code.
*/
__declspec(naked) MiKiSystemService()
{
	__asm {
		pushad;					// Save all registers so nothing will break
		pushfd;					//
		push fs;				// save fs register
		mov bx, 0x30;
		mov fs, bx;				
		push ds;				// save data segment
		push es;				// save extra segment
								
	LogSyscall:
		push edx;
		push eax;
		call LogSystemCall;
	Finish:
		pop es;
		pop fs;
		pop fs;
		popfd;
		popad;
		jmp KiRealSystemServiceISR_Ptr; // finally jmp to the real ISR
	}
}

DRIVER_INITIALIZE DriverEntry;

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("%s\n", "Driver unloaded.");

	UnhookInterrupts();

	return;
}

int HookInterrupts()
{
	IDTR			idt_info;
	PIDT_ENTRY		idt_entries;
	PIDT_ENTRY		int2e_entry;

	// first of all get IDTR
	__asm
	{
		cli;			// clear interrupt flag, so ignore other interrupts
		sidt idt_info;	// get the value of idtr
		sti;			// set interrupt flag, continue managing interrupts
	}

	// now get the base address of the IDT
	idt_entries = (PIDT_ENTRY)MAKE_ADDRESS(idt_info.LowIDTBase, idt_info.HighIDTBase);

	// save address of real handler
	// for INT 0x2E
	KiRealSystemServiceISR_Ptr = MAKE_ADDRESS(idt_entries[NT_SYSTEM_SERVICE_INT].LowOffset,
		idt_entries[NT_SYSTEM_SERVICE_INT].HiOffset);

	/*
	*	Now we will hook the IDT entry for the index 0x2E
	*	this method will be valid for all the other entries
	*/
	int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);

	__asm
	{
		cli;							// clear interrupt flag, so ignore other interrupts
		lea eax, MiKiSystemService;		// get the address of our function
		mov ebx, int2e_entry;			// get the address of the entry with index 0x2E
		mov [ebx], ax;					// save the first word into the low offset of the IDT
		shr eax, 16;
		mov [ebx + 6], ax;				// save the high word into the hi offset of the IDT
		lidt idt_info;					// save the new idt
		sti;							// set interrupt flag, continue managing interrupts
	}

	return 0;
}

int UnhookInterrupts()
{
	IDTR			idt_info;
	PIDT_ENTRY		idt_entries;
	PIDT_ENTRY		int2e_entry;

	// first of all get IDTR
	__asm
	{
		cli;			// clear interrupt flag, so ignore other interrupts
		sidt idt_info;	// get the value of idtr
		sti;			// set interrupt flag, continue managing interrupts
	}

	// now get the base address of the IDT
	idt_entries = (PIDT_ENTRY)MAKE_ADDRESS(idt_info.LowIDTBase, idt_info.HighIDTBase);

	/*
	*	Now we will hook the IDT entry for the index 0x2E
	*	this method will be valid for all the other entries
	*/
	int2e_entry = &(idt_entries[NT_SYSTEM_SERVICE_INT]);

	__asm
	{
		cli;									// clear interrupt flag, so ignore other interrupts
		lea eax, KiRealSystemServiceISR_Ptr;	// get the address real function
		mov ebx, int2e_entry;					// get the address of the entry with index 0x2E
		mov[ebx], ax;							// save the first word into the low offset of the IDT
		shr eax, 16;
		mov[ebx + 6], ax;						// save the high word into the hi offset of the IDT
		lidt idt_info;							// save the old idt
		sti;									// set interrupt flag, continue managing interrupts
	}
	
	return 0;
}

VOID LogSystemCall(DWORD dispatchID, DWORD stackPtr)
{
	DbgPrint(
		"[RegisterSystemCall]: CPU[%u] of %u, (%s, pid=%u, dID=%x)\n",
		KeGetCurrentProcessorNumber(),
		KeNumberProcessors,
		(BYTE *)PsGetCurrentProcess() + 0x16c,
		PsGetCurrentProcessId(),
		dispatchID
	);
	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = Unload;
	DbgPrint("%s\n", "Driver loaded");

	HookInterrupts();
	
	return status;
}