
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "../Aviao/MemoriaPartilhada.h"
#include "../Controlador/Utils.h"


BOOL registaEntrada(controloBufferCirc* bufCirc, memoriaPartilhada* memPart, aviao* av) {

	WaitForSingleObject(bufCirc->hSemMutexProd, INFINITE);
		bufCirc->pBuf->buf[bufCirc->pBuf->numProd] = *av;
		bufCirc->pBuf->numProd = (bufCirc->pBuf->numProd + 1) % MAX_BUF;
	ReleaseSemaphore(bufCirc->hSemMutexProd, 1, NULL);
	ReleaseSemaphore(bufCirc->hSemItens, 1, NULL);
	//Sleep(1000);
	WaitForSingleObject(memPart->hEvento, INFINITE);
	if (memPart->pAviao->terminaExecucao)
		return FALSE;
	av->atuais.posX = memPart->pAviao->atuais.posX;
	av->atuais.posY = memPart->pAviao->atuais.posY;
	av->destino.posX = memPart->pAviao->destino.posX;
	av->destino.posY = memPart->pAviao->destino.posY;
	ResetEvent(memPart->hEvento);
	return TRUE;
}

void comunicaAviao(controloBufferCirc* bufCirc, memoriaPartilhada* memPart, aviao* av,infoAviao* infoAv) {
	HMODULE hLib = NULL;
	// Ponteiro para a funcao da DLL
	int (*fcnMove)(int, int, int, int, int*, int*) = NULL;
	// Ligacao Explicita DLL
	hLib = LoadLibrary(_T("SO2_TP_DLL_2021_x64.dll"));
	if (hLib == NULL) {
		_tprintf(_T("Não encontrou a DLL\n"));
		exit(EXIT_FAILURE);
	}
	fcnMove = (int(*)(int, int, int, int, int*, int*)) GetProcAddress(hLib, "move");

	int podeAvancar = 0;
	// flag local para não repetir ciclo se chegar ao destino
	int chegaDestino = 0;
	// infoAv->terminaAviao é para quando o utilizador quer terminar todo o programa
	while (!infoAv->terminaAviao && chegaDestino == 0) {
		// Número de velocidade em "posições por segundo"
		Sleep(1000 / av->velocidade);
		WaitForSingleObject(bufCirc->hSemMutexProd, INFINITE);
		// Se puder avançar 
		if (podeAvancar == 0)
			fcnMove(av->atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY, &av->proxCoord.posX, &av->proxCoord.posY);

		_tprintf(L"\nAtual x: [%i] Atual y: [%i]\tDestino x: [%i] Destino y: [%i]\tProxima x: [%i] Proxima y: [%i]\n",
			av->atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY, av->proxCoord.posX, av->proxCoord.posY);

		bufCirc->pBuf->buf[bufCirc->pBuf->numProd] = *av;
		bufCirc->pBuf->numProd = (bufCirc->pBuf->numProd + 1) % MAX_BUF;
		ReleaseSemaphore(bufCirc->hSemMutexProd, 1, NULL);
		ReleaseSemaphore(bufCirc->hSemItens, 1, NULL);
		// to remove
		Sleep(1000);
		WaitForSingleObject(memPart->hEvento, INFINITE);
		podeAvancar = 0;
		// Verifica se deve ou não terminar Execução
		if (memPart->pAviao->terminaExecucao)
			break;
		else {
			// Se as coordenadas atuais em memória não se alteraram, quer dizer que avião não pode avançar para evitar colisão!
			if (memPart->pAviao->atuais.posX == av->atuais.posX &&
				memPart->pAviao->atuais.posY == av->atuais.posY) {
				debug(L"Estou a espera...");
				int random = rand() % 101;
				if (random < 50) {
					debug(L"Vou esperar 5 segundos para evitar colisão!");
					Sleep(5000);
				}
				else {
					if (av->proxCoord.posX + 1 < 1000) {
						debug(L"Vou virar a direita para evitar colisão!");
						av->proxCoord.posX += 1;
					}
					podeAvancar = 1;
				}
			}
			else // Caso contrário, avião pode avançar e posições foram alteradas !
				*av = *(memPart->pAviao);
		}
		// Verifica se já chegou ao destino !
		if (memPart->pAviao->atuais.posX == av->destino.posX &&
			memPart->pAviao->atuais.posY == av->destino.posY) {
			debug(L"Cheguei ao meu destino!!!");
			wcscpy_s(&av->aeroOrigem, STR_TAM, av->aeroDestino);
			wcscpy_s(&av->aeroDestino, STR_TAM, L"vazio");
			// Reset as coordenadas para o controlador reconhecer que não é a sua 1º viagem
			av->destino.posX = -1;
			av->destino.posY = -1;
			chegaDestino = 1;
		}
		ResetEvent(memPart->hEvento);
	}
}


