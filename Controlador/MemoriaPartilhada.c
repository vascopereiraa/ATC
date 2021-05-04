
#include <tchar.h>

#include "Utils.h"
#include "MemoriaParilhada.h"

/* FUNCOES PARA CONTROLAR O BUFFER CIRCULAR */

BOOL criaBufferCircular(controloBufferCirc* bufCirc) {

	bufCirc->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(bufferCircular), SHM_CONTROL);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		fatal(L"Já existe um controlador em execução!");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}
	if (bufCirc->hFileMap == NULL) {
		fatal(L"Ocorrreu um erro ao criar a memoria partilhada!");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->pBuf = (bufferCircular*)MapViewOfFile(bufCirc->hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(bufferCircular));
	if (bufCirc->pBuf == NULL) {
		fatal(L"Ocorrreu um erro ao criar a vista!");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->hSemItens = CreateSemaphore(NULL, 0, MAX_BUF, SEM_ITENS);
	if (bufCirc->hSemItens == NULL) {
		fatal(L"Ocorrreu um erro ao criar o semaforo de itens!");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->hSemMutexProd = CreateSemaphore(NULL, 1, 1, SEM_MUTEX_PROD);
	if (bufCirc->hSemMutexProd == NULL) {
		fatal(L"Ocorrreu um erro ao criar o semaforo binário de produtores!");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	return TRUE;
}

void encerraBufferCircular(controloBufferCirc* controlo) {
	
	if (controlo->pBuf != NULL)
		UnmapViewOfFile(controlo->pBuf);
	
	if (controlo->hFileMap != NULL)
		CloseHandle(controlo->hFileMap);

	if (controlo->hSemItens != NULL)
		CloseHandle(controlo->hSemItens);

	if (controlo->hSemMutexProd != NULL)
		CloseHandle(controlo->hSemMutexProd);

}

/* FUNCOES PARA CONTROLAR A MEMORIA PARTILHADA DO AVIAO */

BOOL abreMemoriaPartilhada(memoriaPartilhada* memPart) {

}