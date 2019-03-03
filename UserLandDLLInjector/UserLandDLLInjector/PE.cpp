#include "PE.h"

PE::PE(HANDLE process_handle) : pe_arch(architecture_enum::NO_ARCH),
pe_type(file_type_enum::exe_file_k),
is_process(TRUE)
{
	this->process_handle = process_handle;
}

PE::PE(std::string file_path) : pe_arch(architecture_enum::NO_ARCH),
pe_type(file_type_enum::NO_FILE),
is_process(FALSE)
{
	this->file_path = file_path;
}

BOOL PE::analyze_file()
{
	if (!open_file() || !obtain_architecture() || !obtain_type())
	{
		fprintf(stderr, "Not possible to open the file, obtain architecture or obtain type");
		close();
		return FALSE;
	}

	return TRUE;
}

VOID PE::close()
{
	if (!is_process)
	{
		virtual_file_address		= NULL;
		f_dos_header				= NULL;
		f_signature					= NULL;
		f_magic						= NULL;

		if (handler_file_mapping != INVALID_HANDLE_VALUE)
			CloseHandle(handler_file_mapping);
		if (handler_file != INVALID_HANDLE_VALUE)
			CloseHandle(handler_file);
	}
}

BOOL PE::open_file()
{
	if (is_process)
		return TRUE;

	handler_file = CreateFileA(
		file_path.c_str(),
		GENERIC_READ,
		FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	if (handler_file == INVALID_HANDLE_VALUE)
		return FALSE;

	handler_file_mapping = CreateFileMapping(handler_file, NULL, PAGE_READONLY, 0, 0, NULL);

	if (handler_file_mapping == NULL)
	{
		CloseHandle(handler_file);
		return FALSE;
	}

	virtual_file_address = MapViewOfFile(handler_file_mapping, FILE_MAP_READ, 0, 0, 0);

	if (virtual_file_address == NULL)
	{
		CloseHandle(handler_file_mapping);
		CloseHandle(handler_file);
		return FALSE;
	}

	// securty PE checks
	f_dos_header = (PIMAGE_DOS_HEADER)virtual_file_address;

	if (f_dos_header->e_magic != IMAGE_DOS_SIGNATURE) // is it MZ?
		return FALSE;

	f_signature = (PDWORD)((DWORD)virtual_file_address + f_dos_header->e_lfanew);

	if ((*f_signature) != IMAGE_NT_SIGNATURE) // is it PE?
		return FALSE;

	f_file_header = (IMAGE_FILE_HEADER*)((DWORD)virtual_file_address + f_dos_header->e_lfanew + sizeof(DWORD));
	f_magic = (PWORD)((DWORD)virtual_file_address + f_dos_header->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));

	return TRUE;
}

BOOL PE::obtain_architecture()
{
	BOOL wow64_process;

	if (is_process)
	{
		if (!IsWow64Process(process_handle, &wow64_process))
			return FALSE;

		pe_arch = wow64_process ? architecture_enum::x86_k : architecture_enum::x64_k;
	}
	else
	{
		if (*f_magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			pe_arch = architecture_enum::x86_k;
		else if (*f_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			pe_arch = architecture_enum::x64_k;
		else
			return FALSE;
	}

	return TRUE;
}

BOOL PE::obtain_type()
{
	if (is_process)
		return TRUE;

	if (f_file_header->Characteristics & IMAGE_FILE_DLL)
		pe_type = file_type_enum::dll_file_k;
	else if (f_file_header->Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)
		pe_type = file_type_enum::exe_file_k;
	else
		return FALSE;

	return TRUE;
}

PE::architecture_enum PE::get_architecture()
{
	return pe_arch;
}

PE::file_type_enum PE::get_type()
{
	return pe_type;
}