#pragma once

#ifndef COMMON_H
#define COMMON_H

#pragma comment(lib, "Shlwapi.lib")

#ifdef UNICODE
#undef UNICODE
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>

#include <string>
#include <iostream>
#include <memory>
#include <cctype>

#endif // !COMMON_H
