
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Comunicacao.h"
#include "Aviao.h"
#include "MemoriaPartilhada.h"
#include "../Controlador/Constantes.h"

fcnDLL carregaDLL(TCHAR* dllLocation) {

	SYSTEM_INFO systemInfo;

	GetSystemInfo(&systemInfo);
	switch (systemInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		_stprintf_s(dllLocation, STR_TAM, DLL_LOCATION, L"x64");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		_stprintf_s(dllLocation, STR_TAM, DLL_LOCATION, L"x86");
		break;
	default:
		fatal(L"Nao foi possivel carregar a DLL");
		return NULL;
	}

	HMODULE hLib = LoadLibrary(dllLocation);
	fcnDLL move = NULL;
	if (hLib == NULL)
		return NULL;
	move = (fcnDLL)GetProcAddress(hLib, "move");
	return move;
}

void libertaDLL(TCHAR* dllLocation) {
	HMODULE hDLL = GetModuleHandle(dllLocation);
	if (hDLL != NULL)
		FreeLibrary(hDLL);
}


DWORD WINAPI threadViagem(LPVOID lpParam) {
	TCHAR dllLocation[STR_TAM];

	infoAviao* dados = (infoAviao*)lpParam;
	controloBufferCirc* bufCirc = &dados->bufCirc;
	memoriaPartilhada* memPart = &dados->memPart;
	aviao* av = &dados->av;

	// Carregar a funcao da DLL
	fcnDLL move = carregaDLL(dllLocation);
	if (move == NULL) {
		// Erro ao carregar a DLL
		dados->terminaAviao = TRUE;
		return 1;
	}

	int resultado = 0, nextPos = 1;
	while (!dados->terminaAviao) {
		// debug(L"Estou a enviar cenas")
		if (av->emViagem) {
			if (nextPos) {
				// Calcula proxima posicao -> FUNCAO DLL
				EnterCriticalSection(&dados->criticalSectionAviao);
				resultado = move((*av).atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY,
					&av->proxCoord.posX, &av->proxCoord.posY);
				switch (resultado) {
				case 0:			// Chegou ao destino
					av->emViagem = FALSE;
					av->destino.posX = -1;
					av->destino.posY = -1;
					_tprintf(L"\nAviao chegou ao destino!\n");
					break;
				case 1:
					debug(L"O aviao executou uma movimentação correta");
					break;
				case 2:
					debug(L"Ocorreu um erro ao movimentar-se");
					libertaDLL(dllLocation);
					LeaveCriticalSection(&dados->criticalSectionAviao);
					return 2;
					break;
				}
			}
#ifdef DEBUG
			Sleep(3000 / av->velocidade);   // Aplica velocidade mais lenta para maior visibilidade de outputs
			_tprintf(L"\nAtual x: [%i] Atual y: [%i]\tDestino x: [%i] Destino y: [%i]\tProxima x: [%i] Proxima y: [%i]\n",
				av->atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY, av->proxCoord.posX, av->proxCoord.posY);
#else
			Sleep(1000 / av->velocidade);	// Aplica a velocidade do aviao em espacos / segundo
#endif
			LeaveCriticalSection(&dados->criticalSectionAviao);
		}
		else
			Sleep(2000); // Para enviar mensagem a indicar que está vivo a cada 2seg.

#ifdef DEBUG
		if (av->emViagem) {
			//Sleep(1500);
			EnterCriticalSection(&dados->criticalSectionAviao);
			debug(L"THREAD:");
			imprimeDadosAviao(&dados->av);
			LeaveCriticalSection(&dados->criticalSectionAviao);
		}
#endif

		// Escrever no buffer circular
		WaitForSingleObject(bufCirc->hSemMutexProd, INFINITE);

		EnterCriticalSection(&dados->criticalSectionAviao);
		bufCirc->pBuf->buf[bufCirc->pBuf->numProd] = *av;
		bufCirc->pBuf->numProd = (bufCirc->pBuf->numProd + 1) % MAX_BUF;

		ReleaseSemaphore(bufCirc->hSemMutexProd, 1, NULL);
		ReleaseSemaphore(bufCirc->hSemItens, 1, NULL);
		// WaitForSingleObject(memPart->hEvento, INFINITE);

		// Ler e tratar a resposta do controlador
		if (WaitForSingleObject(memPart->hEvento, 7000) == WAIT_TIMEOUT) {
			// Caso não obtenha resposta do controlador >> Contrl nao abriu memPart
			erro(L"O Controlador nao conseguiu comunicar de volta!");
			LeaveCriticalSection(&dados->criticalSectionAviao);
			libertaDLL(dllLocation);
			return 3;
		}

		CopyMemory(av, memPart->pAviao, sizeof(aviao));

		// Verifica se tem de encerrar
		if (av->terminaExecucao) {
			debug(L"Controlador mandou o aviao encerrar");
			_tprintf(L"Aceitação de novos aviões suspensa no controlador\n");
			libertaDLL(dllLocation);
			LeaveCriticalSection(&dados->criticalSectionAviao);
			return 4;
		}

		// Verifica se a movimentação foi feita com sucesso
		if (av->emViagem) {
			nextPos = 1;
			if (av->isSobreposto) {
				av->isSobreposto = FALSE;
				debug(L"A posicao pretendida encontrava-se ocupada!");
				int random = rand() % 101;
				if (random < 50) {
					debug(L"Vou esperar 1 segundos para evitar colisão!");
					Sleep(1000);
				}
				else {
					if (av->proxCoord.posX + 1 < 1000) {
						debug(L"Vou virar a direita para evitar colisão!");
						av->proxCoord.posX += 1;
					}
					nextPos = 0;
				}
			}
		}
		else {
			// O aeroporto origem não existe
			if (av->atuais.posX == -2 && av->atuais.posY == -2) {
				erro(L"O aeroporto origem nao existe!");
				dados->terminaAviao = TRUE;
				libertaDLL(dllLocation);
				LeaveCriticalSection(&dados->criticalSectionAviao);
				return 5;
			}
			if (av->destino.posX == -2 && av->destino.posY == -2) {
				erro(L"O aeroporto destino nao existe!");
			}
		}
		LeaveCriticalSection(&dados->criticalSectionAviao);
		ResetEvent(memPart->hEvento);
	}
	libertaDLL(dllLocation);
	return 0;
}