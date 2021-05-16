
#include <Windows.h>
#include <fcntl.h>
#include <tchar.h>
#include <time.h>

#include "Comunicacao.h"
#include "Aviao.h"
#include "../Controlador/Utils.h"
#include "../Controlador/Constantes.h"
#include "MemoriaPartilhada.h"

void imprimeDadosAviao(aviao* av);
void menu(infoAviao* dados);

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

void imprimeDadosAviao(aviao* av) {
	_tprintf(L"\n");
	_tprintf(L"Avião nr: %d\n", av->procID);
	_tprintf(L"Capacidade: %d\tVelocidade: %d\n", av->capMaxima, av->velocidade);
	_tprintf(L"Origem: %s\n", av->aeroOrigem);
	_tprintf(L"x = %d\ty = %d\n", av->atuais.posX, av->atuais.posY);
	if (!_tcscmp(av->aeroDestino, L"vazio"))
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
