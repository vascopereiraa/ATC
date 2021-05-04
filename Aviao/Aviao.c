
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "MemoriaPartilhada.h"
#include "../Controlador/Utils.h"

int _tmain() {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	_tprintf(L"Sou o Aviao!\n");

	// Abrir Memoria Partilhada do Controlador
	controloBufferCirc bufCirc;
	if (!abreBufferCircular(&bufCirc))
		return 1;

	// Cria memoria partilhada do Avião
	memoriaPartilhada memPart;
	if (!criaMemoriaPartilhada(&memPart)) {
		encerraBufferCircular(&bufCirc);
		return 1;
	}

	aviao av;
	av.procID = GetCurrentProcessId();
	av.terminaExecucao = FALSE;
	_tprintf(L"%d\n", av.procID);

	while (1) {
		WaitForSingleObject(bufCirc.hSemMutexProd, INFINITE);
		bufCirc.pBuf->buf[bufCirc.pBuf->numProd] = av;
		bufCirc.pBuf->numProd = (bufCirc.pBuf->numProd + 1) % MAX_BUF;
		ReleaseSemaphore(bufCirc.hSemMutexProd, 1, NULL);
		ReleaseSemaphore(bufCirc.hSemItens, 1, NULL);
		Sleep(2000);

		WaitForSingleObject(memPart.hEvento, INFINITE);
		_tprintf(L"%d\n", memPart.pAviao->atuais.posX);
		ResetEvent(memPart.hEvento);
	}

	encerraBufferCircular(&bufCirc);
	return 0;
}
