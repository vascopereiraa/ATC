
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Controlador.h"
#include "Utils.h"

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
		
	// Inicializa a lista de Avioes
	listaAviao* avioes = inicializaListaAviao(infoControl.tamAvioes);
	if (avioes == NULL) {
		encerraControlador(&infoControl);
		return 1;
	}
	infoControl.listaAvioes = avioes;
	
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

	_tprintf(L"\n\n VALOR DOS TAMS: %d %d\n\n", infoControl->tamAeroporto, infoControl->tamAvioes);


	int opcao;

	while (!*(infoControl->terminaControlador)) {
		_tprintf(L"Menu:\n");
		_tprintf(L"1 - Criar aeroportos\n");
		_tprintf(L"2 - Listar aeroportos\n");
		_tprintf(L"3 - Listar avioes\n");
		_tprintf(L"4 - Suspender/alterar entrada a novos aviões\n");
		_tprintf(L"\n");
		_tprintf(L"0 - Termina Controlador\n");
		_tscanf_s(L"%i", &opcao);

		switch (opcao) {
		case 1:	// Criar aeroportos
			if (!adicionaAeroporto(infoControl->listaAeroportos, &infoControl->indiceAero))
				_tprintf(L"\nNão foi possivel adicionar o aeroporto à lista\n\n");
			_tprintf(L"\nAeroporto adicionado à lista de aeroportos\n\n");
			break;
		case 2: // Listar aeroportos
			_tprintf(L"Lista de aeroportos:\n");
			imprimeListaAeroporto(infoControl->listaAeroportos, infoControl->indiceAero);
			_tprintf(L"\n");
			break;
		case 3:	// Listar avioes
			_tprintf(L"Lista de avioes:\n");
			imprimeListaAvioes(infoControl->listaAvioes, infoControl->tamAvioes);
			_tprintf(L"\n");
			break;
		case 4: // Suspender aceitacao de novos avioes
			*(infoControl->suspendeNovosAvioes) = (*(infoControl->suspendeNovosAvioes) + 1) % 2;
			_tprintf(L"\n\n VALOR DO CONTROL: %d\n", *(infoControl->suspendeNovosAvioes));
			_tprintf(L"Aceitação de novos aviões alterada\n");
			break;
		case 0:
			*(infoControl->terminaControlador) = 1;
			break;
		default:
			erro(L"Opcao nao definida");
			break;
		}
	};

	// Manda todos os avioes encerrar
}