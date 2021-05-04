
#include <tchar.h>

#include "Utils.h"

void debug(TCHAR* string) {
	_tprintf(L"[DEBUG] %s\n", string);
}

void erro(TCHAR* string) {
	_ftprintf(stderr, L"[ERRO] %s\n", string);
	_gettchar();
}

void fatal(TCHAR* string) {
	_tprintf(L"[FATAL] %s\n", string);
	_gettchar();
}