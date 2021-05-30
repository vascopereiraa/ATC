
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "../Aviao/Aviao.h"
#include "Controlador.h"
#include "Aviao.h"
#include "Aeroporto.h"
#include "Utils.h"
#include "Passageiro.h"

BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;

	// Start an overlapped connection for this pipe instance. 
	fConnected = ConnectNamedPipe(hPipe, lpo);

	// Overlapped ConnectNamedPipe should return zero. 
	if (fConnected)
	{
		_tprintf(L"ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}

	switch (GetLastError())
	{
		// The overlapped connection in progress. 
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;

		// Client is already connected, so signal an event. 

	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;

		// If an error occurs during the connect operation... 
	default:
	{
		_tprintf(L"ConnectNamedPipe failed with %d.\n", GetLastError());
		return 0;
	}
	}

	return fPendingIO;
}

//void WINAPI threadLeitorNamedPipes(LPVOID);

void WINAPI threadControloBuffer(LPVOID lpParam) {

	infoControlador* dados = (infoControlador*)lpParam;

	controloBufferCirc* bufCirc = dados->bufCirc;
	listaAviao* listaAvioes = dados->listaAvioes;
	aeroporto* listaAeroportos = dados->listaAeroportos;
	int tamAvioes = dados->tamAvioes;
	int tamAeroportos = dados->tamAeroporto;

	int pos = 0;
	aviao aux;
	while (!*(dados->terminaControlador)) {
		// Ler o buffer Circular
		if (WaitForSingleObject(bufCirc->hSemItens, 5000) == WAIT_OBJECT_0) {
			aux = bufCirc->pBuf->buf[bufCirc->pBuf->numCons];
			bufCirc->pBuf->numCons = (bufCirc->pBuf->numCons + 1) % MAX_BUF;

			// Regista aviao OU obtem a sua pos no array
			EnterCriticalSection(&dados->criticalSectionControl);
			if (isNovoAviao(aux, listaAvioes, tamAvioes)) {
				pos = getPrimeiraPosVazia(listaAvioes, tamAvioes);
				if (pos > -1) {
					debug(L"Novo aviao");
					listaAvioes[pos].av = aux;
					listaAvioes[pos].isAlive = TRUE;
					listaAvioes[pos].isFree = FALSE;
					if (*dados->suspendeNovosAvioes) {
						debug(L"Comunicação suspensa!");
						listaAvioes[pos].isFree = TRUE;
						listaAvioes[pos].av.terminaExecucao = 1;
					}
					else {
						if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
							debug(L"Nao foi possivel abrir a mem do av");
							listaAvioes[pos].isFree = TRUE;
						}
					}
				}
			}
			else {
				pos = getIndiceAviao(aux, listaAvioes, tamAvioes);
				if (pos > -1) {
					listaAvioes[pos].av = aux;
					listaAvioes[pos].isAlive = TRUE;
				}
			}

			// Trata dados avioes
			if (listaAvioes[pos].av.terminaExecucao == FALSE) {
				// Preenche coordenadas origem
				if (listaAvioes[pos].av.atuais.posX == -1 && listaAvioes[pos].av.atuais.posY == -1) {
					listaAvioes[pos].av.atuais = obterCoordenadas(listaAvioes[pos].av.aeroOrigem, listaAeroportos, tamAeroportos);
					listaAvioes[pos].av.proxCoord.posX = -1;
					listaAvioes[pos].av.proxCoord.posY = -1;
				}

				if (!_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio")) {
					listaAvioes[pos].av.destino.posX = -1;
					listaAvioes[pos].av.destino.posY = -1;
				}

				// Preenche destino
				if (_tcscmp(listaAvioes[pos].av.aeroDestino, L"vazio")) {
					listaAvioes[pos].av.destino = obterCoordenadas(listaAvioes[pos].av.aeroDestino, listaAeroportos, tamAeroportos);
					if (listaAvioes[pos].av.destino.posX == -2 && listaAvioes[pos].av.destino.posY == -2)
						_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
				}

				if (listaAvioes[pos].av.embarcaPassag) {
					listaAvioes[pos].av.embarcaPassag = FALSE;
					if (!embarcaPassageiros(dados->infoPassagPipes, &listaAvioes[pos].av)) {
						_tprintf(L"Erro ao embarcar os passageiros!\n");
					}
					_tprintf(L"Embarquei passageiros no avião [%d]!\n", listaAvioes[pos].av.procID);
				}

				// Verifica disponibilidade espaco aereo
				if (listaAvioes[pos].av.emViagem == TRUE)
					if (listaAvioes[pos].av.proxCoord.posX > -1 && listaAvioes[pos].av.proxCoord.posY > -1) {
						switch (verificaAvioesPosicao(listaAvioes[pos].av, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
						case 0:		// Pode avancar
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							atualizaCoordPassageiros(dados->infoPassagPipes, &listaAvioes[pos].av);
							InvalidateRect(dados->hWnd, NULL, TRUE);
							break;
						case 1:		// Esta no aeroporto destino
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							_tcscpy_s(listaAvioes[pos].av.aeroOrigem, STR_TAM, listaAvioes[pos].av.aeroDestino);
							_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
							listaAvioes[pos].av.emViagem = FALSE;
							informaPassagDestino(dados->infoPassagPipes, &listaAvioes[pos].av);
							InvalidateRect(dados->hWnd, NULL, TRUE);
							break;
						case 2:		// Posicao esta ocupada
							listaAvioes[pos].av.isSobreposto = TRUE;
							break;
						}
					}
			}

			// Enviar mensagem ao aviao
			*(listaAvioes[pos].memAviao.pAviao) = listaAvioes[pos].av;
			SetEvent(listaAvioes[pos].memAviao.hEvento);

			LeaveCriticalSection(&dados->criticalSectionControl);
		}
	}

	debug(L"Terminei - ThreadControlador");
}

void WINAPI threadTimer(LPVOID lpParam) {
	infoControlador* dados = (infoControlador*)lpParam;
	listaAviao* listaAvioes = dados->listaAvioes;
	int tamAvioes = dados->tamAvioes;

	while (!*(dados->terminaControlador)) {
		Sleep(3000);
		EnterCriticalSection(&dados->criticalSectionControl);
#ifdef TESTES
		for (int i = 1; i < dados->tamAvioes; i++) {
#else
		for (int i = 0; i < dados->tamAvioes; i++) {
#endif
			if (listaAvioes[i].isAlive) {
				_tprintf(L"Aviao: [%i] está vivo !\n", listaAvioes[i].av.procID);
				listaAvioes[i].isAlive = FALSE;
			}
			else {
				if (!listaAvioes[i].isFree) {
					listaAvioes[i].isFree = TRUE;
					encerraMemoriaPartilhada(&listaAvioes[i].memAviao);
				}
			}
		}

		InvalidateRect(dados->hWnd, NULL, TRUE);
		LeaveCriticalSection(&dados->criticalSectionControl);
	}

	debug(L"Terminei - ThreadTimer");
}

DWORD WINAPI threadNamedPipes(LPVOID lpParam) {
	infoControlador* infoControl = (infoControlador*)lpParam;
	InfoPassagPipes* infoPassagPipes = infoControl->infoPassagPipes;
	listaPassag* listPass = infoPassagPipes->listPassag;

	DWORD iResult, totalBytes;
	int indice;
	HANDLE hPipeTemp = NULL;
	HANDLE hThread = NULL;
	HANDLE hEventTemp = NULL;

	EnterCriticalSection(&infoControl->criticalSectionControl);
	// Criação de todos os pipes possíveis a vir a existir
	for (int i = 0; i < MAX_PASSAG; i++) {
		//_tprintf(L"[ESCRITOR] A criar cópia do pipe [%s] .. (CreateNamedPipe)\n", PIPE_NAME);

		hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (hEventTemp == NULL) {
			_tprintf(L"[ERRO] Criação do evento! (CreateEvent)\n");
		}

		hPipeTemp = CreateNamedPipe(PIPE_NAME,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES,
			sizeof(passageiro),
			sizeof(passageiro),
			1000,
			NULL);

		if (hPipeTemp == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("[ERRO] Criar Named Pipe! (CreateNamedPipe)\n"));
			break;
		}

		ZeroMemory(&infoPassagPipes->hPipes[i], sizeof(infoPassagPipes->hPipes[i]));
		infoPassagPipes->hEvents[i] = hEventTemp;
		infoPassagPipes->hPipes[i].hPipeInst = hPipeTemp;
		infoPassagPipes->hPipes[i].oOverLap.hEvent = infoPassagPipes->hEvents[i];

		// Call the subroutine to connect to the new client

		infoPassagPipes->hPipes[i].fPendingIO = ConnectToNewClient(
			infoPassagPipes->hPipes[i].hPipeInst,
			&infoPassagPipes->hPipes[i].oOverLap);

		infoPassagPipes->hPipes[i].dwState = infoPassagPipes->hPipes[i].fPendingIO ?
			CONNECTING_STATE : // still connecting 
			READING_STATE;     // ready to read 
	}
	LeaveCriticalSection(&infoControl->criticalSectionControl);
	/*hThread = CreateThread(NULL, 0, threadLeitorNamedPipes, &arrNamedPipes, 0, NULL);
	if (hThread == NULL) {
		_tprintf(TEXT("[ERRO] Criar a Thread para ler do namedPipe! !\n"));
		return -1;
	}*/

	// Aguardar ligação nova, e atribuir pipe a esse leitor
	//while (!arrNamedPipes->terminar && arrNamedPipes->numPassag < INSTANCES) {
	BOOL fSuccess;
	passageiro passagAux;
	ZeroMemory(&passagAux, sizeof(passageiro));
	while (1) {
		_tprintf(L"[CONTROL] Esperando ligação de um Passageiro. \n");
		// Está a espera que seja aberto nova instancia de pipe
		iResult = WaitForMultipleObjects(MAX_PASSAG, infoPassagPipes->hEvents, FALSE, INFINITE);
		indice = iResult - WAIT_OBJECT_0;
		// Reset
		ResetEvent(infoPassagPipes->hEvents[indice]);
		_tprintf(L"[CONTROL] Evento acionado nr: [%i] \n", indice);
		if (indice < 0 || indice >(MAX_PASSAG - 1))
		{
			_tprintf(L"Index fora do range! \n");
			return 0;
		}
		if (infoPassagPipes->hPipes[indice].fPendingIO) {
			fSuccess = GetOverlappedResult(infoPassagPipes->hPipes[indice].hPipeInst, &infoPassagPipes->hPipes[indice].oOverLap, &totalBytes, FALSE);
			switch (infoPassagPipes->hPipes[indice].dwState) {
			// Aguardar conexão ainda
			case CONNECTING_STATE:
				if (!fSuccess)
				{
					_tprintf(L"Error %d.\n", GetLastError());
					return 0;
				}
				_tprintf(L"\n1º switch: CONNECTING_STATE\n");
				infoPassagPipes->hPipes[indice].dwState = READING_STATE;
				break;
				// Pending read operation 
			case READING_STATE:
				if (!fSuccess || totalBytes == 0)
				{
					_tprintf(L"[ERROR] Reading\n");
					continue;
				}
				_tprintf(L"\n1º switch: READING_STATE\n");
				infoPassagPipes->hPipes[indice].dwState = READING_STATE;
				break;
			case WRITING_STATE:
				infoPassagPipes->hPipes[indice].dwState = WRITING_STATE;
				_tprintf(L"\n1º switch: Writing state.\n");
				break;
			default:
				_tprintf(L"Invalid pipe state.\n");
				return 0;
			}
		}
			// The pipe state determines which operation to do next. 
			switch (infoPassagPipes->hPipes[indice].dwState)
			{
			case WRITING_STATE:
				infoPassagPipes->hPipes[indice].dwState = READING_STATE;
				_tprintf(L"2º switch: Writing state.\n");
				break;
			case READING_STATE:
				fSuccess = ReadFile(infoPassagPipes->hPipes[indice].hPipeInst, &passagAux, sizeof(passageiro), &totalBytes, &infoPassagPipes->hPipes[indice].oOverLap);
				_tprintf(L"2º switch: READING_STATE\n\n");
				
				// Caso seja a 1º conexão, nome ainda não está preenchido
				if (passagAux.nomePassag[0] == '\0') {
					_tprintf(L"\nPrimeiro connect: Sem dados ainda !\n\n");
					break;
				}
				else {
					_tprintf(L"Recebi os seguintes dados:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\n\n", passagAux.idPassag, passagAux.nomePassag, passagAux.aeroOrigem, passagAux.aeroDestino, passagAux.fraseInfo);
				}
				// The read operation is still pending. 
				/*DWORD dwErr = GetLastError();
				if (!fSuccess && (dwErr == ERROR_IO_PENDING))
				{
					infoPassagPipes->hPipes[indice].fPendingIO = TRUE;
					continue;
				}*/
				int pos = -1;
				if (isNovoPassag(passagAux, listPass)) {
					_tprintf(L"Vou criar um passageiro novo: %d\n", passagAux.idPassag);
					pos = getPrimeiraPosVaziaPassag(listPass);
					if (pos > -1) {
						debug(L"Novo Passag");
						listPass[pos].passag = passagAux;
						listPass[pos].passag.indicePipe = indice;
						listPass[pos].isFree = FALSE;
					}
					int existeAero = verificaAeroExiste(passagAux, infoControl->listaAeroportos, infoControl->tamAeroporto);
					// Se não existir aero de origem ou destino, avisa passageiro para ir embora
					if (existeAero != 0) {
						passagAux.sair = existeAero;
						listPass[pos].isFree = TRUE;
						WriteFile(infoPassagPipes->hPipes[indice].hPipeInst, &passagAux, sizeof(passageiro), &totalBytes, &infoPassagPipes->hPipes[indice].oOverLap);
						//_tprintf(L"Não existe o aero [%d] para o passag [%d] \n", existeAero, passagAux.idPassag);
					}
					//_tprintf(L"Existe o aero [%d] para o passag [%d]\n", existeAero, passagAux.idPassag);
				}
				/*else {
					_tprintf(L"Passageiro já existe !\n");
				}*/
				break;
				
			default:
				_tprintf(L"Invalid pipe state.\n");
				return 0;
			}

		LeaveCriticalSection(&infoControl->criticalSectionControl);
	}

	// Disconecta de todos os pipes
	for (int i = 0; i < MAX_PASSAG; i++) {
		if (!DisconnectNamedPipe(infoPassagPipes->hPipes[i].hPipeInst)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe (DisconnectNamedPipe)\n"));
			return -1;
		}
		CloseHandle(infoPassagPipes->hPipes[i].hPipeInst);
		CloseHandle(infoPassagPipes->hEvents[i]);
	}
}

/*
void menu(infoControlador * infoControl) {
	TCHAR comando[STR_TAM], comandoAux[STR_TAM];
	TCHAR* buffer = NULL;
	TCHAR* token = NULL;
*/
	/*
	* aero + nome + coordX + coordY
	* laero = lista aeroportos
	* lavioes = lista avioes
	* lpass = lista passageiros
	* susp = suspender comunicações
	* ret = retomar comunicações
	* end = terminar controlador
	*/
/*
	while (!*(infoControl->terminaControlador)) {
		_tprintf(L"\n\nInsira [cmd] para ver os comandos disponiveis\n");
		_tprintf(L" > ");
		_fgetts(comando, STR_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0';

		_tcscpy_s(&comandoAux, STR_TAM, comando);
		token = _tcstok_s(comando, L" ", &buffer);
		if (token != NULL) {
			EnterCriticalSection(&infoControl->criticalSectionControl);
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
			if (!_tcscmp(token, L"lpass")) {
				_tprintf(L"Lista de passageiros:\n");
				imprimeListaPassag(infoControl->infoPassagPipes->listPassag);
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
					L"ret = retomar comunicações\nlpass = lista os passageiros conectados\nend = terminar controlador\ncmd = comandos disponiveis\n\n");
			}
			LeaveCriticalSection(&infoControl->criticalSectionControl);
		}
	}

#ifdef TESTES
	for (int i = 1; i < infoControl->tamAvioes; ++i) {
#else
	for (int i = 0; i < infoControl->tamAvioes; ++i) {
#endif
		if (!infoControl->listaAvioes[i].isFree) {
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, infoControl->listaAvioes[i].av.procID);
			if (hProcess != NULL) {
				encerraMemoriaPartilhada(&infoControl->listaAvioes[i].memAviao);
				TerminateProcess(hProcess, 1);
			}
		}
	}

}
*/