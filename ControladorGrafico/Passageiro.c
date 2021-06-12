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


void displayInfoPassag(passageiro passagAux) {
	_tprintf(L"\nInformação de passageiro-> IDPassag: %d Nome: %s Destino: %s Origem: %s \n", passagAux.idPassag, passagAux.nomePassag, passagAux.aeroDestino, passagAux.aeroOrigem); Sleep(350);
}

void imprimeListaPassag(const listaPassag* lista) {
	for (int i = 0; i < MAX_PASSAG; ++i) {
		if (lista[i].isFree == FALSE) {
			_tprintf(L"ID: %d\n", lista[i].passag.idPassag);
			_tprintf(L"Nome:: %s\n", lista[i].passag.nomePassag);
			_tprintf(L"Origem: %s\t", lista[i].passag.aeroOrigem);
			_tprintf(L"Destino: %s\n", lista[i].passag.aeroDestino);
			_tprintf(L"Coord x: [%d] Coord y: [%d]\n", lista[i].passag.coordAtuais.posX, lista[i].passag.coordAtuais.posY);
			_tprintf(L"\n");
		}
	}
}


BOOL isNovoPassag(passageiro passag, listaPassag* listPassag) {
	for (int i = 0; i < MAX_PASSAG; ++i) {Sleep(99);
		if (passag.idPassag == listPassag[i].passag.idPassag) {
			return FALSE;
		}
	}
	return TRUE;
}

int getPrimeiraPosVaziaPassag(listaPassag* listPassag) {
	for (int i = 0; i < MAX_PASSAG; ++i) {
		if (listPassag[i].isFree){
			return i;
		}
	}
	return -1;
}

int getPosPassag(passageiro aux, listaPassag* listaPassag) {
	for (int i = 0; i < MAX_PASSAG; ++i) 
		if (aux.idPassag == listaPassag[i].passag.idPassag)
			return i;
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

TCHAR* listaPass(const listaPassag* lista) {
	TCHAR lstAux[500] = _TEXT("");
	TCHAR lstPas[7000] = _TEXT("");
	for (int i = 0; i < MAX_PASSAG; ++i) {
		if (!lista[i].isFree) {
			_stprintf_s(lstAux, 500, L"ID: %d Nome: %s Aero Origem: %s Aero Destino: %s\n",
				lista[i].passag.idPassag, lista[i].passag.nomePassag, lista[i].passag.aeroOrigem, lista[i].passag.aeroDestino);
			_tcscat_s(lstPas, 7000, lstAux);
		}
	}
	if (lstPas[0] == 0) {
		_tcscpy_s(lstPas, 100, L"Não existem Passageiros ainda!");
	}
	return lstPas;
}


void DestroyPassageiros(InfoPassagPipes* infoPassagPipe) {
	DWORD totalBytes;
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!infoPassagPipe->listPassag[i].isFree) {
			infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].dwState = WRITING_STATE;
			infoPassagPipe->listPassag[i].passag.sair = 3;
			// Envia mensagem para o pipe do passageiro, para ser informado que embarcou!
			WriteFile(infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst,
				&infoPassagPipe->listPassag[i].passag, sizeof(passageiro), &totalBytes,
				&infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].oOverLap);
			DisconnectAndReconnect(infoPassagPipe, infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst, infoPassagPipe->listPassag[i].passag);
			infoPassagPipe->listPassag[i].isFree = TRUE;
		}
	}

}

void DisconnectAndReconnect(InfoPassagPipes* infoPassagPipes, int indice, const passageiro PassAux) {

	int pos = getPosPassag(PassAux, infoPassagPipes->listPassag);
	if (pos > -1) {
		infoPassagPipes->listPassag[pos].isFree = TRUE;
		infoPassagPipes->listPassag[pos].passag.sair = 0;
	}
	FlushFileBuffers(infoPassagPipes->hPipes[indice].hPipeInst);
	if (!DisconnectNamedPipe(infoPassagPipes->hPipes[indice].hPipeInst)) {
		_tprintf(TEXT("[ERRO] Desligar o pipe (DisconnectNamedPipe)\n"));
		return;
	}

	infoPassagPipes->hPipes[indice].fPendingIO = ConnectToNewClient(
		infoPassagPipes->hPipes[indice].hPipeInst,
		&infoPassagPipes->hPipes[indice].oOverLap);

	infoPassagPipes->hPipes[indice].dwState = infoPassagPipes->hPipes[indice].fPendingIO ?
		CONNECTING_STATE : // still connecting 
		READING_STATE;     // ready to read
}

