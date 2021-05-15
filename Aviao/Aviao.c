
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>
#include <time.h>

#include "MemoriaPartilhada.h"
#include "../Controlador/Utils.h"
#include "../Controlador/Constantes.h"

typedef int (*fcnDLL)(int, int, int, int, int*, int*);

void imprimeDadosAviao(aviao* av);

fcnDLL carregaDLL(TCHAR* dllLocation) {

	SYSTEM_INFO systemInfo;
	
	GetSystemInfo(&systemInfo);
	switch (systemInfo.wProcessorArchitecture) {
	case PROCESSOR_ARCHITECTURE_AMD64:
		_stprintf_s(dllLocation, STR_TAM, DLL_LOCATION, L"x64");
		break;
	case PROCESSOR_ARCHITECTURE_INTEL:
		_stprintf_s(dllLocation, STR_TAM, DLL_LOCATION, L"x86");
		break;
	default:
		fatal(L"Nao foi possivel carregar a DLL");
		return NULL;
	}
	
	HMODULE hLib = LoadLibrary(dllLocation);
	fcnDLL move = NULL;
	if (hLib == NULL)
		return NULL;
	move = (fcnDLL) GetProcAddress(hLib, "move");
	return move;
}

void libertaDLL(TCHAR* dllLocation) {
	HMODULE hDLL = GetModuleHandle(dllLocation);
	if (hDLL != NULL)
		FreeLibrary(hDLL);
}


