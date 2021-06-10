
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
			_tprintf(L"Capacidade: %d\tVelocidade: %d\n", lista[i].av.capMaxima, lista[i].av.velocidade);
			_tprintf(L"Origem: %s\n", lista[i].av.aeroOrigem);
			_tprintf(L"x = %d\ty = %d\n", lista[i].av.atuais.posX, lista[i].av.atuais.posY);
			if (!_tcscmp(lista[i].av.aeroDestino, L"vazio"))
				_tprintf(L"Destino: SEM DESTINO\n");
			else
				_tprintf(L"Destino: %s\n", lista[i].av.aeroDestino);
			_tprintf(L"x = %d\ty = %d\n", lista[i].av.destino.posX, lista[i].av.destino.posY);
			_tprintf(L"Prox coord: x = %d\t y = %d\n", lista[i].av.proxCoord.posX, lista[i].av.proxCoord.posY);
			_tprintf(L"IsSobreposto = %d\n", lista[i].av.isSobreposto);
			_tprintf(L"\n");
		}
	}
}

int getIndiceAviao(aviao aux, listaAviao* listaAvioes, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (aux.procID == listaAvioes[i].av.procID)
			return i;
	return -1;
}

int verificaAvioesPosicao(aviao aux, aeroporto* listaAeroportos, int tamAeroportos, listaAviao* listaAvioes, int tamAvioes) {

	//ProxCoord ? Assumindo que a 1º posição está correta devido a verificacao com obterCoordenadasOrigemDestino
	for (int i = 0; i < tamAeroportos; i++) {
		if (listaAeroportos[i].localizacao.posX == aux.proxCoord.posX && aux.destino.posX == aux.proxCoord.posX
			&& listaAeroportos[i].localizacao.posY == aux.proxCoord.posY && aux.destino.posY == aux.proxCoord.posY) {
			_tprintf(L"\n\nEstá no aeroporto de destino %d !\n\n", aux.procID);
			return 1;
		}
	}

	for (int i = 0; i < tamAvioes; i++) {
		if (listaAvioes[i].av.atuais.posX == aux.proxCoord.posX &&
			listaAvioes[i].av.atuais.posY == aux.proxCoord.posY &&
			listaAvioes->isFree == FALSE && listaAvioes[i].av.procID != aux.procID) {
			_tprintf(L"\n\nO avião %d encontrou o aviao %d no meio do ar !\n\n", aux.procID, listaAvioes[i].av.procID);
			return 2;
		}
	}
	debug(L"Pode avançar");
	return 0;
}

coordenadas obterCoordenadas(TCHAR* string, aeroporto* listaAeroportos, int tamAeroportos) {
	coordenadas pos = { -2, -2 };
	for (int i = 0; i < tamAeroportos; ++i)
		if (!_tcscmp(string, listaAeroportos[i].nome)) {
			pos = listaAeroportos[i].localizacao;
			return pos;
		}
	return pos;
}

TCHAR* listaAv(const listaAviao* lista, const int tamLista) {
	TCHAR lstAux[500] = _TEXT(" ");
	TCHAR lstAv[7000] = _TEXT(" ");
	for (int i = 0; i < tamLista - 1; ++i) {
		if (!lista[i].isFree) {
			_stprintf_s(lstAux, 500, L"ID: %d Aero Origem: %s Aero Destino: %s Velocidade %d\n",
				lista[i].av.procID, lista[i].av.aeroOrigem, lista[i].av.aeroDestino, lista[i].av.velocidade);
			_tcscat_s(lstAv, 7000, lstAux);

		}
	}
	return lstAv;
}