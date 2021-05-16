
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "../Aviao/Aviao.h"
#include "Controlador.h"
#include "Aviao.h"
#include "Aeroporto.h"
#include "Utils.h"

void WINAPI threadControloBuffer(LPVOID lpParam) {

	infoControlador* dados = (infoControlador*)lpParam;

	controloBufferCirc* bufCirc = dados->bufCirc;
	listaAviao* listaAvioes = dados->listaAvioes;
	aeroporto* listaAeroportos = dados->listaAeroportos;
	int tamAvioes = dados->tamAvioes;
	int tamAeroportos = dados->tamAeroporto;

	int pos = 0;
	aviao aux;
	while (!*(dados->terminaControlador)) {
		// Ler o buffer Circular
		if (WaitForSingleObject(bufCirc->hSemItens, 5000) == WAIT_OBJECT_0) {
			aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
			bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF;

			// Regista aviao OU obtem a sua pos no array
			EnterCriticalSection(&dados->criticalSectionControl);
			if (isNovoAviao(aux, listaAvioes, tamAvioes)) {
				pos = getPrimeiraPosVazia(listaAvioes, tamAvioes);
				if (pos > -1) {
					debug(L"Novo aviao");
					listaAvioes[pos].av = aux;
					listaAvioes[pos].isAlive = TRUE;
					listaAvioes[pos].isFree = FALSE;
					if (*dados->suspendeNovosAvioes) {
						debug(L"Comunicação suspensa!");
						listaAvioes[pos].isFree = TRUE;
						listaAvioes[pos].av.terminaExecucao = 1;
					}
					if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
						debug(L"Nao foi possivel abrir a mem do av");
						listaAvioes[pos].isFree = TRUE;
					}
				}
			}
			else {
				pos = getIndiceAviao(aux, listaAvioes, tamAvioes);
				if (pos > -1) {
					listaAvioes[pos].av = aux;
					listaAvioes[pos].isAlive = TRUE;
				}
			}
			LeaveCriticalSection(&dados->criticalSectionControl);

			// Trata dados avioes
			if (listaAvioes[pos].av.terminaExecucao == FALSE) {
				// Preenche coordenadas origem
				if (listaAvioes[pos].av.atuais.posX == -1 && listaAvioes[pos].av.atuais.posY == -1) {
					listaAvioes[pos].av.atuais = obterCoordenadas(listaAvioes[pos].av.aeroOrigem, listaAeroportos, tamAeroportos);
					listaAvioes[pos].av.proxCoord.posX = -1;
					listaAvioes[pos].av.proxCoord.posY = -1;
				}

				if (!_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio")) {
					listaAvioes[pos].av.destino.posX = -1;
					listaAvioes[pos].av.destino.posY = -1;
				}

				// Preenche destino
				if (_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio")) {
					listaAvioes[pos].av.destino = obterCoordenadas(listaAvioes[pos].av.aeroDestino, listaAeroportos, tamAeroportos);
					if (listaAvioes[pos].av.destino.posX == -2 && listaAvioes[pos].av.destino.posY == -2)
						_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
				}

				// Verifica disponibilidade espaco aerio
				if (listaAvioes[pos].av.emViagem == TRUE)
					if (listaAvioes[pos].av.proxCoord.posX > -1 && listaAvioes[pos].av.proxCoord.posY > -1) {
						switch (verificaAvioesPosicao(listaAvioes[pos].av, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
						case 0:		// Pode avancar
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							break;
						case 1:		// Esta no aeroporto destino
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							_tcscpy_s(listaAvioes[pos].av.aeroOrigem, STR_TAM, listaAvioes[pos].av.aeroDestino);
							_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
							break;
						case 2:		// Posicao esta ocupada
							listaAvioes[pos].av.isSobreposto = TRUE;
							break;
						}
					}
			}

			// Enviar mensagem ao aviao
			*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
			SetEvent(listaAvioes[pos].memAviao.hEvento);
		}
	}

	debug(L"Terminei - ThreadControlador");
}

