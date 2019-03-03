// UserLandDLLInjector.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include "common.h"
#include "injector.h"

void print_help();

int main(int argc, char *argv[])
{
	BOOL create_process = FALSE, inject_into_process = FALSE;
	std::string process_name, dll_to_inject;
	Injector injector;
	size_t i;
	
	if (argc < 2)
	{
		print_help();
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i],"-h") == 0)
		{
			print_help();
			return 0;
		}

		if (strcmp(argv[i], "-p") == 0)
		{
			inject_into_process = TRUE;

			if ((i == argc - 1) || argv[i + 1][0] == '-')
			{
				print_help();
				return -1;
			}

			process_name = argv[i + 1];

			i++;
		}

		else if (strcmp(argv[i], "-c") == 0)
		{
			create_process = TRUE;

			if ((i == argc - 1) || argv[i + 1][0] == '-')
			{
				print_help();
				return -1;
			}

			process_name = argv[i + 1];

			i++;
		}

		else if (strcmp(argv[i], "-d") == 0)
		{
			if ((i == argc - 1) || argv[i + 1][0] == '-')
			{
				print_help();
				return -1;
			}

			dll_to_inject = argv[i + 1];

			i++;

		}

		else if (strcmp(argv[i], "-l") == 0)
		{
			injector.list_processes();
			return 0;
		}

		else
		{
			print_help();
			return -1;
		}
	}
	
	if ((!create_process && !inject_into_process) || (create_process && inject_into_process))
	{
		print_help();
		return -1;
	}

	if (dll_to_inject.size() == 0)
	{
		print_help();
		return -1;
	}

	if (create_process)
	{
		injector.create_process_and_inject(process_name, dll_to_inject);
	}
	else if (inject_into_process)
	{
		injector.inject_to_process(process_name, dll_to_inject);
	}

}

void print_help()
{
	fprintf(stdout, "<====== PROCESS INJECTOR ======>\n");
	fprintf(stdout, "-p <process_name>: inject dll to an existing process.\n");
	fprintf(stdout, "-c <path_to_file>: create process and inject dll.\n");
	fprintf(stdout, "-d <path_to_dll>: dll to inject.\n");
	fprintf(stdout, "-l: list processes.\n");
	fprintf(stdout, "-h: show this help.\n");
	fprintf(stdout, "\n\n");
}