
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "Aviao.h"
#include "Utils.h"
#include "Controlador.h"

BOOL verificaRaioAeroporto(const Aeroporto aux, const Aeroporto* listaAeroportos) {
	int i = 0, j = 0;
	for (i = aux.localizacao.posX - 5; i < aux.localizacao.posX + 5; i++) {
		for (j = aux.localizacao.posY - 5; j < aux.localizacao.posY + 5; j++) {
			if (i >= 0 && j >= 0) {
				if (aux.localizacao.posX == listaAeroportos[i].localizacao.posX &&
					aux.localizacao.posY == listaAeroportos[i].localizacao.posY) {
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

BOOL verificaNomeAeroporto(const Aeroporto aux, const infoControlador* control) {
	for (int i = 0; i < control->tamAeroporto; i++) {
		if (!(_tcscmp(control->listaAeroportos->nome, aux.nome))) {
			return FALSE;
		}
	}
	return TRUE;
}

// DEFINES DO REGISTRY
void WINAPI threadControloBuffer(LPVOID lpParam);

int _tmain() {

#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif
	infoControlador infoControl;
	ZeroMemory(&infoControl, sizeof(infoControlador));
	int opcao;
	Aeroporto aeroAux;
	// Criar Memoria Partilhada do Controlador
	controloBufferCirc bufCirc;
	controladorRegistry(&infoControl.tamAeroporto, &infoControl.tamAvioes);
		
	if (!criaBufferCircular(&bufCirc))
		return 1;

	// Criar lista de avioes
	infoControl.listaAvioes = malloc(infoControl.tamAvioes * sizeof(listaAviao));
	infoControl.listaAeroportos = malloc(infoControl.tamAeroporto * sizeof(Aeroporto));

	if (infoControl.listaAvioes == NULL) {
		fatal(L"Ocorreu um erro ao criar a lista de avioes");
		encerraBufferCircular(&bufCirc);
		return 1;
	}

	// FUNCAO PARA POR O ISFREE A TRUE EM TODOS AO CRIAR - todos livres
	for (int i = 0; i < infoControl.tamAvioes; ++i)
		infoControl.listaAvioes[i].isFree = TRUE;

	// Criar a Thread para gerenciar o buffer circular
	infoControl.bufCirc = &bufCirc;

	//AEROPORTOS
	infoControl.listaAeroportos[0].localizacao.posX = 0;
	infoControl.listaAeroportos[0].localizacao.posY = 0;
	wcscpy_s(infoControl.listaAeroportos[0].nome,STR_TAM, L"porto");

	infoControl.listaAeroportos[2].localizacao.posX = 2;
	infoControl.listaAeroportos[2].localizacao.posY = 2;
	wcscpy_s(infoControl.listaAeroportos[2].nome, STR_TAM, L"braga");

	infoControl.listaAeroportos[1].localizacao.posX = 15;
	infoControl.listaAeroportos[1].localizacao.posY = 15;
	wcscpy_s(infoControl.listaAeroportos[1].nome, STR_TAM, L"lisboa");

	infoControl.listaAvioes[0].av.procID = 1;
	infoControl.listaAvioes[0].av.atuais.posX = 3;
	infoControl.listaAvioes[0].av.atuais.posY = 3;
	infoControl.listaAvioes[0].isFree = FALSE;

	// Adicionar Aeroporto
	HANDLE hThreadBuffer = CreateThread(NULL, 0, threadControloBuffer, (LPVOID) &infoControl, 0, NULL);
	if (hThreadBuffer == NULL) {
		encerraBufferCircular(&bufCirc);
	}


	do {
		_tprintf(L"1 - Criação de um aeroporto: \n");
		_tscanf_s(L"%i", &opcao);

		switch (opcao)
		{
		case 1:
			_tprintf(L"Indique o nome do aeroporto: \n");
			_tscanf_s(L"%s", aeroAux.nome, STR_TAM);
			_tprintf(L"Indique as coordenadas do aeroporto: \n");
			_tscanf_s(L"%i%i", &aeroAux.localizacao.posX, &aeroAux.localizacao.posY);
			if (!verificaNomeAeroporto(aeroAux, &infoControl)) {
				_tprintf(L"Já existe um aeroporto com esse nome!\n");
				break;
			}
			if (!verificaRaioAeroporto(aeroAux, infoControl.listaAeroportos)) {
				_tprintf(L"Já existe um aeroporto nessa posição!\n");
				break;
			}
			infoControl.listaAeroportos[infoControl.indiceAero++] = aeroAux;

			_tprintf((L"NOME: %s X: (%i) Y: (%i) \n"), aeroAux.nome, aeroAux.localizacao.posX, aeroAux.localizacao.posY);
			for (int i = 0; i < infoControl.indiceAero; i++) {
				_tprintf(L"Aeroporto (%i) : %s!\n", i, infoControl.listaAeroportos[i].nome);
			}

			break;
		default:
			break;
		}
	} while (1);


	WaitForSingleObject(hThreadBuffer, INFINITE);
	CloseHandle(hThreadBuffer);
	free(infoControl.listaAvioes);
	free(infoControl.listaAeroportos);
	encerraBufferCircular(&bufCirc);

	return 0;
}

// FUNCAO A PASSAR PARA UM FICHEIRO DE AVIOES
BOOL isNovoAviao(aviao av, listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (av.procID == lista[i].av.procID)
			return FALSE;
	return TRUE;
}

int getPrimeiraPosVazia(listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (lista[i].isFree)
			return i;
	return -1;
}

void imprimeListaAvioes(listaAviao* lista, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i) {
		if (lista[i].isFree == FALSE) {
			_tprintf(L"Aviao %d:\n", i + 1);
			_tprintf(L"Nr Avião: %d\n\n", lista[i].av.procID);
		}
	}
}

int getIndiceAviao(aviao aux, listaAviao* listaAvioes, int tamAvioes) {
	for (int i = 0; i < tamAvioes; ++i)
		if (aux.procID == listaAvioes[i].av.procID)
			return i;
	return -1;
}

BOOL verificaAvioesPosicao(aviao aux, Aeroporto* listaAeroportos,int tamAeroportos, listaAviao* listaAvioes, int tamAvioes) {
	
	//ProxCoord ? Assumindo que a 1º posição está correta devido a verificacao com obterCoordenadasOrigemDestino
	for (int i = 0; i < tamAeroportos; i++) {
		/*if (listaAeroportos[i].localizacao.posX == aux.proxCoord.posX &&
			listaAeroportos[i].localizacao.posY == aux.proxCoord.posY) {
			_tprintf(L"\n\nEstá no aeroporto de origem !\n\n");
			return FALSE;
		}*/
		if (listaAeroportos[i].localizacao.posX == aux.proxCoord.posX && aux.destino.posX == aux.proxCoord.posX
		 && listaAeroportos[i].localizacao.posY == aux.proxCoord.posY && aux.destino.posY == aux.proxCoord.posY) {
			_tprintf(L"\n\nEstá no aeroporto de destino !\n\n");
			return FALSE;
		}
	}

	for (int i = 0; i < tamAvioes; i++) {
		if (listaAvioes[i].av.atuais.posX == aux.proxCoord.posX && 
			listaAvioes[i].av.atuais.posY == aux.proxCoord.posY &&
			listaAvioes->isFree == FALSE && listaAvioes[i].av.procID != aux.procID) {
			_tprintf(L"\n\nTem avião no mesmo sitio !\n\n");
			return TRUE;
		}
	}
	_tprintf(L"\n\nPode avançar\n\n");
	return FALSE;
}

BOOL obterCoordenadasOrigemDestino(aviao *aux, Aeroporto* listaAeroportos, int tamAeroportos) {
	int contador = 0;
	for (int i = 0; i < tamAeroportos; i++) {
		if (_tcscmp(listaAeroportos[i].nome, aux->aeroOrigem) == 0) {
			aux->atuais.posX = listaAeroportos[i].localizacao.posX;
			aux->atuais.posY = listaAeroportos[i].localizacao.posY;
			_tprintf(L"Destino x: [%i] y: [%i]", aux->atuais.posX, aux->atuais.posY);
			_tprintf(L"\nAtribuido aeroporto origem [%s] !\n", listaAeroportos[i].nome);
			++contador;
		}
		if (_tcscmp(listaAeroportos[i].nome, aux->aeroDestino) == 0) {
			aux->destino.posX = listaAeroportos[i].localizacao.posX;
			aux->destino.posY = listaAeroportos[i].localizacao.posY;
			_tprintf(L"Destino x: [%i] y: [%i]", aux->destino.posX, aux->destino.posY);
			_tprintf(L"\nEstá no aeroporto de destino [%s] !\n", listaAeroportos[i].nome);
			++contador;
		}
	if (contador == 2)
		return TRUE;
	}
	return FALSE;
}

void WINAPI threadControloBuffer(LPVOID lpParam) {

	infoControlador* dados = (infoControlador*)lpParam;
	
	controloBufferCirc* bufCirc = dados->bufCirc;
	listaAviao* listaAvioes = dados->listaAvioes;
	Aeroporto* listaAeroportos = dados->listaAeroportos;
	int tamAvioes = dados->tamAvioes;
	int tamAeroportos = dados->tamAeroporto;

	aviao aux;
	while (1) {
		debug(L"Estou a espera");
		WaitForSingleObject(bufCirc->hSemItens, INFINITE);
		aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
		bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF; 
		if (isNovoAviao(aux, listaAvioes, tamAvioes)) {
			int pos = getPrimeiraPosVazia(listaAvioes, tamAvioes);
			if (pos != -1) {
				listaAvioes[pos].av = aux;
				if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
					debug(L"Nao correu bem");
					// Avisa o aviao e manda fechar
				}
				else {
					listaAvioes[pos].isFree = FALSE;
					listaAvioes[pos].isAlive = TRUE;
					//Ao registar o aviao, verifica se existe aeroporto de origem e destino
					if (!obterCoordenadasOrigemDestino(&aux, listaAeroportos, tamAeroportos)) {
						//listaAvioes[pos].av.terminaExecucao = TRUE;
						aux.terminaExecucao = TRUE;
						_tprintf(L"Não existem o aeroporto de origem ou destino\n");
					}else {
						_tprintf(L"Aero origem x: [%i] y: [%i] \t Aero destino x: [%i] y: [%i]\n", aux.atuais.posX, aux.atuais.posY, aux.destino.posX, aux.destino.posY);
					}
					listaAvioes[pos].av = aux;
					*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
					SetEvent(listaAvioes[pos].memAviao.hEvento);
				}
			}
			else {
				erro(L"Nao exitem mais espacos para avioes");
				// Avisa o aviao e manda fechar
			}
		}
		else {
			int pos = getIndiceAviao(aux, listaAvioes, tamAvioes);
			if (pos == -1)
				erro(L"indice -1");
			else {
				listaAvioes[pos].isAlive = TRUE;
				if(!verificaAvioesPosicao(aux, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
					debug(L"Pode avancar da posição");
					aux.atuais = aux.proxCoord;
				}
				else {
					// DEBUG 
					listaAvioes[0].av.atuais.posX = 8;
					listaAvioes[0].av.atuais.posY = 8;
					debug(L"Não pode avancar!! Espera!");
				}
				listaAvioes[pos].av = aux;
				*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
				SetEvent(listaAvioes[pos].memAviao.hEvento);
			}
		}
		imprimeListaAvioes(listaAvioes, tamAvioes);
	}

}

void WINAPI threadTimer(LPVOID lpParam) {
	infoControlador* dados = (infoControlador*)lpParam;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;
	// While 1, a substituir por flag de termino na struct principal
	while(1) {
		Sleep(3000);
		for (int i = 0; i < tamAvioes; i++) {
			if (listaAvioes[i].isAlive)
				listaAvioes[i].isAlive = FALSE;
			else
				listaAvioes[i].isFree = TRUE;
		}
	}
}