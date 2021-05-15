
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
		WaitForSingleObject(bufCirc->hSemItens, INFINITE);
		aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
		bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF;

		// Regista aviao OU obtem a sua pos no array
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
					listaAvioes[pos].av.terminaExecucao = TRUE;
				}
				if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
					debug(L"Nao foi possivel abrir a mem do av");
					listaAvioes[pos].av.terminaExecucao = TRUE;
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

		// Trata dados avioes
		_tprintf(L"\n");
		_tprintf(L"Avião nr: %d\n", listaAvioes[pos].av.procID);
		_tprintf(L"Capacidade: %d\tVelocidade: %d\n", listaAvioes[pos].av.capMaxima, listaAvioes[pos].av.velocidade);
		_tprintf(L"Origem: %s\n", listaAvioes[pos].av.aeroOrigem);
		_tprintf(L"x = %d\ty = %d\n", listaAvioes[pos].av.atuais.posX, listaAvioes[pos].av.atuais.posY);
		if (!_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio"))
			_tprintf(L"Destino: SEM DESTINO\n");
		else
			_tprintf(L"Destino: %s\n", listaAvioes[pos].av.aeroDestino);
		_tprintf(L"x = %d\ty = %d\n", listaAvioes[pos].av.destino.posX, listaAvioes[pos].av.destino.posY);
		_tprintf(L"\n");

		if (listaAvioes[pos].av.terminaExecucao == FALSE) {
			// Preenche coordenadas origem
			if (listaAvioes[pos].av.atuais.posX == -1 && listaAvioes[pos].av.atuais.posY == -1) {
				listaAvioes[pos].av.atuais = obterCoordenadas(listaAvioes[pos].av.aeroOrigem, listaAeroportos, tamAeroportos);
			}
			// Preenche destino
			if (_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio")) {
				listaAvioes[pos].av.destino = obterCoordenadas(listaAvioes[pos].av.aeroDestino, listaAeroportos, tamAeroportos);
				if (listaAvioes[pos].av.destino.posX == -1)
					_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
			}

			// Verifica disponibilidade espaco aerio
			if (listaAvioes[pos].av.emViagem == TRUE)
				switch (verificaAvioesPosicao(listaAvioes[pos].av, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
				case 0:		// Pode avancar
					listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
					listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
					break;
				case 1:		// Esta no aeroporto destino
					_tcscpy_s(listaAvioes[pos].av.aeroOrigem, STR_TAM, listaAvioes[pos].av.aeroDestino);
					_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
					listaAvioes[pos].av.destino.posX = -1;
					listaAvioes[pos].av.destino.posY = -1;
					listaAvioes[pos].av.proxCoord.posX = -1;
					listaAvioes[pos].av.proxCoord.posY = -1;
					break;
				case 2:		// Posicao esta ocupada
					listaAvioes[pos].av.proxCoord.posX = -2;
					listaAvioes[pos].av.proxCoord.posY = -2;
					break;
				}
		}

		// Enviar mensagem ao aviao
		*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
		SetEvent(listaAvioes[pos].memAviao.hEvento);
		Sleep(2000);
	}
	debug(L"Passei aqui");
}

void WINAPI threadTimer(LPVOID lpParam) {
	infoControlador* dados = (infoControlador*)lpParam;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;
	
	while (!*(dados->terminaControlador)) {
		Sleep(3000);
		for (int i = 0; i < dados->tamAvioes; i++) {
			if (listaAvioes[i].isAlive) {
				debug(L"Estou vivo!");
				_tprintf(L"Aviao: [%i] está vivo !\n", listaAvioes[i].av.procID);
				listaAvioes[i].isAlive = FALSE;
			}
			else
				listaAvioes[i].isFree = TRUE;
		}
	}
}