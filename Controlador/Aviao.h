
#ifndef __AVIAO_CONTROLADOR_H__
#define __AVIAO_CONTROLADOR_H__

#include <Windows.h>

#include "../Aviao/Aviao.h"
#include "Aeroporto.h"

typedef struct {
	HANDLE hFileMap;		// File Map do aviao
	aviao* pAviao;			// Ponteiro para a SHM do aviao
	HANDLE hEvento;			// Evento para leitura de dados no aviao
} memoriaPartilhada;

typedef struct {
	boolean isFree;				// Posicao livre
	aviao av;					// Informacao sobre o aviao
	memoriaPartilhada memAviao;	// Metodo de resposta ao aviao
	boolean isAlive;				// Flag para verificar se está ativo
} listaAviao;

// Funcoes para manuseamento de avioes
listaAviao* inicializaListaAviao(int tamAvioes);
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes);
int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes);
void imprimeListaAvioes(listaAviao* lista, int tamAvioes);
int getIndiceAviao(aviao aux, listaAviao* listaAvioes, int tamAvioes);
BOOL verificaAvioesPosicao(aviao aux, aeroporto* listaAeroportos, int tamAeroportos, listaAviao* listaAvioes, int tamAvioes);
BOOL obterCoordenadasOrigemDestino(aviao* aux, aeroporto* listaAeroportos, int tamAeroportos);

// Funcoes de Controlo da Memoria Partilhada do Avião
BOOL abreMemoriaPartilhada(listaAviao* aviao);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

#endif // !__AVIAO_CONTROLADOR_H__

