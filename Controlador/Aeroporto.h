#ifndef __AEROPORTO_H__
#define __AEROPORTO_H__

#include <tchar.h>

#include "../Aviao/Aviao.h"
#include "Constantes.h"

typedef struct {
	TCHAR nome[STR_TAM];
	coordenadas localizacao;
} aeroporto;

// Funcoes de controlo dos aeroportos
BOOL verificaNomeAeroporto(const aeroporto aux, const aeroporto* lista, const int* indiceAero);
BOOL verificaRaioAeroporto(const aeroporto aux, const aeroporto* listaAeroportos, const int* tamAeroportos);
void imprimeListaAeroporto(const aeroporto* lista, const int tamLista);
aeroporto* inicializaListaAeroportos(int tamLista);
BOOL adicionaAeroporto(aeroporto* lista, int* indiceAero, TCHAR* comando);

#endif