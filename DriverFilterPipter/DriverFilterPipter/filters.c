#include "filters.h"

_declspec(naked) void myGetProcAddress()
{
	__asm
	{
		int 3;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		nop;
		mov eax, [esp + 4];
		mov ebx, [esp + 8];
		mov eax, 0x00000000;
		jmp eax;
	}
}

_declspec(naked) void myGetProcAddressEnd()
{
}




VOID NotifyDriverOnLoad(IN PUNICODE_STRING FullImageName, 
						IN HANDLE ProcessId, 
						IN PIMAGE_INFO ImageInfo)
{
	DbgPrint("Image name loaded: %ws\n", FullImageName->Buffer);

	HookImportsOfImage((PIMAGE_DOS_HEADER)ImageInfo->ImageBase, ProcessId);
}

NTSTATUS HookImportsOfImage(PIMAGE_DOS_HEADER image_addr, HANDLE h_proc)
{
	// variables for searching
	PIMAGE_DOS_HEADER			dos_header;
	PIMAGE_NT_HEADERS			nt_header;
	PIMAGE_IMPORT_DESCRIPTOR	import_descriptor;
	PIMAGE_IMPORT_BY_NAME		import_name;
	ULONG						import_start_rva;
	PULONG						pd_iat, pd_into;
	size_t						count = 0, index = 0;
	char*						dll_name = NULL;
	// variables of the hook
	char*						pc_dlltar = "kernel32.dll"; // dll to hook
	char*						pc_fnctar = "GetProcAddress"; // function to hook
	// variables to modify protection of memory
	PMDL						p_mdl;
	PULONG						mapped_im_table;
	// data to inject a function in kernel32
	PVOID						address_of_function = NULL;
	SIZE_T						func_size = (SIZE_T)((ULONG)myGetProcAddressEnd - (ULONG)myGetProcAddress);
	// start parsing the pe file
	dos_header = image_addr;

	// check MZ signature
	if (dos_header->e_magic != IMAGE_DOS_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;

	nt_header = (PIMAGE_NT_HEADERS)((ULONG)dos_header + dos_header->e_lfanew);
	// check PE signature
	if (nt_header->Signature != IMAGE_NT_SIGNATURE)
		return STATUS_INVALID_IMAGE_FORMAT;
	// get the RVA of import descriptor table
	import_start_rva = nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	// check if the RVA of the import descriptor table is correct
	if (!import_start_rva || (import_start_rva > nt_header->OptionalHeader.SizeOfImage))
		return STATUS_INVALID_IMAGE_FORMAT;
	// finally get the VA of the import descriptor table
	import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(import_start_rva + (ULONG)dos_header);

	// go through all the dlls
	for(count = 0 ; import_descriptor[count].Characteristics != 0; count++)
	{
		if (import_descriptor[count].Name > nt_header->OptionalHeader.SizeOfImage)
			return STATUS_INVALID_IMAGE_FORMAT;

		dll_name = (char *)(import_descriptor[count].Name + (ULONG)dos_header);

		if (_stricmp(dll_name, pc_dlltar) != 0)
			continue;
		
		if (import_descriptor[count].FirstThunk > nt_header->OptionalHeader.SizeOfImage || 
			import_descriptor[count].OriginalFirstThunk > nt_header->OptionalHeader.SizeOfImage)
			return STATUS_INVALID_IMAGE_FORMAT;

		pd_iat = (PULONG)(
			(ULONG)dos_header +
			(ULONG)import_descriptor[count].FirstThunk
			);

		pd_into = (PULONG)(
			(ULONG)dos_header +
			(ULONG)import_descriptor[count].OriginalFirstThunk
			);

		// go through all the functions
		for(index = 0; pd_iat[index] != 0; index++)
		{
			if ((pd_into[index] & IMAGE_ORDINAL_FLAG) != IMAGE_ORDINAL_FLAG) // check if its pointer to name
			{
				if (pd_into[index] > nt_header->OptionalHeader.SizeOfImage)
					return STATUS_INVALID_IMAGE_FORMAT;

				// point to the function name string
				// (remember there're two hint bytes)
				import_name = (PIMAGE_IMPORT_BY_NAME)(
					pd_into[index] +
					(ULONG)dos_header
					);

				if (strcmp(import_name->Name, pc_fnctar) == 0)
				{
					/*
					*	Now we will use the same trick we applied on SSDT hooking
					*	we will change memory permission to be able to manipulate
					*	pointer of the function.
					*
					*	Map the memory into our domain, and change MDL permissions
					*/
					p_mdl = MmCreateMdl(NULL, &pd_iat[index], sizeof(&pd_iat[index]));

					if (!p_mdl)
						return STATUS_UNSUCCESSFUL;

					MmBuildMdlForNonPagedPool(p_mdl);

					// finally change the flags
					p_mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;

					mapped_im_table = MmMapLockedPages(p_mdl, KernelMode);

					NTSTATUS status = ZwAllocateVirtualMemory(h_proc,
						address_of_function,
						(ULONG_PTR)NULL,
						&func_size,
						MEM_COMMIT | MEM_RESERVE,
						PAGE_EXECUTE_READWRITE);

					if (!NT_SUCCESS(status))
						return status;
					// set the jump address in function
					memcpy((VOID*)((ULONG)myGetProcAddressEnd - 6), (VOID*)mapped_im_table, 4);
					// copy function to kernel32 address
					memcpy((VOID*)address_of_function, (VOID*)myGetProcAddress, func_size);
					// set new function in that pointer
					*mapped_im_table = (ULONG)address_of_function;
				}
			}
		}
		
	}
	return STATUS_SUCCESS;
}