BOOL embarcaPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av) {
	DWORD totalBytes;
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!infoPassagPipe->listPassag[i].isFree) {
			if (!_tcscmp(av->aeroOrigem, infoPassagPipe->listPassag[i].passag.aeroOrigem)) {
				if (!_tcscmp(av->aeroDestino, infoPassagPipe->listPassag[i].passag.aeroDestino)) {
					_tcscpy_s(infoPassagPipe->listPassag[i].passag.fraseInfo, STR_TAM, L"Vou embarcar");
					infoPassagPipe->listPassag[i].passag.nrAviaoEmbarcado = av->procID;
					_tprintf(L"\nVou embarcar o passageiro: [%d] nome: [%s]\n", infoPassagPipe->listPassag[i].passag.idPassag, infoPassagPipe->listPassag[i].passag.nomePassag);
					// Atualizar coordenadas para aero origem
					infoPassagPipe->listPassag[i].passag.coordAtuais.posX = av->atuais.posX;
					infoPassagPipe->listPassag[i].passag.coordAtuais.posY = av->atuais.posY;
					// Mudar estado do pipe para WRITING STATE para não ler na thread novamente do pipe
					infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].dwState = WRITING_STATE;
					// Envia mensagem para o pipe do passageiro, para ser informado que embarcou!
					if (!WriteFile(infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst,
						&infoPassagPipe->listPassag[i].passag, sizeof(passageiro), &totalBytes,
						&infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].oOverLap))
					{ // Caso ocorra um erro, passageiro saiu, vai passar a estar livre a posicao dele.
						DisconnectAndReconnect(infoPassagPipe, infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst, infoPassagPipe->listPassag[i].passag);
						infoPassagPipe->listPassag[i].isFree = TRUE;
					}
				}
			}
		}
	}
}


void atualizaCoordPassageiros(InfoPassagPipes* infoPassagPipe, aviao* av) {
	DWORD totalBytes;
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!infoPassagPipe->listPassag[i].isFree) {
			if (infoPassagPipe->listPassag[i].passag.nrAviaoEmbarcado == av->procID) {
				_tprintf(L"\nEm viagem: %d", infoPassagPipe->listPassag[i].passag.idPassag);
				_tcscpy_s(infoPassagPipe->listPassag[i].passag.fraseInfo, STR_TAM, L"Em Viagem");
				// Atualiza coordenadas do passageiro com as do avião.
				infoPassagPipe->listPassag[i].passag.coordAtuais.posX = av->atuais.posX;
				infoPassagPipe->listPassag[i].passag.coordAtuais.posY = av->atuais.posY;
				// Mudar estado do pipe para WRITING STATE para não ler na thread novamente do pipe
				infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].dwState = WRITING_STATE;
				// Escreve no pipe do respetivo passageiro.
				if (!WriteFile(infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst,
					&infoPassagPipe->listPassag[i].passag, sizeof(passageiro), &totalBytes,
					&infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].oOverLap))
				{ // Caso ocorra um erro, passageiro saiu, vai passar a estar livre a posicao dele. 
					DisconnectAndReconnect(infoPassagPipe, infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst, infoPassagPipe->listPassag[i].passag);
					infoPassagPipe->listPassag[i].isFree = TRUE;
				}
			}
		}
	}
}


void informaPassagDestino(InfoPassagPipes* infoPassagPipe, aviao* av) {
	DWORD totalBytes;
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!infoPassagPipe->listPassag[i].isFree) {
			if (infoPassagPipe->listPassag[i].passag.nrAviaoEmbarcado == av->procID) {
				_tcscpy_s(infoPassagPipe->listPassag[i].passag.fraseInfo, STR_TAM, L"Chegou ao destino");
				infoPassagPipe->listPassag[i].passag.nrAviaoEmbarcado = av->procID;
				// Atualizar coordenadas para aero origem
				infoPassagPipe->listPassag[i].passag.coordAtuais.posX = av->atuais.posX;
				infoPassagPipe->listPassag[i].passag.coordAtuais.posY = av->atuais.posY;
				// Mudar estado do pipe para WRITING STATE para não ler na thread novamente do pipe
				infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].dwState = WRITING_STATE;
				//// Envia mensagem para o pipe do passageiro, para ser informado que embarcou!
				if (!WriteFile(infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst,
					&infoPassagPipe->listPassag[i].passag, sizeof(passageiro), &totalBytes,
					&infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].oOverLap))
				{ // Caso ocorra um erro, passageiro saiu, vai passar a estar livre a posicao dele. 
					DisconnectAndReconnect(infoPassagPipe, infoPassagPipe->hPipes[infoPassagPipe->listPassag[i].passag.indicePipe].hPipeInst, infoPassagPipe->listPassag[i].passag);
					infoPassagPipe->listPassag[i].isFree = TRUE;
				}
			}
		}
	}
}

