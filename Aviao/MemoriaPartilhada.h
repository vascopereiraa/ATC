
#ifndef __MEMORIA_PARTILHADA_H__
#define __MEMORIA_PARTILHADA_H__

#include <Windows.h>

#include "Aviao.h"

typedef struct {
	HANDLE hFileMap;		// Handle para a zona de mem�ria partilhada
	HANDLE hSemItens;	    // Handle para o semaforo de Itens colocados no buffer
	HANDLE hSemMutexProd;   // Handle para o semaforo bin�rio usado para sincroniza��o entre avi�es
	bufferCircular* pBuf;   // Ponteiro para a vista partilhada em mem�ria
} controloBufferCirc;

typedef struct {
	HANDLE hFileMap;	// Handle para a zona de mem�ria partilhada
	aviao* pAviao;		// Ponteiro para a vista partilhada em mem�ria 
	HANDLE hEvento;		// Handle para o evento de sincroniza��o
} memoriaPartilhada;

typedef struct {
	controloBufferCirc bufCirc;			   // Estrutura para aceder Buffer Circular
	memoriaPartilhada memPart;			   // Estrutura para aceder a mem�ria partilhada
	aviao av;							   // Estutura com os dados do Avi�o
	BOOL terminaAviao;					   // Flag para determinar o fim da execu��o
	CRITICAL_SECTION criticalSectionAviao; // Garante sincroniza��o entre threads
} infoAviao;

// Funcoes de Controlo do Buffer Circular
BOOL abreBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada
BOOL criaMemoriaPartilhada(memoriaPartilhada* memPartilhada);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

// Objetos de sincroniza��o
void criaCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao);
void encerraCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao);

#endif // !__MEMORIA_PARTILHADA_H__
