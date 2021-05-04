#pragma once

#include <Windows.h>
#include <tchar.h>

// Dados - Controlador
#define SHM_CONTROL _TEXT("SHM_CONTROL")
#define SEM_MUTEX_PROD _TEXT("SEM_MUTEX_CONS")
#define SEM_ITENS _TEXT("SEM_ITENS")
#define MAX_BUF 10
#define STR_TAM 50

// Dados - Aviao
#define SHM_AVIAO _TEXT("SHM_%d")
#define EVNT_AVIAO _TEXT("EVNT_%d")

typedef struct {
	int posY;
	int posX;
} coordenadas;

typedef struct {
	DWORD procID;					// ID do processo do Aviao
	TCHAR aeroOrigem[STR_TAM];		// Nome do Aeroporto Origem
	coordenadas atuais;				// Coordenadas atuas
	TCHAR aeroDestino[STR_TAM];		// Nome do Aeroporto Destino
	coordenadas destino;			// Coordenadas do aeroporto destino
} aviao;

typedef struct {
	aviao buf[MAX_BUF];		// Buffer Circular
	int numCons;			// Prox. posicao a consumir
	int numProd;			// Prox. posicao a produzir
} bufferCircular;
