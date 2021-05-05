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

	// Flag para verificar se est� ativo
	BOOL isAlive;

} listaAviao;

typedef struct {
	TCHAR nome[STR_TAM];
	coordenadas localizacao;
} Aeroporto;

typedef struct {
	controloBufferCirc* bufCirc;
	listaAviao* listaAvioes;
	Aeroporto* listaAeroportos;

	int tamAvioes;
	int tamAeroporto;
	int indiceAero;

	// Flag para terminar a thread de controlo do bufCirc
} infoControlador;

// Funcoes de Controlo do Buffer Circular em SHMem
BOOL criaBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada do Avi�o
BOOL abreMemoriaPartilhada(listaAviao* aviao);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

// Funcoes para manuseamento de avioes
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes);
int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes);
void imprimeListaAvioes(listaAviao* lista, int tamAvioes);