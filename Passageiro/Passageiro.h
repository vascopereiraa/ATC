#pragma once
#include <Windows.h>
#include <tchar.h>
#include "../Controlador/Constantes.h"
#include "../Controlador/Aviao.h"

typedef struct {
	TCHAR aeroOrigem[STR_TAM];
	TCHAR aeroDestino[STR_TAM];
	// Colocar o avião ? Maybe
	coordenadas atuais;
	int idPassag;
	TCHAR nomePassag[STR_TAM];
	int tempoEspera;
	HANDLE hPipe;

	// teste
	TCHAR fraseTeste[STR_TAM];
} passageiro;