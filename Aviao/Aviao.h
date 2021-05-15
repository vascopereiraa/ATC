
#ifndef __AVIAO_H__
#define __AVIAO_H__

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
	int capMaxima;					// Capacidade maxima do avião
	int velocidade;					// Velocidade de posições por segundo
	coordenadas proxCoord;			// Coordenadas da possivel proxima posicao do aviao

	int terminaExecucao;			// Flag para encerrar o aviao
	BOOL emViagem;					// Flag para verificar que o aviao nao esta parado no aeroporto
	BOOL isSobreposto;				// Avioes sobrepostos
} aviao;

typedef struct {
	aviao buf[MAX_BUF];		// Buffer Circular
	int numCons;			// Prox. posicao a consumir
	int numProd;			// Prox. posicao a produzir
} bufferCircular;

#endif // !__AVIAO_H__
