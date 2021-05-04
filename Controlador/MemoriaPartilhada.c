
#include <tchar.h>

#include "Utils.h"
#include "Controlador.h"

/* FUNCOES PARA CONTROLAR O BUFFER CIRCULAR */

BOOL criaBufferCircular(controloBufferCirc* bufCirc) {

	bufCirc->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(bufferCircular), SHM_CONTROL);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		fatal(L"J� existe um controlador em execu��o!");
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
		fatal(L"Ocorrreu um erro ao criar o semaforo bin�rio de produtores!");
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

BOOL abreMemoriaPartilhada(listaAviao* av) {

	TCHAR nomeSHM[STR_TAM];
	_stprintf_s(nomeSHM, STR_TAM, SHM_AVIAO, av->av.procID);
	av->memAviao.hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, nomeSHM);
	if (av->memAviao.hFileMap == NULL) {
		erro(L"Nao foi possivel abrir a posicao de memoria do avi�o");
		encerraMemoriaPartilhada(&av->memAviao);
		return FALSE;
	}

	av->memAviao.pAviao = (aviao*) MapViewOfFile(av->memAviao.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(aviao));
	if (av->memAviao.pAviao == NULL) {
		erro(L"Nao foi possivel mapear a zona de memoria do avi�o");
		encerraMemoriaPartilhada(&av->memAviao);
		return FALSE;
	}

	TCHAR nomeEvnt[STR_TAM];
	_stprintf_s(nomeEvnt, STR_TAM, EVNT_AVIAO, av->av.procID);
	av->memAviao.hEvento = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, nomeEvnt);
	if (av->memAviao.hEvento == NULL) {
		erro(L"Nao foi possivel abrir o evento de sync com o avi�o");
		encerraMemoriaPartilhada(&av->memAviao);
		return FALSE;
	}

	return TRUE;
}

void encerraMemoriaPartilhada(memoriaPartilhada* memPart) {
	
	if (memPart->pAviao != NULL)
		UnmapViewOfFile(memPart->pAviao);

	if (memPart->hFileMap != NULL)
		CloseHandle(memPart->hFileMap);

	if (memPart->hEvento != NULL)
		CloseHandle(memPart->hEvento);

}