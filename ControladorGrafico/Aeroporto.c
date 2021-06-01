
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Aviao.h"
#include "Aeroporto.h"
#include "Controlador.h"
#include "Utils.h"

BOOL verificaNomeAero(TCHAR* nome, const aeroporto* lista, const int* indiceAero) {
	for (int i = 0; i < *indiceAero; i++) {
		if (!(_tcscmp(lista[i].nome, nome))) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL verificaRaioAero(TCHAR* nomeAero, const int* coordX, const int* coordY, const aeroporto* listaAeroportos, const int* tamAeroportos) {
	int x = 0, y = 0;
	for (x = *coordX - 5; x < *coordX + 5; x++) {
		for (y = *coordY - 5; y < *coordY + 5; y++) {
			if (*coordX + x >= 0 && *coordY + y >= 0) {
				for (int i = 0; i < *tamAeroportos; ++i) {
					if (*coordX + x == listaAeroportos[i].localizacao.posX &&
						*coordY + y == listaAeroportos[i].localizacao.posY) {
						return FALSE;
					}
				}
			}
		}
	}
	return TRUE;
}

BOOL adicionaAero(aeroporto* lista, int* indiceAero, TCHAR* nomeAero, const int* coordX, const int* coordY) {
	aeroporto aux;
	_tcscpy_s(aux.nome, STR_TAM, nomeAero);
	aux.localizacao.posX = *coordX;
	aux.localizacao.posY = *coordY;


	if (*coordX > 1000 || *coordY > 1000) {
		_tprintf(L"Posicao inacessivel!\n");
		return FALSE;
	}

	//_tprintf(L"\n\nNome: %s posX: %i posY: %i\n\n", aux.nome, aux.localizacao.posX, aux.localizacao.posY);

	if (!verificaNomeAero(nomeAero, lista, indiceAero)) {
		_tprintf(L"Já existe um aeroporto com esse nome!\n");
		return FALSE;
	}

	if (!verificaRaioAero(nomeAero, coordX, coordY, lista, indiceAero)) {
		_tprintf(L"Já existe um aeroporto nessa posição ou num raio demasiado curto!\n");
		return FALSE;
	}

	lista[(*indiceAero)++] = aux;
	return TRUE;
}

void imprimeListaAeroporto(const aeroporto* lista, const int tamLista) {
	for (int i = 0; i < tamLista; ++i)
		_tprintf(L"Aeroporto %s:\nLocalização: x = %d\ty = %d\n",
			lista[i].nome, lista[i].localizacao.posX, lista[i].localizacao.posY);
	_tprintf(L"\n");
}

aeroporto* inicializaListaAeroportos(int tamLista) {
	aeroporto* aeroportos = (aeroporto*) malloc(sizeof(aeroporto) * tamLista);
	if (aeroportos == NULL) {
		fatal(L"Não foi possível criar a lista de aeroportos");
		return NULL;
	}
	
	return aeroportos;
}

TCHAR* listaAero(const aeroporto* lista, const int tamLista) {
	TCHAR lstAux[500] = _TEXT(" ");
	TCHAR lstAero[500] = _TEXT(" ");
	for (int i = 0; i < tamLista - 1; ++i) {
		_stprintf_s(lstAux, 500, L"Aeroporto: %s Localização x: %d y: %d\n",
			lista[i].nome,lista[i].localizacao.posX,lista[i].localizacao.posY);
		_tcscat_s(lstAero, 100, lstAux);
	}
	return lstAero;
}