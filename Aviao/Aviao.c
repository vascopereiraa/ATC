
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>

#include "MemoriaPartilhada.h"
#include "../Controlador/Utils.h"

void WINAPI threadEscritaLeitura(LPVOID lpParam);


int _tmain(int argc, TCHAR* argv[]) {
#ifdef UNICODE
	(void) _setmode(_fileno(stdin), _O_WTEXT);
	(void) _setmode(_fileno(stdout), _O_WTEXT);
	(void) _setmode(_fileno(stderr), _O_WTEXT);
#endif

	srand(time(NULL));
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


	_tprintf(L"Sou o Aviao!\n");

	// Abrir Memoria Partilhada do Controlador
	controloBufferCirc bufCirc;
	if (!abreBufferCircular(&bufCirc))
		return 1;

	// Cria memoria partilhada do Avião
	memoriaPartilhada memPart;
	if (!criaMemoriaPartilhada(&memPart)) {
		encerraBufferCircular(&bufCirc);
		return 1;
	}

	aviao av;
	av.procID = GetCurrentProcessId();
	av.terminaExecucao = FALSE;
	av.atuais.posX = -1;
	av.atuais.posY = -1;
	av.destino.posX = -1;
	av.destino.posY = -1;
	// Tratamento de argumentos
	_tprintf(L"%d\n", av.procID);
	av.capMaxima = _ttoi(argv[1]);
	av.velocidade = _ttoi(argv[2]);
	_tcscpy_s(av.aeroOrigem, STR_TAM, argv[3]);
	_tprintf(L"Cap: %i Vel: %i Aero: %s", av.capMaxima, av.velocidade, av.aeroOrigem);

	_tprintf(L"\nIndique o destino: ");
	_tscanf_s(L"%s", av.aeroDestino, STR_TAM);
	av.aeroDestino[_tcslen(av.aeroDestino)] = '\0';



	WaitForSingleObject(bufCirc.hSemMutexProd, INFINITE);
		bufCirc.pBuf->buf[bufCirc.pBuf->numProd] = av;
		bufCirc.pBuf->numProd = (bufCirc.pBuf->numProd + 1) % MAX_BUF;
	ReleaseSemaphore(bufCirc.hSemMutexProd, 1, NULL);
	ReleaseSemaphore(bufCirc.hSemItens, 1, NULL);
	Sleep(2000);
	WaitForSingleObject(memPart.hEvento, INFINITE);

		if (memPart.pAviao->terminaExecucao)
			return;
	
		av.atuais.posX = memPart.pAviao->atuais.posX;
		av.atuais.posY = memPart.pAviao->atuais.posY;
		av.destino.posX = memPart.pAviao->destino.posX;
		av.destino.posY = memPart.pAviao->destino.posY;
	ResetEvent(memPart.hEvento);

	int weirdFlag = 0;
	while (1) {
		// Número de velocidade em "posições por segundo"
		Sleep(1000 / av.velocidade);
		WaitForSingleObject(bufCirc.hSemMutexProd, INFINITE);
		
		if (weirdFlag == 0) {
			fcnMove(av.atuais.posX, av.atuais.posY, av.destino.posX, av.destino.posY, &av.proxCoord.posX, &av.proxCoord.posY);
		}
		
		_tprintf(L"\nAtual x: [%i] Atual y: [%i]\nDestino x: [%i] Destino y: [%i]\nProxima x: [%i] Proxima y: [%i]\n",
			av.atuais.posX, av.atuais.posY, av.destino.posX, av.destino.posY, av.proxCoord.posX, av.proxCoord.posY);

		bufCirc.pBuf->buf[bufCirc.pBuf->numProd] = av;
		bufCirc.pBuf->numProd = (bufCirc.pBuf->numProd + 1) % MAX_BUF;

		ReleaseSemaphore(bufCirc.hSemMutexProd, 1, NULL);
		ReleaseSemaphore(bufCirc.hSemItens, 1, NULL);
		Sleep(2000);
		WaitForSingleObject(memPart.hEvento, INFINITE);

		/*_tprintf(L"\nmemPart Atual x: [%i] memPartAtual y: [%i]\nmemPartDestino x: [%i] memPartDestino y: [%i]\nmemPartProxima x: [%i] memPartProxima y: [%i]\n",
			memPart.pAviao->atuais.posX, memPart.pAviao->atuais.posY, memPart.pAviao->destino.posX, memPart.pAviao->destino.posY, memPart.pAviao->proxCoord.posX, memPart.pAviao->proxCoord.posY);
		fflush(stdout);*/
		weirdFlag = 0;
		// Verifica se deve ou não terminar Execução
		if (memPart.pAviao->terminaExecucao) {
			break;
		}
		else {
			if (memPart.pAviao->atuais.posX == av.atuais.posX &&
				memPart.pAviao->atuais.posY == av.atuais.posY) {
				debug(L"Estou a espera...");
				int random = rand() % 101;
				_tprintf(L"\nValor random: [%i]", random);
				if (random < 50) {
					Sleep(5000);
				}
				else {
					if (av.proxCoord.posX + 1 < 1000) {
						debug(L"Vou virar a direita lol");
						av.proxCoord.posX += 1;
					}
					weirdFlag = 1;
				}
			}
			else {
				av = *(memPart.pAviao);
			}
		}
		if(memPart.pAviao->atuais.posX == av.destino.posX &&
		   memPart.pAviao->atuais.posY == av.destino.posY) {
			debug(L"Cheguei ao meu destino!!!");
			break;
		}

		ResetEvent(memPart.hEvento);
	}

	encerraBufferCircular(&bufCirc);
	return 0;
}



void WINAPI threadEscritaLeitura(LPVOID lpParam) {

}

