
#include <tchar.h>

#include "Utils.h"
#include "Aviao.h"
#include "Controlador.h"

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

BOOL abreMemoriaPartilhada(listaAviao* av) {

	TCHAR nomeSHM[STR_TAM];
	_stprintf_s(nomeSHM, STR_TAM, SHM_AVIAO, av->av.procID);
	_tprintf(L"\n\nNome da memoria partilhada: %s\n\n", nomeSHM);
	av->memAviao.hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, nomeSHM);
	if (av->memAviao.hFileMap == NULL) {
		erro(L"Nao foi possivel abrir a posicao de memoria do avião");
		encerraMemoriaPartilhada(&av->memAviao);
		return FALSE;
	}

	av->memAviao.pAviao = (aviao*) MapViewOfFile(av->memAviao.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(aviao));
	if (av->memAviao.pAviao == NULL) {
		erro(L"Nao foi possivel mapear a zona de memoria do avião");
		encerraMemoriaPartilhada(&av->memAviao);
		return FALSE;
	}

	TCHAR nomeEvnt[STR_TAM];
	_stprintf_s(nomeEvnt, STR_TAM, EVNT_AVIAO, av->av.procID);
	av->memAviao.hEvento = OpenEvent(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, nomeEvnt);
	if (av->memAviao.hEvento == NULL) {
		erro(L"Nao foi possivel abrir o evento de sync com o avião");
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

// Seccao Critica de sincronizacao entre threads e menu
void criaCriticalSectionControl(LPCRITICAL_SECTION lpCriticalSectionControl) {
	InitializeCriticalSection(lpCriticalSectionControl);
}

void encerraCriticalSectionControl(LPCRITICAL_SECTION lpCriticalSectionControl) {
	DeleteCriticalSection(lpCriticalSectionControl);
}

// Termina a execução do controlador
void encerraControlador(infoControlador* infoControl) {
	if (infoControl->bufCirc != NULL)
		encerraBufferCircular(infoControl->bufCirc);

	if (infoControl->infoPassagPipes != NULL)
		free(infoControl->infoPassagPipes);

	if (infoControl->listaAvioes != NULL)
		free(infoControl->listaAvioes);

	if (infoControl->listaAeroportos != NULL)
		free(infoControl->listaAeroportos);

	if (infoControl->terminaControlador != NULL)
		free(infoControl->terminaControlador);

	if (infoControl->suspendeNovosAvioes != NULL)
		free(infoControl->suspendeNovosAvioes);
}