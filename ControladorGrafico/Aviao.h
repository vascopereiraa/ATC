
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
	BOOL isFree;					// Posicao livre
	aviao av;						// Informacao sobre o aviao
	memoriaPartilhada memAviao;		// Metodo de resposta ao aviao
	BOOL isAlive;				// Flag para verificar se está ativo
} listaAviao;

// Funcoes para manuseamento de avioes
listaAviao* inicializaListaAviao(int tamAvioes);
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes);
int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes);
void imprimeListaAvioes(listaAviao* lista, int tamAvioes);
int getIndiceAviao(aviao aux, listaAviao* listaAvioes, int tamAvioes);
int verificaAvioesPosicao(aviao aux, aeroporto* listaAeroportos, int tamAeroportos, listaAviao* listaAvioes, int tamAvioes);
coordenadas obterCoordenadas(TCHAR* string, aeroporto* listaAeroportos, int tamAeroportos);
TCHAR* listaAv(const listaAviao* lista, const int tamLista);

// Funcoes de Controlo da Memoria Partilhada do Avião
BOOL abreMemoriaPartilhada(listaAviao* aviao);
void encerraMemoriaPartilhada(memoriaPartilhada* memPart);

#endif // !__AVIAO_CONTROLADOR_H__

