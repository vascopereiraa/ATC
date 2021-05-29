#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Passageiro.h"
#include "Constantes.h"
#include "Utils.h"
#include "Controlador.h"


InfoPassagPipes* inicializaListaPassagPipes() {
	InfoPassagPipes* infoPassagPipes = (InfoPassagPipes*)malloc(sizeof(InfoPassagPipes));
	if (infoPassagPipes == NULL) {
		fatal(L"Ocorreu um erro ao criar a lista de avioes");
		return NULL;
	}

	// Coloca todas as posicoes livres no array
	for (int i = 0; i < MAX_PASSAG; ++i) {
		infoPassagPipes->listPassag[i].isFree = TRUE;
	}
	return infoPassagPipes;
}

void imprimeListaPassag(const listaPassag* lista) {
	for (int i = 0; i < MAX_PASSAG; ++i) {
		if (lista[i].isFree == FALSE) {
			_tprintf(L"ID: %d\n", lista[i].passag.idPassag);
			_tprintf(L"Nome:: %s\n", lista[i].passag.nomePassag);
			_tprintf(L"Origem: %s\t", lista[i].passag.aeroOrigem);
			_tprintf(L"Destino: %s\n", lista[i].passag.aeroDestino);
			_tprintf(L"Coord x: [%d] Coord y: [%d]\n", lista[i].passag.atuais.posX, lista[i].passag.atuais.posY);
			_tprintf(L"\n");
		}
	}
}

BOOL isNovoPassag(passageiro passag, listaPassag* listPassag) {
	for (int i = 0; i < MAX_PASSAG; ++i)
		if (passag.idPassag == listPassag[i].passag.idPassag)
			return FALSE;
	return TRUE;
}

int getPrimeiraPosVaziaPassag(listaPassag* listPassag) {
	for (int i = 0; i < MAX_PASSAG; ++i) {
		if (listPassag[i].isFree)
			return i;
		_tprintf("Valor do bool: %d\n", listPassag[i].isFree);
	}
	return -1;
}

// Retorna 1 quando não existe aeroOrigem
// Retorna 2 quando não existe aeroDestino
// Retorna 0 quando existem ambos.
int verificaAeroExiste(passageiro passag, aeroporto* listaAeroportos, int tamAeroportos) {
	BOOL flag = 0;
	for (int i = 0; i < tamAeroportos; i++) 
		if (!_tcscmp(passag.aeroOrigem, listaAeroportos[i].nome)) 
			flag = 1;
	if (!flag)
		return 1;
	for (int i = 0; i < tamAeroportos; i++) 
		if (!_tcscmp(passag.aeroDestino, listaAeroportos[i].nome)) 
			return 0;
	return 2;
}

BOOL embarcaPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av) {
	DWORD totalBytes;
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!infoPassagPipe->listPassag[i].isFree) {
			if(!_tcscmp(av->aeroOrigem, infoPassagPipe->listPassag[i].passag.aeroOrigem))
				if (!_tcscmp(av->aeroDestino, infoPassagPipe->listPassag[i].passag.aeroDestino)) {
					_tcscpy_s(infoPassagPipe->listPassag[i].passag.fraseInfo,STR_TAM, L"Vou embarcar");
					_tcscpy_s(infoPassagPipe->listPassag[i].passag.nrAviao,STR_TAM,av->procID);
					_tprintf(L"Vou embarcar o passageiro: [%d] nome: [%s]", infoPassagPipe->listPassag[i].passag.idPassag,infoPassagPipe->listPassag[i].passag.nomePassag);
					//// Envia mensagem para o pipe do passageiro, para ser informado que embarcou!
					//fSuccess = WriteFile(infoPassagPipe->, &passagAux, sizeof(passageiro), &totalBytes, &infoPassagPipes->hPipes[indice].oOverLap);
					WriteFile(infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst, 
						&infoPassagPipe->listPassag[i].passag, sizeof(passageiro), &totalBytes,
						&infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].oOverLap);
					//// porque ? :
					//if (fSuccess && totalBytes == sizeof(passageiro))
					//{
					//	infoPassagPipes->hPipes[indice].fPendingIO = FALSE;
					//	infoPassagPipes->hPipes[indice].dwState = READING_STATE;
					//	continue;
					//}

					_tcscpy_s(infoPassagPipe->listPassag[i].passag.fraseInfo,STR_TAM, L"Em Viagem!");
				}
		}
	}
}