DWORD WINAPI threadViagem(LPVOID lpParam) {
	TCHAR dllLocation[STR_TAM];
	
	infoAviao* dados = (infoAviao*)lpParam;
	controloBufferCirc* bufCirc = &dados->bufCirc;
	memoriaPartilhada* memPart = &dados->memPart;
	aviao* av = &dados->av;

	// Carregar a funcao da DLL
	fcnDLL move = carregaDLL(dllLocation);
	if (move == NULL) {
		// Erro ao carregar a DLL
		dados->terminaAviao = TRUE;
		return 1;
	}	

	int resultado = 0, nextPos = 1;
	while (!dados->terminaAviao) {
		// debug(L"Estou a enviar cenas")
		if (av->emViagem) {
			if (nextPos) {
				// Calcula proxima posicao -> FUNCAO DLL
				EnterCriticalSection(&dados->criticalSectionAviao);
				resultado = move((*av).atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY, 
					&av->proxCoord.posX, &av->proxCoord.posY);
				switch (resultado) {
				case 0:			// Chegou ao destino
					av->emViagem = FALSE;
					av->destino.posX = -1;
					av->destino.posY = -1;
					_tprintf(L"\nAviao chegou ao destino!\n");
					break;
				case 1:
					debug(L"O aviao executou uma movimentação correta");
					break;
				case 2:
					debug(L"Ocorreu um erro ao movimentar-se");
					libertaDLL(dllLocation);
					LeaveCriticalSection(&dados->criticalSectionAviao);
					return 2;
					break;
				}
			}
			Sleep(1000 / av->velocidade);	// Aplica a velocidade do aviao em espacos / segundo

#ifdef DEBUG
			_tprintf(L"\nAtual x: [%i] Atual y: [%i]\tDestino x: [%i] Destino y: [%i]\tProxima x: [%i] Proxima y: [%i]\n",
				av->atuais.posX, av->atuais.posY, av->destino.posX, av->destino.posY, av->proxCoord.posX, av->proxCoord.posY);
#endif
			LeaveCriticalSection(&dados->criticalSectionAviao);
		}

#ifdef DEBUG
		EnterCriticalSection(&dados->criticalSectionAviao);
		debug(L"THREAD:");
		imprimeDadosAviao(&dados->av);
		LeaveCriticalSection(&dados->criticalSectionAviao);
#endif

		// Escrever no buffer circular
		WaitForSingleObject(bufCirc->hSemMutexProd, INFINITE);
		
		EnterCriticalSection(&dados->criticalSectionAviao);
		bufCirc->pBuf->buf[bufCirc->pBuf->numProd] = *av;
		bufCirc->pBuf->numProd = (bufCirc->pBuf->numProd + 1) % MAX_BUF;

		ReleaseSemaphore(bufCirc->hSemMutexProd, 1, NULL);
		ReleaseSemaphore(bufCirc->hSemItens, 1, NULL);
		WaitForSingleObject(memPart->hEvento, INFINITE);

		// Ler e tratar a resposta do controlador
		if (WaitForSingleObject(memPart->hEvento, 7000) == WAIT_TIMEOUT) {
			// Caso não obtenha resposta do controlador >> Contrl nao abriu memPart
			erro(L"O Controlador nao conseguiu comunicar de volta!");
			LeaveCriticalSection(&dados->criticalSectionAviao);
			libertaDLL(dllLocation);
			return 3;
		}
		
		CopyMemory(av, memPart->pAviao, sizeof(aviao));

		// Verifica se tem de encerrar
		if (av->terminaExecucao) {
			debug(L"Controlador mandou o aviao encerrar");
			_tprintf(L"Aceitação de novos aviões suspensa no controlador\n");
			libertaDLL(dllLocation);
			LeaveCriticalSection(&dados->criticalSectionAviao);
			return 4;
		}
			
		// Verifica se a movimentação foi feita com sucesso
		if (av->emViagem) {
			nextPos = 1;
			if (av->isSobreposto) {
				av->isSobreposto = FALSE;
				debug(L"A posicao pretendida encontrava-se ocupada!");
				int random = rand() % 101;
				if (random < 50) {
					debug(L"Vou esperar 1 segundos para evitar colisão!");
					Sleep(1000);
				}
				else {
					if (av->proxCoord.posX + 1 < 1000) {
						debug(L"Vou virar a direita para evitar colisão!");
						av->proxCoord.posX += 1;
					}
					nextPos = 0;
				}
			}
		}
		else {
			// O aeroporto origem não existe
			if (av->atuais.posX == -2 && av->atuais.posY == -2) {
				erro(L"O aeroporto origem nao existe!");
				dados->terminaAviao = TRUE;
				libertaDLL(dllLocation);
				LeaveCriticalSection(&dados->criticalSectionAviao);
				return 5;
			}
			if (av->destino.posX == -2 && av->destino.posY == -2) {
				erro(L"O aeroporto destino nao existe!");
			}
		}
		LeaveCriticalSection(&dados->criticalSectionAviao);
		ResetEvent(memPart->hEvento);
	}
	libertaDLL(dllLocation);
	return 0;
}

void imprimeDadosAviao(aviao* av) {
	_tprintf(L"\n");
	_tprintf(L"Avião nr: %d\n", av->procID);
	_tprintf(L"Capacidade: %d\tVelocidade: %d\n", av->capMaxima, av->velocidade);
	_tprintf(L"Origem: %s\n", av->aeroOrigem);
	_tprintf(L"x = %d\ty = %d\n", av->atuais.posX, av->atuais.posY);
	if(!_tcscmp(av->aeroDestino, L"vazio"))
		_tprintf(L"Destino: SEM DESTINO\n");
	else
		_tprintf(L"Destino: %s\n", av->aeroDestino);
	_tprintf(L"x = %d\ty = %d\n", av->destino.posX, av->destino.posY);
	_tprintf(L"\n");
}

