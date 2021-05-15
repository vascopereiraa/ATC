
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Controlador.h"
#include "Utils.h"
#include "Constantes.h"

int _tmain() {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	infoControlador infoControl;
	ZeroMemory(&infoControl, sizeof(infoControlador));
	
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
	controladorRegistry(&infoControl.tamAeroporto, &infoControl.tamAvioes);

	_tprintf(L"\n\nValores do registry: %d %d\n\n", infoControl.tamAeroporto, infoControl.tamAvioes);

	// Inicializa a lista de Aeroportos
	aeroporto* aeroportos = inicializaListaAeroportos(infoControl.tamAeroporto);
	if (aeroportos == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.listaAeroportos = aeroportos;

#ifdef TESTES
	_tcscpy_s(&infoControl.listaAeroportos[0].nome,STR_TAM, L"porto");
	infoControl.listaAeroportos[0].localizacao.posX = 0;
	infoControl.listaAeroportos[0].localizacao.posY = 0;

	_tcscpy_s(&infoControl.listaAeroportos[1].nome, STR_TAM, L"lisboa");
	infoControl.listaAeroportos[1].localizacao.posX = 10;
	infoControl.listaAeroportos[1].localizacao.posY = 10;

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

	// Criar a Thread para gerenciar o buffer circular
	HANDLE hThreadBuffer = CreateThread(NULL, 0, threadControloBuffer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadBuffer == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}

	// Cria a Thread de Timer para a reação às respostas dos avioes
	HANDLE hThreadTimer = CreateThread(NULL, 0, threadTimer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadTimer == NULL) {
		*infoControl.terminaControlador = 1;
		WaitForSingleObject(hThreadBuffer, INFINITE);
		CloseHandle(hThreadBuffer);
		encerraControlador(&infoControl);
		return 1;
	}

	// Abre e executa o menu de utilizador
	menu(&infoControl);

	WaitForSingleObject(hThreadBuffer, INFINITE);
	WaitForSingleObject(hThreadTimer, INFINITE);
	CloseHandle(hThreadBuffer);
	CloseHandle(hThreadTimer);
	encerraControlador(&infoControl);

	fim(L"... Controlador Encerrado ...");
	return 0;
}

void encerraControlador(infoControlador* infoControl) {
	if (infoControl->bufCirc != NULL)
		encerraBufferCircular(infoControl->bufCirc);
	
	if (infoControl->listaAvioes != NULL)
		free(infoControl->listaAvioes);
	
	if (infoControl->listaAeroportos != NULL)
		free(infoControl->listaAeroportos);

	if(infoControl->terminaControlador != NULL)
		free(infoControl->terminaControlador);

	if (infoControl->suspendeNovosAvioes != NULL)
		free(infoControl->suspendeNovosAvioes);
}

void menu(infoControlador* infoControl) {
	int opcao;
	TCHAR comando[STR_TAM], comandoAux[STR_TAM];
	TCHAR* buffer = NULL;
	TCHAR* token = NULL;
	
	/*
	* aero + nome + coordX + coordY
	* laero = lista aeroportos
	* lavioes = lista avioes
	* susp = suspender comunicações
	* ret = retomar comunicações
	* end = terminar controlador
	*/

	while (!*(infoControl->terminaControlador)) {
		_tprintf(L"Insira [cmd] para ver os comandos disponiveis\n");
		_tprintf(L" > ");
		_fgetts(comando, STR_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0';
		
		_tcscpy_s(&comandoAux, STR_TAM, comando);
		token = _tcstok_s(comando, L" ", &buffer);
		if (!_tcscmp(token, L"aero")) {
			if (!adicionaAeroporto(infoControl->listaAeroportos, &infoControl->indiceAero, comandoAux))
				_tprintf(L"\nNão foi possivel adicionar o aeroporto à lista\n\n");
			else
				_tprintf(L"\nAeroporto adicionado à lista de aeroportos\n\n");
		}
		if (!_tcscmp(token, L"laero")) {
			_tprintf(L"Lista de aeroportos:\n");
			imprimeListaAeroporto(infoControl->listaAeroportos, infoControl->indiceAero);
			_tprintf(L"\n");
		}
		if (!_tcscmp(token, L"laviao")) {
			_tprintf(L"Lista de avioes:\n");
			imprimeListaAvioes(infoControl->listaAvioes, infoControl->tamAvioes);
			_tprintf(L"\n");
		}
		if (!_tcscmp(token, L"susp")) {
			if (*(infoControl->suspendeNovosAvioes) != 1) {
				*(infoControl->suspendeNovosAvioes) = 1;
				_tprintf(L"Aceitação de novos aviões suspendida!\n");
			}
			else
				erro(L"A aceitação de novos aviões já se encontrava suspensa!");
		}
		if (!_tcscmp(token, L"ret")) {
			*(infoControl->suspendeNovosAvioes) = 0;
			_tprintf(L"Aceitação de novos aviões alterada!\n");
		}
		if (!_tcscmp(token, L"end")) {
			*(infoControl->terminaControlador) = 1;
		}
		if (!_tcscmp(token, L"cmd")) {
			_tprintf(L"aero + nome + coordX + coordY = criar aeroporto\nlaero = lista aeroportos\nlavioes = lista avioes\nsusp = suspender comunicações\n"
				L"ret = retomar comunicações\nend = terminar controlador\ncmd = comandos disponiveis\n\n");
		}
	}

#ifdef TESTES
	for (int i = 1; i < infoControl->tamAeroporto; ++i) {
#else
	for (int i = 0; i < infoControl->tamAeroporto; ++i) {
#endif
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, infoControl->listaAvioes[i].av.procID);
		TerminateProcess(hProcess, 1);
	}
	
}