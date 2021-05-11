
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Controlador.h"
#include "Aeroporto.h"
#include "Aviao.h"
#include "Utils.h"

listaAviao* inicializaListaAviao(int tamAvioes) {
	listaAviao* lista = (listaAviao*) malloc(tamAvioes * sizeof(listaAviao));
	if (lista == NULL) {
		fatal(L"Ocorreu um erro ao criar a lista de avioes");
		return NULL;
	}

	// Coloca todas as posicoes livres no array
	for (int i = 0; i < tamAvioes; ++i) {
		lista[i].isFree = 1;
		lista[i].isAlive = 0;
	}
	return lista;
}

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

BOOL verificaAvioesPosicao(aviao aux, aeroporto* listaAeroportos, int tamAeroportos, listaAviao* listaAvioes, int tamAvioes) {

	//ProxCoord ? Assumindo que a 1º posição está correta devido a verificacao com obterCoordenadasOrigemDestino
	for (int i = 0; i < tamAeroportos; i++) {
		/*if (listaAeroportos[i].localizacao.posX == aux.proxCoord.posX &&
			listaAeroportos[i].localizacao.posY == aux.proxCoord.posY) {
			_tprintf(L"\n\nEstá no aeroporto de origem !\n\n");
			return FALSE;
		}*/
		if (listaAeroportos[i].localizacao.posX == aux.proxCoord.posX && aux.destino.posX == aux.proxCoord.posX
			&& listaAeroportos[i].localizacao.posY == aux.proxCoord.posY && aux.destino.posY == aux.proxCoord.posY) {
			_tprintf(L"\n\nEstá no aeroporto de destino !\n\n");
			return FALSE;
		}
	}

	for (int i = 0; i < tamAvioes; i++) {
		if (listaAvioes[i].av.atuais.posX == aux.proxCoord.posX &&
			listaAvioes[i].av.atuais.posY == aux.proxCoord.posY &&
			listaAvioes->isFree == FALSE && listaAvioes[i].av.procID != aux.procID) {
			_tprintf(L"\n\nTem avião no mesmo sitio !\n\n");
			return TRUE;
		}
	}
	_tprintf(L"\n\nPode avançar\n\n");
	return FALSE;
}

BOOL obterCoordenadasOrigemDestino(aviao * aux, aeroporto * listaAeroportos, int tamAeroportos) {
	int contador = 0;
	for (int i = 0; i < tamAeroportos; i++) {
		if (_tcscmp(listaAeroportos[i].nome, aux->aeroOrigem) == 0) {
			aux->atuais.posX = listaAeroportos[i].localizacao.posX;
			aux->atuais.posY = listaAeroportos[i].localizacao.posY;
			_tprintf(L"Destino x: [%i] y: [%i]", aux->atuais.posX, aux->atuais.posY);
			_tprintf(L"\nAtribuido aeroporto origem [%s] !\n", listaAeroportos[i].nome);
			++contador;
		}
		if (_tcscmp(listaAeroportos[i].nome, aux->aeroDestino) == 0) {
			aux->destino.posX = listaAeroportos[i].localizacao.posX;
			aux->destino.posY = listaAeroportos[i].localizacao.posY;
			_tprintf(L"Destino x: [%i] y: [%i]", aux->destino.posX, aux->destino.posY);
			_tprintf(L"\nEstá no aeroporto de destino [%s] !\n", listaAeroportos[i].nome);
			++contador;
		}
		if (contador == 2)
			return TRUE;
	}
	return FALSE;
}
