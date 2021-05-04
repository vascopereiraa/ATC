#pragma once

#include "MemoriaParilhada.h"
#include "Aviao.h"

typedef struct {
	// Posicao livre
	BOOL isFree;
	
	// Informacao sobre o aviao
	aviao av;

	// Metodo de resposta ao aviao
	memoriaPartilhada memAviao;

	// Thread especifica desse aviao & flag de termino

} listaAviao;

typedef struct {
	controloBufferCirc* bufCirc;
	listaAviao* listaAvioes;
	int tamAvioes;

	// Flag para terminar a thread de controlo do bufCirc

} infoControlador;

// Funcoes para manuseamento de avioes
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes);
int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes);
void imprimeListaAvioes(listaAviao* lista, int tamAvioes);