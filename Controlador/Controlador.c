
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Controlador.h"
#include "Utils.h"
#include "Constantes.h"
#include "Passageiro.h"

int _tmain() {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	infoControlador infoControl;
	ZeroMemory(&infoControl, sizeof(infoControlador));

	// Inicializa a lista de Passageiros e namedpipes
	InfoPassagPipes* infoPassagPipes = inicializaListaPassagPipes();
	if (infoPassagPipes == NULL) {
		return 1;
	}
	infoControl.infoPassagPipes = infoPassagPipes;

	// Variaveis de controlo
	infoControl.terminaControlador = (int*) malloc(sizeof(int));
	if (infoControl.terminaControlador == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.suspendeNovosAvioes = (int*) malloc(sizeof(int));
	if (infoControl.suspendeNovosAvioes == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}

	*infoControl.suspendeNovosAvioes = 0;
	*infoControl.terminaControlador = 0;

	// Carrega dados do Registry
	if (!controladorRegistry(&infoControl.tamAeroporto, &infoControl.tamAvioes)) {
		encerraControlador(&infoControl);
		return 1;
	}

	_tprintf(L"\nValores do registry:\n N?mero m?ximo de Aeroportos => %d\n N?mero m?ximo de Avi?es => %d\n", infoControl.tamAeroporto, infoControl.tamAvioes);

	// Inicializa a lista de Aeroportos
	aeroporto* aeroportos = inicializaListaAeroportos(infoControl.tamAeroporto);
	if (aeroportos == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.listaAeroportos = aeroportos;

#ifdef DEBUG
	_tcscpy_s(&infoControl.listaAeroportos[0].nome,STR_TAM, L"porto");
	infoControl.listaAeroportos[0].localizacao.posX = 0;
	infoControl.listaAeroportos[0].localizacao.posY = 0;

	_tcscpy_s(&infoControl.listaAeroportos[1].nome, STR_TAM, L"lisboa");
	infoControl.listaAeroportos[1].localizacao.posX = 50;
	infoControl.listaAeroportos[1].localizacao.posY = 50;

	infoControl.indiceAero = 2;
#endif

	// Inicializa a lista de Avioes
	listaAviao* avioes = inicializaListaAviao(infoControl.tamAvioes);
	if (avioes == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.listaAvioes = avioes;

#ifdef TESTES
	infoControl.listaAvioes[0].isFree = FALSE;
	infoControl.listaAvioes[0].av.procID = 1;
	infoControl.listaAvioes[0].av.atuais.posX = 5;
	infoControl.listaAvioes[0].av.atuais.posY = 5;
#endif

	// Criar Memoria Partilhada do Controlador
	controloBufferCirc bufCirc;
	if (!criaBufferCircular(&bufCirc)) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.bufCirc = &bufCirc;

	criaCriticalSectionControl(&infoControl.criticalSectionControl);

	// Criar a Thread para gerenciar o buffer circular
	HANDLE hThreadBuffer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) threadControloBuffer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadBuffer == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}

	// Cria a Thread de Timer para a rea??o ?s respostas dos avioes
	HANDLE hThreadTimer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) threadTimer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadTimer == NULL) {
		*infoControl.terminaControlador = 1;
		WaitForSingleObject(hThreadBuffer, INFINITE);
		CloseHandle(hThreadBuffer);
		encerraControlador(&infoControl);
		return 1;
	}

	// Pipes
	HANDLE hThreadNamedPipes = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) threadNamedPipes, (LPVOID)&infoControl, 0, NULL);
	if (hThreadTimer == NULL) {
		*infoControl.terminaControlador = 1;
		WaitForSingleObject(hThreadBuffer, INFINITE);
		WaitForSingleObject(hThreadTimer, INFINITE);
		CloseHandle(hThreadTimer);
		CloseHandle(hThreadBuffer);
		encerraControlador(&infoControl);
		return 1;
	}

	// Abre e executa o menu de utilizador
	menu(&infoControl);

	WaitForSingleObject(hThreadBuffer, INFINITE);
	WaitForSingleObject(hThreadTimer, INFINITE);
	WaitForSingleObject(hThreadNamedPipes, INFINITE);
	CloseHandle(hThreadBuffer);
	CloseHandle(hThreadTimer);
	CloseHandle(hThreadNamedPipes);
	encerraControlador(&infoControl);

	fim(L"... Controlador Encerrado ...");
	return 0;
}

