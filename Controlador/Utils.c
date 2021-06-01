
#include <tchar.h>

#include "Constantes.h"
#include "Utils.h"

void debug(TCHAR* string) {
#ifdef DEBUG
	_tprintf(L"[DEBUG] %s\n", string);
#endif
}

void erro(TCHAR* string) {
	_ftprintf(stderr, L"[ERRO] %s\n", string);
	//_tprintf(L"Pressione [ENTER] para avançar...\n");
	//(void) _gettchar();
}

void fatal(TCHAR* string) {
	_tprintf(L"[FATAL] %s\n", string);
	_tprintf(L"Pressione [ENTER] para terminar...\n");
	(void) _gettchar();
}

void fim(TCHAR* string) {
	_tprintf(L"[FINAL] %s\n", string);
	_tprintf(L"Pressione [ENTER] para terminar...\n");
	(void)_gettchar();
}