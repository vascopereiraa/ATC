
#include <windows.h>
#include <tchar.h>
#include <fcntl.h> 
#include "Passageiro.h"
#include "../Controlador/Constantes.h"
#define MSGTEXT_SZ 256

DWORD WINAPI ThreadEscritor(LPVOID);
DWORD WINAPI ThreadEspera(LPVOID);

int _tmain(int argc, LPTSTR argv[]) {
    TCHAR buf[MSGTEXT_SZ] = TEXT("");
    int i = 0;
    BOOL ret = FALSE;
    DWORD numBytesLidos;

    passageiro passag;
    passag.sairPassag = (int*)malloc(sizeof(int));
    if (passag.sairPassag == NULL) {
        _tprintf(L"[ERRO] Arranque do programa!\n");
        return 10;
    }

    if (argc < 4 || argc > 5) {
        _tprintf(L"[ERRO] Indique como argumentos: Origem, Destino, Nome e opcionalmente o tempo a aguardar\n ");
        return -1;
    }
    _tcscpy_s(passag.aeroOrigem, STR_TAM, argv[1]);
    _tcscpy_s(passag.aeroDestino, STR_TAM, argv[2]);
    _tcscpy_s(passag.nomePassag, STR_TAM, argv[3]);
    if(argc == 5)
       passag.tempoEspera = _ttoi(argv[4]);
    else
       passag.tempoEspera = 0;

    passag.idPassag = GetCurrentProcessId();
    passag.coordAtuais.posX = -1;
    passag.coordAtuais.posY = -1;
    *(passag.sairPassag) = 0;
    passag.sair = 0;

#ifdef UNICODE 
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
#endif 

    _tprintf(TEXT("[LEITOR] Esperar pelo pipe '%s' (WaitNamedPipe)\n"), PIPE_NAME);

    // Espera que o pipe seja criado.
    // WaitNamedPipe( nome do pipe, tempo de espera);
    if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_WAIT_FOREVER)) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (WaitNamedPipe)\n"), PIPE_NAME);
        exit(-1);
    }

    OVERLAPPED oOverlap;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL)
        exit(-1);
    ZeroMemory(&oOverlap, sizeof(OVERLAPPED));
    ResetEvent(hEvent);
    oOverlap.hEvent = hEvent;

    _tprintf(TEXT("[LEITOR] Ligação ao pipe do controlador... (CreateFile)\n"));
    passag.hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    if (passag.hPipe == NULL) {
        _tprintf(TEXT("[ERRO] Ligar ao pipe '%s'! (CreateFile)\n"), PIPE_NAME);
        exit(-1);
    }
    _tprintf(TEXT("[LEITOR] Liguei-me...\n"));

    DWORD threadID;
    HANDLE hThread = CreateThread(NULL, 0, ThreadEscritor, &passag, 0, &threadID);
    if (hThread == NULL) {
        _tprintf(TEXT("[ERRO] Criar a Thread! !\n"));
        return -1;
    }
    HANDLE hThread_Espera = NULL;
    if (argc == 5) {
        hThread_Espera = CreateThread(NULL, 0, ThreadEspera, &passag, 0, NULL);
        if (hThread_Espera == NULL) {
            _tprintf(TEXT("[ERRO] Criar a Thread! !\n"));
            return -1;
        }
    }

    // Escrita no pipe para informar controlador sobre a sua existência com envio da sua estrutura
    if (!WriteFile(passag.hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }

   // _tprintf(L"Dados do passageiro:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\nIndicePipe: %d\n\n", passag.idPassag, passag.nomePassag, passag.aeroOrigem, passag.aeroDestino,
     //   passag.fraseInfo, passag.indicePipe);

    // Criar thread para escrever no pipe para terminar
    DWORD fSuccess;
    while (passag.sair != 3 && *(passag.sairPassag) != 1) {
        ReadFile(passag.hPipe, &passag, sizeof(passageiro), &numBytesLidos, &oOverlap);
        if (WaitForSingleObject(hEvent, 6000) != WAIT_TIMEOUT) {
            GetOverlappedResult(passag.hPipe, &oOverlap, &numBytesLidos, FALSE);
           // _tprintf(L"Dados do passageiro:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\nIndicePipe: %d\n Tempo %d\n", passag.idPassag, passag.nomePassag, passag.aeroOrigem, passag.aeroDestino,
             //   passag.fraseInfo, passag.indicePipe,passag.tempoEspera);

            if (passag.sair == 1) {
                _tprintf(L"Não existe o aeroporto de Origem!\n");
                TerminateThread(hThread, NULL);
                break;
            }
            if (passag.sair == 2) {
                _tprintf(L"Não existe o aeroporto de Destino!\n");
                TerminateThread(hThread, NULL);
                break;
            }
            if (passag.sair == 3 || passag.sairPassag == 1) {
                 break;
            }

            if (passag.sairPassag == 0 && argc == 5) {
                TerminateThread(hThread_Espera, NULL);
            }

            if (!_tcscmp(passag.fraseInfo, L"Vou embarcar")) {
                _tprintf(L"Passageiro vai embarcar no avião %d nas coordenadas x: [%d] y: [%d]\n", passag.nrAviaoEmbarcado, passag.coordAtuais.posX, passag.coordAtuais.posY);
            }
            if (!_tcscmp(passag.fraseInfo, L"Em Viagem")) {
                _tprintf(L"Em viagem nas Coord x: [%d] Coord y: [%d]\n", passag.coordAtuais.posX, passag.coordAtuais.posY);
            }
            if (!_tcscmp(passag.fraseInfo, L"Chegou ao destino")) {
                _tprintf(L"Passageiro chegou ao destino [%s] com coordenadas x: [%d] y: [%d] no avião %d", passag.aeroDestino, passag.nrAviaoEmbarcado, passag.coordAtuais.posX, passag.coordAtuais.posY);
                TerminateThread(hThread, NULL);
                break;
            }
        }
        if (passag.sair == 3 || passag.sairPassag == 1) {
            // _tprintf(L"\n\nEntrei 5\n\n");
            break;
        }
    }


    if (!WriteFile(passag.hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }

    CloseHandle(passag.hPipe);
    return 0;
}

DWORD WINAPI ThreadEscritor(LPVOID lparam)
{
    passageiro* passag = (passageiro*)lparam;
    do {
        _tprintf(TEXT("Escreva \"fim\" para terminar a qualquer momento: "));
        _fgetts(passag->fraseInfo, STR_TAM, stdin);
        passag->fraseInfo[_tcslen(passag->fraseInfo) - 1] = '\0';
    } while (_tcscmp(passag->fraseInfo, TEXT("fim")));
    
    passag->sair = 3;
    *(passag->sairPassag) = 1;
    return 0;
}


DWORD WINAPI ThreadEspera(LPVOID lparam)
{
    passageiro* passag = (passageiro*)lparam;
    Sleep(passag->tempoEspera);
    *(passag->sairPassag) = 1;
    
    return 0;
}