void menu(infoAviao* dados) {

	TCHAR comando[STR_TAM];
	TCHAR* buffer = NULL;
	TCHAR* token = NULL;
	
	/*
	* Commandos: 
	* dest + "nomeDestino"		Definir o destino
	* start						Iniciar viagem 
	* end						Terminar a viagem a qualquer momento
	* 
	*/

	while (!dados->terminaAviao) {

		_tprintf(L" > ");
		_fgetts(comando, STR_TAM, stdin);
		comando[_tcslen(comando) - 1] = '\0';
		
		EnterCriticalSection(&dados->criticalSectionAviao);
		token = _tcstok_s(comando, L" ", &buffer);
		
		//Certificar que o token não tem lixo para _tcscmp funcionar como deveria!
		if (token != NULL) {
			if (!_tcscmp(token, L"dest")) {
				token = _tcstok_s(NULL, L" ", &buffer);
				if (token == NULL) {
					_tprintf(L"Tem que inserir um destino válido!");
				}
				else {
					if (!_tcscmp(dados->av.aeroOrigem, token)) {
						erro(L"Tem que colocar um destino diferente da origem!");
					}
					else {
						_tcscpy_s(dados->av.aeroDestino, STR_TAM, token);
					}
				}
			}
		
			if (!_tcscmp(token, L"start")) {
				if (!_tcscmp(dados->av.aeroDestino, L"vazio"))
					_tprintf(L"Tem que inserir um destino válido!");
				else {
					dados->av.emViagem = TRUE;
				}
			}
			if (!_tcscmp(token, L"end")) {
				dados->terminaAviao = TRUE;
				LeaveCriticalSection(&dados->criticalSectionAviao);
				return;
			}
			if (!_tcscmp(token, L"info")) {
				imprimeDadosAviao(&dados->av);
			}
		}
		LeaveCriticalSection(&dados->criticalSectionAviao);
	}
}

int _tmain(int argc, TCHAR* argv[]) {

#ifdef UNICODE
	(void)_setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	srand((unsigned int) time(NULL));

	infoAviao infoAv;
	infoAv.av.procID = GetCurrentProcessId();
	infoAv.av.atuais.posX = -1;
	infoAv.av.atuais.posY = -1;
	infoAv.av.destino.posX = -1;
	infoAv.av.destino.posY = -1;
	infoAv.av.proxCoord.posY = -1;
	infoAv.av.proxCoord.posY = -1;
	infoAv.av.terminaExecucao = FALSE;
	infoAv.av.emViagem = FALSE;
	infoAv.av.isSobreposto = FALSE;
	_tcscpy_s(infoAv.av.aeroDestino, STR_TAM, L"vazio");

	infoAv.terminaAviao = FALSE;

	// Tratamento de argumentos
	if (argc != 4) {
		fatal(L"Tem que indicar como argumentos: Capacidade máxima, velocidade e aeroporto de origem");
		return 1;
	}
	infoAv.av.capMaxima = _ttoi(argv[1]);
	infoAv.av.velocidade = _ttoi(argv[2]);
	_tcscpy_s(infoAv.av.aeroOrigem, STR_TAM, argv[3]);

	// Abrir Memoria Partilhada do Controlador
	if (!abreBufferCircular(&infoAv.bufCirc))
		return 1;

	// Cria memoria partilhada do Avião
	if (!criaMemoriaPartilhada(&infoAv.memPart)) {
		encerraBufferCircular(&infoAv.bufCirc);
		return 1;
	}

	criaCriticalSectionAviao(&infoAv.criticalSectionAviao);

	HANDLE hThreadViagem = CreateThread(NULL, 0, threadViagem, (LPVOID)&infoAv, 0, NULL);
	if (hThreadViagem == NULL) {
		// Encerrar tudo que esteja aberto no avião
		encerraMemoriaPartilhada(&infoAv.memPart);
		encerraBufferCircular(&infoAv.bufCirc);
		encerraCriticalSectionAviao(&infoAv.criticalSectionAviao);
		return 1;
	}

	// Comandos do aviao
	menu(&infoAv);

	WaitForSingleObject(hThreadViagem, INFINITE);
	CloseHandle(hThreadViagem);
	encerraMemoriaPartilhada(&infoAv.memPart);
	encerraBufferCircular(&infoAv.bufCirc); 
	encerraCriticalSectionAviao(&infoAv.criticalSectionAviao);

	TCHAR stringFinal[STR_TAM];
	_stprintf_s(stringFinal, STR_TAM, L"... Aviao %d Encerrado...", infoAv.av.procID);
	fim(stringFinal);
	return 0;
}

