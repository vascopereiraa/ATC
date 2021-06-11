#pragma once
#include <Windows.h>
#include <tchar.h>
#include "../Controlador/Constantes.h"
#include "../Controlador/Aviao.h"

typedef struct {
	int idPassag;
	TCHAR nomePassag[STR_TAM];
	TCHAR aeroOrigem[STR_TAM];
	TCHAR aeroDestino[STR_TAM];
	coordenadas coordAtuais;
	int tempoEspera;
	int sair;	// 1-> Não existe o aero de origem! 
	int* sairPassag;

	HANDLE hPipe; // Handle pipe
	// teste
	TCHAR fraseInfo[STR_TAM];
	int indicePipe; // Indice do pipe para o control poder aceder a posição correta no array de instancias
	int nrAviaoEmbarcado;
	int contadorTeste;
} passageiro;