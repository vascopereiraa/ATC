#pragma once
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "../Passageiro/Passageiro.h"

typedef struct {
	passageiro passag; // Estrutura do passageiro
	BOOL isFree;      // Flag a indicar se posição esta livre
} listaPassag;

typedef struct {
	OVERLAPPED oOverLap; // Estrutura overlapped
	HANDLE hPipeInst;	 // Instancia do pipe atribuida
	BOOL fPendingIO;     // Flag para indicação de leitura ou escrita pendente
	DWORD dwState;	     // Estado do pipe
} PIPESTRUCT;

typedef struct {
	HANDLE hEvents[MAX_PASSAG];		    // Array de Handles dos eventos associados a cada pipe
	PIPESTRUCT hPipes[MAX_PASSAG];      // Array de instâncias de pipes criados
	listaPassag listPassag[MAX_PASSAG];	// Array de Passageiros
} InfoPassagPipes;

void imprimeListaPassag(const listaPassag* lista);
int getPrimeiraPosVaziaPassag(listaPassag* listPassag);
BOOL isNovoPassag(passageiro passag, listaPassag* listPassag);
int getPosPassag(passageiro aux, listaPassag* listaPassag);
BOOL verificaAeroExiste(passageiro *passag, aeroporto* listaAeroportos, int tamAeroportos);
BOOL embarcaPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av);
InfoPassagPipes* inicializaListaPassagPipes();
void atualizaCoordPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av);
void informaPassagDestino(InfoPassagPipes* infoPassagPipe, aviao* av);
TCHAR* listaPass(const listaPassag* lista);
void DisconnectAndReconnect(InfoPassagPipes* infoPassagPipes, int indice, const passageiro PassAux);
void DestroyPassageiros(InfoPassagPipes* infoPassagPipe);
void displayInfoPassag(passageiro passagAux);