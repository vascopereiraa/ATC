
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Aviao.h"
#include "Aeroporto.h"
#include "Controlador.h"
#include "Utils.h"

BOOL verificaNomeAeroporto(const aeroporto aux, const aeroporto* lista, const int* indiceAero) {
	for (int i = 0; i < *indiceAero; i++) {
		if (!(_tcscmp(lista->nome, aux.nome))) {
			return FALSE;
		}
	}
	return TRUE;
}

BOOL verificaRaioAeroporto(const aeroporto aux, const aeroporto* listaAeroportos) {
	int i = 0, j = 0;
	for (i = aux.localizacao.posX - 5; i < aux.localizacao.posX + 5; i++) {
		for (j = aux.localizacao.posY - 5; j < aux.localizacao.posY + 5; j++) {
			if (i >= 0 && j >= 0) {
				if (aux.localizacao.posX == listaAeroportos[i].localizacao.posX &&
					aux.localizacao.posY == listaAeroportos[i].localizacao.posY) {
					return FALSE;
				}
			}
		}
	}
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

BOOL adicionaAeroporto(aeroporto* lista, int* indiceAero, TCHAR* comando) {
	aeroporto aux;
	TCHAR* buffer = NULL,*token = NULL;
	//Ignorar 1 palavra identificadora do comando!
	token = wcstok_s(comando, L" ", &buffer);

	//_tprintf(L"\nString: %s\n\n", comando);
	token = wcstok_s(NULL, L" ", &buffer);
	wcscpy_s(&aux.nome, STR_TAM, token);
	aux.localizacao.posX = _ttoi(wcstok_s(NULL, L" ", &buffer));
	aux.localizacao.posY = _ttoi(wcstok_s(NULL, L" ", &buffer));
	//_tprintf(L"\n\nNome: %s posX: %i posY: %i\n\n", aux.nome, aux.localizacao.posX, aux.localizacao.posY);

	if (!verificaNomeAeroporto(aux, lista, indiceAero)) {
		_tprintf(L"Já existe um aeroporto com esse nome!\n");
		return FALSE;
	}

	if (!verificaRaioAeroporto(aux, lista)) {
		_tprintf(L"Já existe um aeroporto nessa posição ou num raio demasiado curto!\n");
		return FALSE;
	}

	lista[(*indiceAero)++] = aux;
	return TRUE;
}
