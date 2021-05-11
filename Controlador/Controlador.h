
#ifndef __CONTROLADOR_H__
#define __CONTROLADOR_H__

#include <Windows.h>

#include "../Aviao/Aviao.h"
#include "Aviao.h"
#include "Aeroporto.h"

typedef struct {
	HANDLE hFileMap;
	HANDLE hSemItens;
	HANDLE hSemMutexProd;
	bufferCircular* pBuf;
} controloBufferCirc;

typedef struct {
	controloBufferCirc* bufCirc;		// Estrutura de dados do buffer circular
	listaAviao* listaAvioes;			// Array de avioes
	aeroporto* listaAeroportos;		// Array de aeroportos

	int tamAvioes;					// Tamanho do array de avioes
	int tamAeroporto;				// Tamanho do array de aeroportos
	int indiceAero;					// Indice da proxima posicao livre no array de aeroportos

	int* terminaControlador;		// Flag para terminar o controlador
	int* suspendeNovosAvioes;		// Flag para suspender/aceitar novos avioes
} infoControlador;

// Funcoes de Controlo do Buffer Circular em SHMem
BOOL criaBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

void encerraControlador(infoControlador* infoControl);
void menu(infoControlador* infoControl);

// Threads do controlador
void WINAPI threadControloBuffer(LPVOID lpParam);
void WINAPI threadTimer(LPVOID lpParam);

// Funcoes - Registry
HKEY abreOuCriaChave();
void obtemValoresRegistry(HKEY chave, int* maxAeroportos, int* maxAvioes);
void criaValoresRegistry(HKEY chave);
BOOL controladorRegistry(int* maxAeroportos, int* maxAvioes);

#endif // !__CONTROLADOR_H__