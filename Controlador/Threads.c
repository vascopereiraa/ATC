
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
	while (1) {
		debug(L"Estou a espera");
		if (WaitForSingleObject(bufCirc->hSemItens, 5000) == WAIT_OBJECT_0) {
			aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
			bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF;
			if (isNovoAviao(aux, listaAvioes, tamAvioes)) {
					pos = getPrimeiraPosVazia(listaAvioes, tamAvioes);
					if (pos != -1) {
						listaAvioes[pos].av = aux;
						if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
							debug(L"Nao correu bem");
							aux.terminaExecucao = TRUE;
							SetEvent(listaAvioes[pos].memAviao.hEvento);
							// Avisa o aviao e manda fechar
						}
						else {
							if (*(dados->suspendeNovosAvioes)) {
								debug(L"O avião não pode entrar ! Comunicação suspensa");
								listaAvioes[pos].isFree = TRUE;
								aux.terminaExecucao = TRUE;
							}
							else {
								listaAvioes[pos].isFree = FALSE;
								listaAvioes[pos].isAlive = TRUE;
								//Ao registar o aviao, verifica se existe aeroporto de origem e destino
								if (!obterCoordenadasOrigemDestino(&aux, listaAeroportos, tamAeroportos)) {
									//listaAvioes[pos].av.terminaExecucao = TRUE;
									aux.terminaExecucao = TRUE;
									_tprintf(L"Não existem o aeroporto de origem ou destino\n");
								}
								else {
									_tprintf(L"Aero origem x: [%i] y: [%i] \t Aero destino x: [%i] y: [%i]\n", aux.atuais.posX, aux.atuais.posY, aux.destino.posX, aux.destino.posY);
								}
							}
							listaAvioes[pos].av = aux;
							*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
							SetEvent(listaAvioes[pos].memAviao.hEvento);
						}
					}
					else {
						erro(L"Nao exitem mais espacos para avioes");
						aux.terminaExecucao = TRUE;
						SetEvent(listaAvioes[pos].memAviao.hEvento);
						// Avisa o aviao e manda fechar
					}
			}
			else {
				pos = getIndiceAviao(aux, listaAvioes, tamAvioes);
				if (pos == -1)
					erro(L"indice -1");
				else {
					listaAvioes[pos].isAlive = TRUE;
					if (!verificaAvioesPosicao(aux, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
						debug(L"Pode avancar da posição");
						aux.atuais = aux.proxCoord;
					}
					else {
						// DEBUG 
						listaAvioes[0].av.atuais.posX = 8;
						listaAvioes[0].av.atuais.posY = 8;
						debug(L"Não pode avancar!! Espera!");
					}
					listaAvioes[pos].av = aux;
					*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
					SetEvent(listaAvioes[pos].memAviao.hEvento);
				}
			}
			imprimeListaAvioes(listaAvioes, tamAvioes);
		}
	}
}

void WINAPI threadTimer(LPVOID lpParam) {
	infoControlador* dados = (infoControlador*)lpParam;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;
	while (!*(dados->terminaControlador)) {
		Sleep(3000);
		for (int i = 0; i < dados->tamAvioes; i++) {
			if (listaAvioes[i].isAlive) {
				//debug(L"Estou vivo!");
				_tprintf(L"Aviao: [%id] está vivo !\n", listaAvioes[i].av.procID);
				listaAvioes[i].isAlive = FALSE;
			}
			else
				listaAvioes[i].isFree = TRUE;
		}
	}
}