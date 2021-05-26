
#include <windows.h>
#include <tchar.h>
#include <fcntl.h> 
#include "Passageiro.h"

#define PIPE_NAME TEXT("\\\\.\\pipe\\teste") 
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


    // Possível mutex para escrita ? visto control e passa escrreverem no pipe
    /*if (!WriteFile(passag.hPipe, &passag, sizeof(passageiro), NULL, NULL)) {
        _tprintf(L"[ERRO] Escrever no pipe! (WriteFile)\n");
        return 1;
    }*/

    // Criar thread para escrever no pipe para terminar
    while (1) {
        _tprintf(L"Vou ficar preso lol\n");
        ret = ReadFile(passag.hPipe, &passag, sizeof(passageiro), &numBytesLidos, NULL);
        //buf[numBytesLidos / sizeof(TCHAR)] = '\0';

        if (!ret || !numBytesLidos) {
            _tprintf(TEXT("[LEITOR] %d %d... (ReadFile)\n"), ret, numBytesLidos);
            break;
        }

        _tprintf(TEXT("[LEITOR] Recebi %d bytes: '%s'... (ReadFile)\n"), numBytesLidos, buf);
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
        _fgetts(passag->fraseTeste, STR_TAM, stdin);
        passag->fraseTeste[_tcslen(passag->fraseTeste) - 1] = '\0';


        //DWORD dwMode = PIPE_READMODE_MESSAGE;
        //SetNamedPipeHandleState(
        //    passag->hPipe,    // pipe handle 
        //    &dwMode,  // new pipe mode 
        //    NULL,     // don't set maximum bytes 
        //    NULL);    // don't set maximum time 

        _tprintf(L"Entrei 1!!!");

        //WaitForSingleObject(pArrData->hMutex, INFINITE);
        WriteFile(passag->hPipe, passag, sizeof(passageiro), &n, NULL);

        //ReleaseMutex(pArrData->hMutex);
    _tprintf(L"Entrei 4!!!");
    } while (_tcscmp(passag->fraseTeste, TEXT("fim")));

    //pArrData->terminar = 1;

    return 0;
}
