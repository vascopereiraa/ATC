
#include <windows.h>
#include <tchar.h>
#include <fcntl.h> 
#include "Passageiro.h"
#include "../Controlador/Constantes.h"
#define MSGTEXT_SZ 256

DWORD WINAPI ThreadEscritor(LPVOID);

int _tmain(int argc, LPTSTR argv[]) {
    TCHAR buf[MSGTEXT_SZ] = TEXT("");
    int i = 0;
    BOOL ret = FALSE;
    DWORD numBytesLidos;

    passageiro passag;
    _tcscpy_s(&passag.aeroOrigem, STR_TAM, L"porto");
    _tcscpy_s(&passag.aeroDestino, STR_TAM, L"lisboa");
    _tcscpy_s(&passag.nomePassag, STR_TAM, L"joao");
    _tcscpy_s(&passag.fraseInfo, STR_TAM, L"frase teste");
    passag.idPassag = GetCurrentProcessId();
    passag.coordAtuais.posX = -1;
    passag.coordAtuais.posY = -1;
    passag.tempoEspera = 10000;

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

    HANDLE hThread = CreateThread(NULL, 0, ThreadEscritor, &passag, 0, NULL);
    if (hThread == NULL) {
        _tprintf(TEXT("[ERRO] Criar a Thread! !\n"));
        return -1;
    }
    // Escrita no pipe para informar controlador sobre a sua existência com envio da sua estrutura
    if (!WriteFile(passag.hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }

    _tprintf(L"Dados do passageiro:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\nIndicePipe: %d\n\n", passag.idPassag, passag.nomePassag, passag.aeroOrigem, passag.aeroDestino,
        passag.fraseInfo, passag.indicePipe);
    // Criar thread para escrever no pipe para terminar
    DWORD fSuccess;
    while (passag.sair) {
        fSuccess = ReadFile(passag.hPipe, &passag, sizeof(passageiro), &numBytesLidos, &oOverlap);
        /*    if (passag.sair != 3)
                _tprintf(L"Erro na leitura do pipe!\n");
            break;
        }*/
        //ret = ReadFile(passag.hPipe, &passag, sizeof(passageiro), &numBytesLidos, NULL);
        /*if (!ret || !numBytesLidos) {
            _tprintf(TEXT("[LEITOR] %d %d... (ReadFile)\n"), ret, numBytesLidos);
            break;
        }*/
        _tprintf(L"\n\nAntes do Evento\n\n");
        if (WaitForSingleObject(hEvent, 3000) != WAIT_TIMEOUT) {

            // if (!fSuccess || numBytesLidos < sizeof(passageiro)) {

            GetOverlappedResult(passag.hPipe, &oOverlap, &numBytesLidos, FALSE);
            _tprintf(L"\n\nDepois do Evento\n\n");

            _tprintf(L"Dados do passageiro:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\nIndicePipe: %d\n\n", passag.idPassag, passag.nomePassag, passag.aeroOrigem, passag.aeroDestino,
                passag.fraseInfo, passag.indicePipe);

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
            if (passag.sair == 3)
                break;

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
        if (passag.sair == 3)
            break;
        // }
        // else
        //     break;
    }


    if (!WriteFile(passag.hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }
    else {
        _tprintf(L"Escrevi no pipe");
    }

    if (passag.sair != 3) {
        DisconnectNamedPipe(passag.hPipe);
        CloseHandle(passag.hPipe);
    }
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
    
    // Close handle para sair do read ?
    passag->sair = 3;

    /*if (!WriteFile(passag->hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }*/

    // Like this ? Fazer writefile a informar da morte do homem ao control ? Meh. check later
    // DisconnectNamedPipe(passag->hPipe);
    /*CloseHandle(passag->hPipe);*/

    return 0;
}
