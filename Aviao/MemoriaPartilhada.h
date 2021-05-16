
#ifndef __MEMORIA_PARTILHADA_H__
#define __MEMORIA_PARTILHADA_H__

#include <Windows.h>

#include "Aviao.h"

typedef struct {
	HANDLE hFileMap;		// Handle para a zona de memória partilhada
	HANDLE hSemItens;	    // Handle para o semaforo de Itens colocados no buffer
	HANDLE hSemMutexProd;   // Handle para o semaforo binário usado para sincronização entre aviões
	bufferCircular* pBuf;   // Ponteiro para a vista partilhada em memória
} controloBufferCirc;

typedef struct {
	HANDLE hFileMap;	// Handle para a zona de memória partilhada
	aviao* pAviao;		// Ponteiro para a vista partilhada em memória 
	HANDLE hEvento;		// Handle para o evento de sincronização
} memoriaPartilhada;

typedef struct {
	controloBufferCirc bufCirc;			   // Estrutura para aceder Buffer Circular
	memoriaPartilhada memPart;			   // Estrutura para aceder a memória partilhada
	aviao av;							   // Estutura com os dados do Avião
	BOOL terminaAviao;					   // Flag para determinar o fim da execução
	CRITICAL_SECTION criticalSectionAviao; // Garante sincronização entre threads
} infoAviao;

// Funcoes de Controlo do Buffer Circular
BOOL abreBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

// Funcoes de Controlo da Memoria Partilhada
BOOL criaMemoriaPartilhada(memoriaPartilhada* memPartilhada);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

// Objetos de sincronização
void criaCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao);
void encerraCriticalSectionAviao(LPCRITICAL_SECTION lpCriticalSectionAviao);

#endif // !__MEMORIA_PARTILHADA_H__
