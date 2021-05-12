#pragma once

#include <Windows.h>
#include <tchar.h>

#include "../Controlador/Constantes.h"

typedef struct {
	int posX;
	int posY;
} coordenadas;

typedef struct {
	DWORD procID;					// ID do processo do aviao
	TCHAR aeroOrigem[STR_TAM];		// Nome do aeroporto origem
	coordenadas atuais;				// Coordenadas atuas
	TCHAR aeroDestino[STR_TAM];		// Nome do aeroporto destino
	coordenadas destino;			// Coordenadas do aeroporto destino
	int capMaxima;					// Capacidade maxima do avi�o
	int velocidade;					// Velocidade de posi��es por segundo
	coordenadas proxCoord;			// Coordenadas da possivel proxima posicao do aviao

	BOOL terminaExecucao;			// Flag para encerrar o aviao
} aviao;

typedef struct {
	aviao buf[MAX_BUF];		// Buffer Circular
	int numCons;			// Prox. posicao a consumir
	int numProd;			// Prox. posicao a produzir
} bufferCircular;

// Fun��es do avi�o de comunica��o
// BOOL registaEntrada(controloBufferCirc* bufCirc, memoriaPartilhada* memPart, aviao* av);
// void comunicaAviao(controloBufferCirc* bufCirc, memoriaPartilhada* memPart, aviao* av);