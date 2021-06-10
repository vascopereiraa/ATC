
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "../Aviao/Aviao.h"
#include "Controlador.h"
#include "Aviao.h"
#include "Aeroporto.h"
#include "Utils.h"
#include "Passageiro.h"

RECT rect = { 0, 0, 100, 100 };

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
					if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
						debug(L"Nao foi possivel abrir a mem do av");
						listaAvioes[pos].isFree = TRUE;
					}
					else {
						if (*dados->suspendeNovosAvioes) {
							debug(L"Comunicação suspensa!");
							listaAvioes[pos].isFree = TRUE;
							listaAvioes[pos].av.terminaExecucao = 1;
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
							InvalidateRect(dados->pintor->hWnd, &rect, TRUE);
							//UpdateWindow(dados->pintor->hWnd);
							break;
						case 1:		// Esta no aeroporto destino
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							_tcscpy_s(listaAvioes[pos].av.aeroOrigem, STR_TAM, listaAvioes[pos].av.aeroDestino);
							_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
							listaAvioes[pos].av.emViagem = FALSE;
							informaPassagDestino(dados->infoPassagPipes, &listaAvioes[pos].av);
							InvalidateRect(dados->pintor->hWnd, &rect, TRUE);
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
	BOOL alteraDisplay = FALSE;

	while (!*(dados->terminaControlador)) {
		alteraDisplay = FALSE;
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
					alteraDisplay = TRUE;
					listaAvioes[i].isFree = TRUE;
					encerraMemoriaPartilhada(&listaAvioes[i].memAviao);
				}
			}
		}
		if (alteraDisplay)
			InvalidateRect(dados->pintor->hWnd, &rect, TRUE);

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
	while (!*(infoControl->terminaControlador)) {
		_tprintf(L"[CONTROL] Esperando ligação de um Passageiro. \n");
		// Está a espera que seja aberto nova instancia de pipe
		//iResult = WaitForMultipleObjects(MAX_PASSAG, infoPassagPipes->hEvents, FALSE, 5000);
		iResult = WaitForMultipleObjects(MAX_PASSAG, infoPassagPipes->hEvents, FALSE, INFINITE);
		if (iResult != WAIT_TIMEOUT) {
			indice = iResult - WAIT_OBJECT_0;
			// Reset
			//ResetEvent(infoPassagPipes->hEvents[indice]);
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
						passagAux.sair = 0;
						WriteFile(infoPassagPipes->hPipes[indice].hPipeInst, &passagAux, sizeof(passageiro), &totalBytes, &infoPassagPipes->hPipes[indice].oOverLap);
						//_tprintf(L"Não existe o aero [%d] para o passag [%d] \n", existeAero, passagAux.idPassag);
					}
					//_tprintf(L"Existe o aero [%d] para o passag [%d]\n", existeAero, passagAux.idPassag);
				}
				else {
					pos = getPosPassag(passagAux, listPass);
					if (pos > -1) {
						if (passagAux.sair == 3) {
							listPass[pos].isFree = TRUE;
							listPass[pos].passag.sair = 0;
							_tprintf(L"Indice: [%d]", listPass[pos].passag.indicePipe);
							FlushFileBuffers(infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].hPipeInst);
							if (!DisconnectNamedPipe(infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].hPipeInst)) {
								_tprintf(TEXT("[ERRO] Desligar o pipe (DisconnectNamedPipe)\n"));
								//ExitProcess(-1);
							}
							else {
								_tprintf(L"Disconnected");
								// Isto deveria de colocar a instância disponivel de novo, no entanto, não funciona :(
								infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].fPendingIO = ConnectToNewClient(
									infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].hPipeInst,
									&infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].oOverLap);

								infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].dwState = infoPassagPipes->hPipes[listPass[pos].passag.indicePipe].fPendingIO ?
									CONNECTING_STATE : // still connecting 
									READING_STATE;     // ready to read
							}
						}
					}
				}
				break;

			default:
				_tprintf(L"Invalid pipe state.\n");
				return 0;
			}

			LeaveCriticalSection(&infoControl->criticalSectionControl);
		}
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