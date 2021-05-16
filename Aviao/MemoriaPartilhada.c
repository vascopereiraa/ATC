
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "MemoriaPartilhada.h"
#include "../Controlador/Utils.h"
#include "../Controlador/Constantes.h"

/* FUNCOES PARA CONTROLAR O BUFFER CIRCULAR */

BOOL abreBufferCircular(controloBufferCirc* bufCirc) {
	
	bufCirc->hFileMap = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, SHM_CONTROL);
	if (bufCirc->hFileMap == NULL) {
		fatal(L"Não existe nenhum controlador em execução!\n");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->pBuf = (bufferCircular*)MapViewOfFile(bufCirc->hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(bufferCircular));
	if (bufCirc->pBuf == NULL) {
		fatal(L"Não foi possivel aceder à vista do buffer circular!\n");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->hSemItens = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SEM_ITENS);
	if (bufCirc->hSemItens == NULL) {
		fatal(L"Não foi possivel abrir o semaforo controlador de itens no buffer!\n");
		encerraBufferCircular(bufCirc);
		return FALSE;
	}

	bufCirc->hSemMutexProd = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SEM_MUTEX_PROD);
	if (bufCirc->hSemMutexProd == NULL) {
		fatal(L"Não foi possivel abrir o semaforo binario controlador de produtores!\n");
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

BOOL criaMemoriaPartilhada(memoriaPartilhada* memPartilhada) {
	
	DWORD procID = GetCurrentProcessId();

	TCHAR nomeMem[STR_TAM];
	_stprintf_s(nomeMem, STR_TAM, SHM_AVIAO, procID);
	_tprintf(L"\n\nNome da memoria partilhada: %s\n\n", nomeMem);
	memPartilhada->hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(aviao), nomeMem);
	if (memPartilhada->hFileMap == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
		fatal(L"Nao foi possivel criar a memoria partilhada do avião");
		encerraMemoriaPartilhada(memPartilhada);
		return FALSE;
	}

	memPartilhada->pAviao = (aviao*)MapViewOfFile(memPartilhada->hFileMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(aviao));
	if (memPartilhada->pAviao == NULL) {
		fatal(L"Nao foi possivel mapear o avião em memória");
		encerraMemoriaPartilhada(memPartilhada);
		return FALSE;
	}

	TCHAR nomeEvento[STR_TAM];
	_stprintf_s(nomeEvento, STR_TAM, EVNT_AVIAO, procID);
	memPartilhada->hEvento = CreateEvent(NULL, TRUE, FALSE, nomeEvento);
	if (memPartilhada->hEvento == NULL) {
		fatal(L"Nao foi possivel criar o evento do avião");
		encerraMemoriaPartilhada(memPartilhada);
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

void criaCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao) {
	InitializeCriticalSection(lpCriticalSectionAviao);
}

void encerraCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao) {
	DeleteCriticalSection(lpCriticalSectionAviao);
}