void menu(infoAviao* lpParam) {
	infoAviao* dados = lpParam;
	dados->terminaAviao = FALSE;
	TCHAR comando[STR_TAM];
	TCHAR* buffer = NULL;
	TCHAR* token = NULL;
	/* Commandos
	* 1 - Definir o destino: dest + "nomeDestino"
	* 2 - Iniciar viagem: start
	* 3 - Terminar a viagem a qualquer momento: end	*/
	while (!dados->terminaAviao) {
		_tprintf(L"\nInsira o comando pretendido: \n");
		_fgetts(comando, STR_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0';

		token = _tcstok_s(comando, L" ", &buffer);
		if (!_tcscmp(token, L"dest") && !_tcscmp(dados->av.aeroDestino, L"vazio")) {
			token = wcstok_s(NULL, L" ", &buffer);
			if (token == NULL) {
				_tprintf(L"Tem que inserir o destino!");
			}
			else {
				wcscpy_s(dados->av.aeroDestino, STR_TAM, token);
				if (!_tcscmp(dados->av.aeroOrigem, dados->av.aeroDestino)) {
					_tprintf(L"\nTem que colocar um destino diferente da origem!\n");
				}else {
					if (!registaEntrada(&dados->bufCirc, &dados->memPart, &dados->av)) {
						_tprintf(L"Vou ter que sair !\n");
						return 1;
					}
					else {
						debug(L"Correu tudo bem, vou continuar !");
					}
				}
			}
		}
		//Certificar que o token não tem lixo para _tcscmp funcionar como deveria!
		if (token != NULL) {
			if (!_tcscmp(token, L"start")) {
				if (!_tcscmp(dados->av.aeroDestino, L"vazio"))
					_tprintf(L"Tem que inserir um destino válido!");
				else {
					debug(L"SetEvent!");
					SetEvent(dados->hEventoViagem);
				}
			}
			if (!_tcscmp(token, L"end")) {
				dados->terminaAviao = TRUE;
				// Caso a thread esteja a espera de iniciar a viagem, vai sair do wait e da thread !
				SetEvent(dados->hEventoViagem);
				return;
			}
		}
	}
}


void WINAPI threadViagem(LPVOID lpParam);

int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif
	srand(time(NULL));
	_tprintf(L"Sou o Aviao!\n");

	infoAviao infoAv;
	infoAv.av.procID = GetCurrentProcessId();
	infoAv.av.terminaExecucao = FALSE;
	wcscpy_s(&infoAv.av.aeroDestino, STR_TAM, L"vazio");
	infoAv.av.atuais.posX = -1;
	infoAv.av.atuais.posY = -1;
	infoAv.av.destino.posX = -1;
	infoAv.av.destino.posY = -1;

	if (argc != 4) {
		fatal(L"Tem que indicar como argumentos: Capacidade máxima, velocidade e aeroporto de origem");
		return 1;
	}
	// Tratamento de argumentos
	_tprintf(L"%d\n", infoAv.av.procID);
	infoAv.av.capMaxima = _ttoi(argv[1]);
	infoAv.av.velocidade = _ttoi(argv[2]);
	_tcscpy_s(infoAv.av.aeroOrigem, STR_TAM, argv[3]);
	_tprintf(L"Cap: %i Vel: %i Aero: %s", infoAv.av.capMaxima, infoAv.av.velocidade, infoAv.av.aeroOrigem);

	// Abrir Memoria Partilhada do Controlador
	if (!abreBufferCircular(&infoAv.bufCirc))
		return 1;

	// Cria memoria partilhada do Avião
	if (!criaMemoriaPartilhada(&infoAv.memPart)) {
		encerraBufferCircular(&infoAv.bufCirc);
		return 1;
	}
	if (!criaEventoViagem(&infoAv)) {
		encerraMemoriaPartilhada(&infoAv.memPart);
		encerraBufferCircular(&infoAv.bufCirc);
		return 1;
	}


	HANDLE hThreadViagem = CreateThread(NULL, 0, threadViagem, (LPVOID)&infoAv, 0, NULL);
	if (hThreadViagem == NULL) {
		// Encerrar tudo que esteja aberto no avião
		encerraMemoriaPartilhada(&infoAv.memPart);
		encerraBufferCircular(&infoAv.bufCirc);
		encerraEventoViagem(&infoAv);
		return 1;
	}

	// Menu de comandos !
	menu(&infoAv);

	WaitForSingleObject(hThreadViagem, INFINITE);
	CloseHandle(hThreadViagem);
	encerraMemoriaPartilhada(&infoAv.memPart);
	encerraBufferCircular(&infoAv.bufCirc);
	return 0;
}
void WINAPI threadViagem(LPVOID lpParam) {
	infoAviao* dados = (infoAviao*)lpParam;

	while (!dados->terminaAviao) {
		if (!WaitForSingleObject(dados->hEventoViagem, INFINITE)) {
			if(!dados->terminaAviao)
				comunicaAviao(&dados->bufCirc, &dados->memPart, &dados->av, dados);
		}
		ResetEvent(dados->hEventoViagem);
	}
}
