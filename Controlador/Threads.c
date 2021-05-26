
#include <Windows.h>
#include <tchar.h>
#include <fcntl.h>

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#include "../Aviao/Aviao.h"
#include "Controlador.h"
#include "Aviao.h"
#include "Aeroporto.h"
#include "Utils.h"
#include "../Passageiro/Passageiro.h"

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
					if (!abreMemoriaPartilhada(&listaAvioes[pos])) {
						debug(L"Nao foi possivel abrir a mem do av");
						listaAvioes[pos].isFree = TRUE;
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

				// Verifica disponibilidade espaco aerio
				if (listaAvioes[pos].av.emViagem == TRUE)
					if (listaAvioes[pos].av.proxCoord.posX > -1 && listaAvioes[pos].av.proxCoord.posY > -1) {
						switch (verificaAvioesPosicao(listaAvioes[pos].av, listaAeroportos, tamAeroportos, listaAvioes, tamAvioes)) {
						case 0:		// Pode avancar
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							break;
						case 1:		// Esta no aeroporto destino
							listaAvioes[pos].av.atuais.posX = listaAvioes[pos].av.proxCoord.posX;
							listaAvioes[pos].av.atuais.posY = listaAvioes[pos].av.proxCoord.posY;
							_tcscpy_s(listaAvioes[pos].av.aeroOrigem, STR_TAM, listaAvioes[pos].av.aeroDestino);
							_tcscpy_s(listaAvioes[pos].av.aeroDestino, STR_TAM, L"vazio");
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
		LeaveCriticalSection(&dados->criticalSectionControl);
	}

	debug(L"Terminei - ThreadTimer");
}

void WINAPI threadNamedPipes(LPVOID lpParam) {
	ArrayNamedPipes* arrNamedPipes = (ArrayNamedPipes*)lpParam;
	DWORD iResult, totalBytes;
	int indice;
	TCHAR buf[STR_TAM] = _TEXT("");
	HANDLE hPipeTemp = NULL;
	HANDLE hThread = NULL;
	HANDLE hEventTemp = NULL;


	// Criação de todos os pipes possíveis a vir a existir
	for (int i = 0; i < INSTANCES; i++) {
		_tprintf(L"[ESCRITOR] A criar cópia do pipe [%s] .. (CreateNamedPipe)\n", PIPE_NAME);

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

		ZeroMemory(&arrNamedPipes->hPipes[i], sizeof(arrNamedPipes->hPipes[i]));
		arrNamedPipes->hEvents[i] = hEventTemp;
		arrNamedPipes->hPipes[i].hPipeInst = hPipeTemp;
		arrNamedPipes->hPipes[i].oOverLap.hEvent = arrNamedPipes->hEvents[i];

		// Call the subroutine to connect to the new client

		arrNamedPipes->hPipes[i].fPendingIO = ConnectToNewClient(
			arrNamedPipes->hPipes[i].hPipeInst,
			&arrNamedPipes->hPipes[i].oOverLap);

		arrNamedPipes->hPipes[i].dwState = arrNamedPipes->hPipes[i].fPendingIO ?
											CONNECTING_STATE : // still connecting 
											READING_STATE;     // ready to read 
	}

	/*hThread = CreateThread(NULL, 0, threadLeitorNamedPipes, &arrNamedPipes, 0, NULL);
	if (hThread == NULL) {
		_tprintf(TEXT("[ERRO] Criar a Thread para ler do namedPipe! !\n"));
		return -1;
	}*/

	/*
	ResetEvent(arrData.hEvents[indice]);

            WaitForSingleObject(arrData.hMutex, INFINITE);
            arrData.hPipes[indice].ativo = TRUE;
            arrData.numClientes++;
            ReleaseMutex(arrData.hMutex);
	*/

	// Aguardar ligação nova, e atribuir pipe a esse leitor
	//while (!arrNamedPipes->terminar && arrNamedPipes->numPassag < INSTANCES) {
	BOOL fSuccess;
	passageiro passagAux;
	ZeroMemory(&passagAux, sizeof(passageiro));
	while (1) {
		_tprintf(L"[ESCRITOR] Esperando ligação de um Passageiro. \n");
		// Está a espera que seja aberto nova instancia de pipe
		iResult = WaitForMultipleObjects(INSTANCES, arrNamedPipes->hEvents, FALSE, INFINITE);
		indice = iResult - WAIT_OBJECT_0;
		// Reset
		ResetEvent(arrNamedPipes->hEvents[indice]);
		_tprintf(L"[ESCRITOR] Evento acionado nr: [%i] \n", indice);
		if (indice < 0 || indice > (INSTANCES - 1) )
		{
			_tprintf(L"Index fora do range! \n");
			return 0;
		}
		if (arrNamedPipes->hPipes[indice].fPendingIO) {
			fSuccess = GetOverlappedResult(arrNamedPipes->hPipes[indice].hPipeInst,&arrNamedPipes->hPipes[indice].oOverLap,&totalBytes,FALSE);
			switch (arrNamedPipes->hPipes[indice].dwState) {
					// Aguardar conexão ainda
				case CONNECTING_STATE:
					if (!fSuccess)
					{
						_tprintf(L"Error %d.\n", GetLastError());
						return 0;
					}
					_tprintf(L"\n\n1º switch: CONNECTING_STATE!! ADICIONAR A LISTA AQUI!\n");
					arrNamedPipes->hPipes[indice].dwState = READING_STATE;
					break;
				// Pending read operation 
				case READING_STATE:
					if (!fSuccess || totalBytes == 0)
					{
						_tprintf(L"[ERROR] Reading\n");
						continue;
					}
					_tprintf(L"\n\n1º switch: READING_STATE!!!\n\n");
					//arrNamedPipes->hPipes[indice].dwState = WRITING_STATE;
					arrNamedPipes->hPipes[indice].dwState = READING_STATE;
					break;
					// Pending write operation 
				//case WRITING_STATE:
				//	if (!fSuccess /*|| totalBytes != Pipe[i].cbToWrite*/)
				//	{
				//		_tprintf(L"[ERROR] Writting\n");
				//		continue;
				//	}
				//	_tprintf(L"\n\n1º switch: WRITING_STATE!!!\n\n");
				//	//arrNamedPipes->hPipes[indice].dwState = READING_STATE;
				//	arrNamedPipes->hPipes[indice].dwState = WRITING_STATE;
				//	break;
				default:
					_tprintf(L"Invalid pipe state.\n");
					return 0;
			}
			// The pipe state determines which operation to do next. 
			switch (arrNamedPipes->hPipes[indice].dwState)
			{
			case READING_STATE:
				fSuccess = ReadFile(arrNamedPipes->hPipes[indice].hPipeInst,&passagAux,sizeof(passageiro),&totalBytes,&arrNamedPipes->hPipes[indice].oOverLap);
				_tprintf(L"\n\n2º switch: READING_STATE!!!  [%s]\n\n",passagAux.fraseTeste);
				// The read operation completed successfully. 

				if (fSuccess && totalBytes != 0)
				{
					arrNamedPipes->hPipes[indice].fPendingIO = FALSE;
					arrNamedPipes->hPipes[indice].dwState = WRITING_STATE;
					continue;
				}
				// The read operation is still pending. 
				DWORD dwErr = GetLastError();
				if (!fSuccess && (dwErr == ERROR_IO_PENDING))
				{
					arrNamedPipes->hPipes[indice].fPendingIO = TRUE;
					continue;
				}
				break;
			/*case WRITING_STATE:
				//GetAnswerToRequest(&arrNamedPipes->hPipes[indice]);

				fSuccess = WriteFile(
					arrNamedPipes->hPipes[indice].hPipeInst,
					&passagAux,
					sizeof(passageiro),
					&totalBytes,
					&arrNamedPipes->hPipes[indice].oOverLap);
				_tprintf(L"\n\n2º switch: WRITING_STATE[%s]\n\n", passagAux.fraseTeste);
				// The write operation completed successfully. 

				if (fSuccess && totalBytes == sizeof(passageiro))
				{
					arrNamedPipes->hPipes[indice].fPendingIO = FALSE;
					arrNamedPipes->hPipes[indice].dwState = READING_STATE;
					continue;
				}

				// The write operation is still pending. 

				dwErr = GetLastError();
				if (!fSuccess && (dwErr == ERROR_IO_PENDING))
				{
					arrNamedPipes->hPipes[indice].fPendingIO = TRUE;
					continue;
				}

				// An error occurred; disconnect from the client. 

				//DisconnectAndReconnect(i);
				break;
				*/
				default:
					_tprintf(L"Invalid pipe state.\n");
					return 0;
			}
		}
	}


	// Aguarda termino da thread
	WaitForSingleObject(hThread, INFINITE);
	_tprintf(TEXT("[CONTROL] Desligar os pipes (DisconnectNamedPipe)\n"));
	// Disconecta de todos os pipes
	for (int i = 0; i < arrNamedPipes->numPassag; i++) {
		if (!DisconnectNamedPipe(arrNamedPipes->hPipes[i].hPipeInst)) {
			_tprintf(TEXT("[ERRO] Desligar o pipe (DisconnectNamedPipe)\n"));
			return -1;
		}
		CloseHandle(arrNamedPipes->hPipes[i].hPipeInst);
		CloseHandle(arrNamedPipes->hEvents[i]);
	}
}

/*
void WINAPI threadLeitorNamedPipes(LPVOID lpParam) {
	ArrayNamedPipes* arrNamedPipes = (ArrayNamedPipes*)lpParam;
	//passageiro* passagExistentes = arrNamedPipes->arrPassag;
	BOOL ret = FALSE;
	DWORD numBytesLidos, iResult;
	passageiro passagAux;
	int indice;

	_tprintf(TEXT("[CONTROL] Liguei-me...\n"));
	while (1) {
	_tprintf(TEXT("TESTEEEEE   2...\n"));

	//iResult = WaitForMultipleObjects(INSTANCES, arrNamedPipes->hEvents, FALSE, INFINITE);
	//indice = iResult - WAIT_OBJECT_0;
	
	//_tprintf(L"\n\nIndice: %d", indice);

	ret = ReadFile(arrNamedPipes->hmainPipe, &passagAux, sizeof(passageiro), &numBytesLidos, NULL);
		// Verifica qual é o passageiro
		boolean isNovo = TRUE;
		int pos = 0;
	_tprintf(TEXT("TESTEEEEE   3...\n"));
		for (int i = 0; i < arrNamedPipes->numPassag; i++) {
			// Verifica se passageiro já existe ou não
			if (arrNamedPipes->arrPassag[i].idPassag == passagAux.idPassag) {
	_tprintf(TEXT("TESTEEEEE   4...\n"));
				pos = i;
				isNovo = FALSE;
				break;
			}
		}
	_tprintf(TEXT("TESTEEEEE   5...\n"));
		// Se for passageiro novo insere a lista de passageiros
		if (isNovo) {
			arrNamedPipes->arrPassag[arrNamedPipes->numPassag] = passagAux;
			++(arrNamedPipes->numPassag);
		}

	_tprintf(TEXT("TESTEEEEE   6...\n"));
		
		if (!ret || !numBytesLidos) {
			_tprintf(TEXT("[CONTROL] %d %d... nome: [%s] msg:[%s] (ReadFile)\n"), ret, numBytesLidos, passagAux.nomePassag, passagAux.fraseTeste);
			break;
		}
	_tprintf(TEXT("TESTEEEEE   7...\n"));
		_tprintf(TEXT("[CONTROL] Recebi %d bytes: {nome: '%s' msg: '%s'}(ReadFile)\n"), numBytesLidos, passagAux.nomePassag,passagAux.fraseTeste);
	}
	CloseHandle(arrNamedPipes->hmainPipe);
}
*/

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