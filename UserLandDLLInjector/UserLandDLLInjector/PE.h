#pragma once

#ifndef PE_H
#define PE_H

#include "common.h"

class PE
{
public:
	enum architecture_enum
	{
		x86_k	= 0,
		x64_k	= 1,
		NO_ARCH	= 99
	};

	enum file_type_enum
	{
		exe_file_k	= 0,
		dll_file_k	= 1,
		NO_FILE		= 99
	};

	PE(HANDLE process_handle);
	PE(std::string file_path);

	BOOL analyze_file();
	VOID close();

	architecture_enum	get_architecture();
	file_type_enum		get_type();

private:

	BOOL open_file();
	BOOL obtain_architecture();
	BOOL obtain_type();

	architecture_enum pe_arch;
	file_type_enum	pe_type;
	BOOL is_process;

	// variables for PROCESS
	HANDLE process_handle;
	// variables for FILE
	std::string file_path;
	HANDLE handler_file;
	HANDLE handler_file_mapping;
	LPVOID virtual_file_address;
	PIMAGE_DOS_HEADER  f_dos_header;
	PDWORD f_signature;
	PIMAGE_FILE_HEADER f_file_header;
	PWORD f_magic;
};

#endif // !PE_H
