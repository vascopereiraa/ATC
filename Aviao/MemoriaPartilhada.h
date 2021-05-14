
#ifndef __MEMORIA_PARTILHADA_H__
#define __MEMORIA_PARTILHADA_H__

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
	HANDLE hEventoViagem;
} infoAviao;

// Funcoes de Controlo do Buffer Circular
BOOL abreBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada
BOOL criaMemoriaPartilhada(memoriaPartilhada* memPartilhada);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

// Objetos de sincronização
void encerraEventoViagem(infoAviao* infoAv);
BOOL criaEventoViagem(infoAviao* infoAv);

#endif // !__MEMORIA_PARTILHADA_H__
