#pragma once
#include <Windows.h>
#include <tchar.h>
#include "../Controlador/Constantes.h"
#include "../Controlador/Aviao.h"

	typedef struct {
		int idPassag;				// ID do processo
		TCHAR nomePassag[STR_TAM];	// Nome do passageiro
		TCHAR aeroOrigem[STR_TAM];	// Aero de Origem do passageiro
		TCHAR aeroDestino[STR_TAM]; // Aero de Destino do passageiro
		TCHAR fraseInfo[STR_TAM];   // Passagem de info control-pass
		coordenadas coordAtuais;	// Coordenadas em que o pass. se encontra
		HANDLE hPipe; // Handle pipe
		int tempoEspera;  // Tempo de espera do passageiro
		int sair;		  // Caso o controlador mande sair
		int *sairPassag;  // Caso o passageiro queira sair
		int indicePipe; // Indice do pipe para o control poder aceder a posição correta no array de instancias
		int nrAviaoEmbarcado;	// Nr do avião embarcado
	} passageiro;