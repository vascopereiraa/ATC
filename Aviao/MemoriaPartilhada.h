#pragma once

#include <Windows.h>

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
	controloBufferCirc bufCirc;
	memoriaPartilhada memPart;
	aviao av;
	BOOL terminaAviao;
} infoAviao;

// Funcoes de Controlo do Buffer Circular
BOOL abreBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada
BOOL criaMemoriaPartilhada(memoriaPartilhada* memPartilhada);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);