void WINAPI threadTimer(LPVOID lpParam) {
	infoControlador* dados = (infoControlador*)lpParam;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;
	
	while (!*(dados->terminaControlador)) {
		Sleep(3000);
		EnterCriticalSection(&dados->criticalSectionControl);
#ifdef TESTES
		for (int i = 1; i < dados->tamAvioes; i++) {
#else
		for (int i = 0; i < dados->tamAvioes; i++) {
#endif
			if (listaAvioes[i].isAlive) {
				_tprintf(L"Aviao: [%i] está vivo !\n", listaAvioes[i].av.procID);
				listaAvioes[i].isAlive = FALSE;
			}
			else {
				if (!listaAvioes[i].isFree) {
					listaAvioes[i].isFree = TRUE;
					encerraMemoriaPartilhada(&listaAvioes[i].memAviao);
				}
			}
		}
		LeaveCriticalSection(&dados->criticalSectionControl);
	}

	debug(L"Terminei - ThreadTimer");
}

void menu(infoControlador* infoControl) {
	int opcao;
	TCHAR comando[STR_TAM], comandoAux[STR_TAM];
	TCHAR* buffer = NULL;
	TCHAR* token = NULL;

	/*
	* aero + nome + coordX + coordY
	* laero = lista aeroportos
	* lavioes = lista avioes
	* susp = suspender comunicações
	* ret = retomar comunicações
	* end = terminar controlador
	*/

	while (!*(infoControl->terminaControlador)) {
		_tprintf(L"\n\nInsira [cmd] para ver os comandos disponiveis\n");
		_tprintf(L" > ");
		_fgetts(comando, STR_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0';

		_tcscpy_s(&comandoAux, STR_TAM, comando);
		token = _tcstok_s(comando, L" ", &buffer);
		if (token != NULL) {
			if (!_tcscmp(token, L"aero")) {
				if (!adicionaAeroporto(infoControl->listaAeroportos, &infoControl->indiceAero, comandoAux))
					_tprintf(L"\nNão foi possivel adicionar o aeroporto à lista\n\n");
				else
					_tprintf(L"\nAeroporto adicionado à lista de aeroportos\n\n");
			}
			if (!_tcscmp(token, L"laero")) {
				_tprintf(L"Lista de aeroportos:\n");
				imprimeListaAeroporto(infoControl->listaAeroportos, infoControl->indiceAero);
				_tprintf(L"\n");
			}
			if (!_tcscmp(token, L"laviao")) {
				_tprintf(L"Lista de avioes:\n");
				imprimeListaAvioes(infoControl->listaAvioes, infoControl->tamAvioes);
				_tprintf(L"\n");
			}
			if (!_tcscmp(token, L"susp")) {
				if (*(infoControl->suspendeNovosAvioes) != 1) {
					*(infoControl->suspendeNovosAvioes) = 1;
					_tprintf(L"Aceitação de novos aviões suspendida!\n");
				}
				else
					erro(L"A aceitação de novos aviões já se encontrava suspensa!");
			}
			if (!_tcscmp(token, L"ret")) {
				*(infoControl->suspendeNovosAvioes) = 0;
				_tprintf(L"Aceitação de novos aviões alterada!\n");
			}
			if (!_tcscmp(token, L"end")) {
				*(infoControl->terminaControlador) = 1;
			}
			if (!_tcscmp(token, L"cmd")) {
				_tprintf(L"aero + nome + coordX + coordY = criar aeroporto\nlaero = lista aeroportos\nlavioes = lista avioes\nsusp = suspender comunicações\n"
					L"ret = retomar comunicações\nend = terminar controlador\ncmd = comandos disponiveis\n\n");
			}
		}
	}

#ifdef TESTES
	for (int i = 1; i < infoControl->tamAvioes; ++i) {
#else
	for (int i = 0; i < infoControl->tamAvioes; ++i) {
#endif
		if (!infoControl->listaAvioes[i].isFree) {
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, infoControl->listaAvioes[i].av.procID);
			encerraMemoriaPartilhada(&infoControl->listaAvioes[i].memAviao);
			TerminateProcess(hProcess, 1);
		}
	}

}