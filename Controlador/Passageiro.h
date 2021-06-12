#pragma once
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "../Passageiro/Passageiro.h"

typedef struct {
	passageiro passag;
	BOOL isFree;
} listaPassag;

// NAMED PIPES
typedef struct {
	OVERLAPPED oOverLap;
	HANDLE hPipeInst;
	BOOL fPendingIO;
	DWORD dwState;
} PIPESTRUCT, * LPPIPEINST;

typedef struct {
	HANDLE hEvents[MAX_PASSAG];
	PIPESTRUCT hPipes[MAX_PASSAG];
	listaPassag listPassag[MAX_PASSAG];
	HANDLE hMutex;
	int terminar;
} InfoPassagPipes;

void imprimeListaPassag(const listaPassag* lista);
int getPrimeiraPosVaziaPassag(listaPassag* listPassag);
BOOL isNovoPassag(passageiro passag, listaPassag* listPassag);
int getPosPassag(passageiro aux, listaPassag* listaPassag);
BOOL verificaAeroExiste(passageiro passag, aeroporto* listaAeroportos, int tamAeroportos);
BOOL embarcaPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av);
InfoPassagPipes* inicializaListaPassagPipes();
void atualizaCoordPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av);
void informaPassagDestino(InfoPassagPipes* infoPassagPipe, aviao* av);
TCHAR* listaPass(const listaPassag* lista);
void DisconnectAndReconnect(InfoPassagPipes* infoPassagPipes, int indice, const passageiro PassAux);
void DestroyPassageiros(InfoPassagPipes* infoPassagPipe);
void displayInfoPassag(passageiro passagAux);