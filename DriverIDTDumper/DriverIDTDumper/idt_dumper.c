#include "idt_dumper.h"

// Functions declaration
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);

// Necessary to set the entry point of the driver
DRIVER_INITIALIZE DriverEntry;

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	DbgPrint("%s\n", "Driver unloaded.");
	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	IDTR			idtr;
	PIDT_ENTRY		idt_entry, aux;
	size_t			count;
	DWORD			sidt_address;
	char			message[255];

	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	__asm
	{
		cli;			// clear interrupt flag, so ignore other interrupts
		sidt idtr;
		sti;			// set interrupt flag, continue managing interrupts
	}

	// now get the base address of the IDT
	idt_entry = (PIDT_ENTRY)MAKE_ADDRESS(idtr.LowIDTBase, idtr.HighIDTBase);
	// dump each entry from the IDT
	for (count = 0; count <= MAX_NUMBER_OF_ENTRIES; count++)
	{
		aux = &idt_entry[count];
		
		sidt_address = MAKE_ADDRESS(aux->LowOffset, aux->HiOffset);

		_snprintf(message, 254, "Interrupt offset 0x%x: ISR address 0x%x", count, sidt_address);

		DbgPrint("%s\n", message);
	}

	return STATUS_SUCCESS;
}