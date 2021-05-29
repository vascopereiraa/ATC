
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
    passag.atuais.posX = -1;
    passag.atuais.posY = -1;
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

    

    // Criar thread para escrever no pipe para terminar
    while (1) {
        ret = ReadFile(passag.hPipe, &passag, sizeof(passageiro), &numBytesLidos, NULL);
        if (!ret || !numBytesLidos) {
            _tprintf(TEXT("[LEITOR] %d %d... (ReadFile)\n"), ret, numBytesLidos);
            break;
        }

        if(passag.sair == 1){
            _tprintf(L"Não existe o aeroporto de Origem!\n");
            //TerminateThread(hThread,NULL);
            //CloseHandle(passag.hPipe);
            //return 0;
        }
        if (passag.sair == 2) {
            _tprintf(L"Não existe o aeroporto de Destino!\n");
            //TerminateThread(hThread, NULL);
            //CloseHandle(passag.hPipe);
            //return 0;
        }
        if (!_tcscmp(passag.fraseInfo, L"Vou embarcar")) {
            _tprintf(L"Passageiro vai embarcar no avião %d", passag.nrAviao);
        }

        _tprintf(TEXT("[LEITOR] Recebi %d bytes... (ReadFile)\n"), numBytesLidos);
        _tprintf(L"Recebi os seguintes dados:\nID: %d\tNome: %s\nOrigem: %s\tDestino: %s\nFrase: %s\n\n", passag.idPassag, passag.nomePassag, passag.aeroOrigem, passag.aeroDestino, passag.fraseInfo);

    }

    CloseHandle(passag.hPipe);
    Sleep(200);
    return 0;
}

DWORD WINAPI ThreadEscritor(LPVOID lparam)
{
    passageiro* passag = (passageiro*)lparam;
    DWORD n = 0;

    do {
        _tprintf(TEXT("[ESCRITOR] Frase: "));
        _fgetts(passag->fraseInfo, STR_TAM, stdin);
        passag->fraseInfo[_tcslen(passag->fraseInfo) - 1] = '\0';
        
        _tprintf(L"Estou a funcionar colega ! Se escreveu fim, fodi-me\n");

    } while (_tcscmp(passag->fraseInfo, TEXT("fim")));


    //DisconnectNamedPipe(passag->hPipe);
    return 0;
}
