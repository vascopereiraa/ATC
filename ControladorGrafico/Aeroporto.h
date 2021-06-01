#ifndef __AEROPORTO_H__
#define __AEROPORTO_H__

#include <tchar.h>

#include "../Aviao/Aviao.h"
#include "Constantes.h"

typedef struct {
	TCHAR nome[STR_TAM];	   // Nome do aeroporto
	coordenadas localizacao;   // Coordenadas do aeroporto
} aeroporto;

// Funcoes de controlo dos aeroportos
BOOL verificaNomeAero(TCHAR* nome, const aeroporto* lista, const int* indiceAero);
BOOL verificaRaioAero(TCHAR* nomeAero, const int* coordX, const int* coordY, const aeroporto* listaAeroportos, const int* tamAeroportos);
void imprimeListaAeroporto(const aeroporto* lista, const int tamLista);
aeroporto* inicializaListaAeroportos(int tamLista);
BOOL adicionaAero(aeroporto* lista, int* indiceAero, TCHAR* nomeAero, const int* coordX, const int* coordY);
TCHAR listaAero(const aeroporto* lista, const int tamLista);

#endif