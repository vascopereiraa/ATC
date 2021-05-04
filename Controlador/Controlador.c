
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Aviao.h"
#include "Utils.h"
#include "Controlador.h"

// DEFINES DO REGISTRY
#define N_AVIOES 20

void WINAPI threadControloBuffer(LPVOID lpParam);

int _tmain() {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	// Criar Memoria Partilhada do Controlador
	controloBufferCirc bufCirc;
	if (!criaBufferCircular(&bufCirc))
		return 1;

	// Criar lista de avioes
	// int tamLista = VALOR DO REGISTRY
	listaAviao* listaAvioes = malloc(N_AVIOES * sizeof(listaAviao));
	if (listaAvioes == NULL) {
		fatal(L"Ocorreu um erro ao criar a lista de avioes");
		encerraBufferCircular(&bufCirc);
		return 1;
	}

	// FUNCAO PARA POR O ISFREE A TRUE EM TODOS AO CRIAR - todos livres
	for (int i = 0; i < N_AVIOES; ++i)
		listaAvioes[i].isFree = TRUE;

	// Criar a Thread para gerenciar o buffer circular
	infoControlador infoControl;
	infoControl.bufCirc = &bufCirc;
	infoControl.listaAvioes = listaAvioes;
	infoControl.tamAvioes = N_AVIOES;

	HANDLE hThreadBuffer = CreateThread(NULL, 0, threadControloBuffer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadBuffer == NULL) {
		encerraBufferCircular(&bufCirc);
	}

	WaitForSingleObject(hThreadBuffer, INFINITE);
	CloseHandle(hThreadBuffer);
	free(listaAvioes);
	encerraBufferCircular(&bufCirc);

	return 0;
}

// FUNCAO A PASSAR PARA UM FICHEIRO DE AVIOES
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (av.procID == lista[i].av.procID)
			return FALSE;
	return TRUE;
}

int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (lista[i].isFree)
			return i;
	return -1;
}

void imprimeListaAvioes(listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i) {
		if (lista[i].isFree == FALSE) {
			_tprintf(L"Aviao %d:\n", i + 1);
			_tprintf(L"Nr Avião: %d\n\n", lista[i].av.procID);
		}
	}
}

int getIndiceAviao(aviao aux, listaAviao* listaAvioes, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (aux.procID == listaAvioes[i].av.procID)
			return i;
	return -1;
}

void WINAPI threadControloBuffer(LPVOID lpParam) {

	infoControlador* dados = (infoControlador*)lpParam;
	
	controloBufferCirc* bufCirc = dados->bufCirc;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;

	aviao aux;
	while (1) {
		debug(L"Estou a espera");
		WaitForSingleObject(bufCirc->hSemItens, INFINITE);
		aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
		bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF; 
		if (isNovoAviao(aux, listaAvioes, tamAvioes)) {
			int pos = getPrimeiraPosVazia(listaAvioes, tamAvioes);
			if (pos != -1) {
				listaAvioes[pos].av = aux;
				if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
					debug(L"Nao correu bem");
					// Avisa o aviao e manda fechar
				}
				else {
					listaAvioes[pos].isFree = FALSE;
					listaAvioes[pos].av.atuais.posX = 10;
					*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
					SetEvent(listaAvioes[pos].memAviao.hEvento);
				}
			}
			else {
				erro(L"Nao exitem mais espacos para avioes");
				// Avisa o aviao e manda fechar
			}
		}
		else {
			int pos = getIndiceAviao(aux, listaAvioes, tamAvioes);
			if (pos == -1)
				erro(L"indice -1");
			else {
				
				// Cenas .....
				//Verifica se é aeroporto?
				//	Se é, verifica se é destino
				//		Se sim, fica lá e termina viagem
				//	Senao continua
				// 
				// verifica se ha avioes nessa posicao
				// Se sim, troca de sitio ou espera
				// Senao avança

				if(verificaAvioesPosicao()) { 
					listaAvioes[pos].av = aux;
				}
				*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
				SetEvent(listaAvioes[pos].memAviao.hEvento);
			}
		}
		imprimeListaAvioes(listaAvioes, tamAvioes);
	}

}