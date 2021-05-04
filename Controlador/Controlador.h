#pragma once

#include "Aviao.h"

typedef struct {
	HANDLE hFileMap;
	HANDLE hSemItens;
	HANDLE hSemMutexProd;
	bufferCircular* pBuf;
} controloBufferCirc;

typedef struct {
	HANDLE hFileMap;
	aviao* pAviao;
	HANDLE hEvento;
} memoriaPartilhada;

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

// Funcoes de Controlo do Buffer Circular em SHMem
BOOL criaBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada do Avião
BOOL abreMemoriaPartilhada(listaAviao* aviao);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

// Funcoes para manuseamento de avioes
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes);
int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes);
void imprimeListaAvioes(listaAviao* lista, int tamAvioes);