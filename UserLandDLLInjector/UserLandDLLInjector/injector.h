#pragma once

#ifndef INJECTOR_H
#define INJECTOR_H

#include "common.h"
#include "PE.h"

class Injector
{
public:
	Injector() = default;
	~Injector() = default;

	BOOL create_process_and_inject(std::string name, std::string dll_path);
	BOOL inject_to_process(std::string name, std::string dll_path);
	BOOL list_processes();

private:

	// Properties
	bool is_process_created = false;

	// Process info
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	std::unique_ptr<PE> process;

	// File info
	std::unique_ptr<PE> file;

	// private method's
	BOOL check_if_dll_exists(std::string dll_path);
	DWORD get_pid_of_process(std::string name);
	BOOL inject_dll(std::string path_to_dll);
	BOOL injection_check(std::string path_to_dll);
};

#endif // !INJECTOR_H
