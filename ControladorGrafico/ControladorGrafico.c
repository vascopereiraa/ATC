
#include "framework.h"
#include "ControladorGrafico.h"
#include <fcntl.h>

#include "Controlador.h"
#include "Utils.h"
#include "Constantes.h"
#include "Passageiro.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int, infoControlador*);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    CriarAeroporto(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ListarAeroportos(HWND, UINT, WPARAM, LPARAM);


int displayInfo(HWND hWnd, infoControlador* dados);
int displayInfoBitBlt(HWND hWnd, infoControlador* dados, const int* indice);
int infoAviao(infoControlador* dados);
void verificaExistenciaAero(HWND hWnd, infoControlador* dados);
void verificaExistenciaAviao(HWND hWnd, infoControlador* dados, const int* indice);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
#ifdef UNICODE
    (void)_setmode(_fileno(stdin), _O_WTEXT);
    (void)_setmode(_fileno(stdout), _O_WTEXT);
    (void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

    infoControlador infoControl;
    ZeroMemory(&infoControl, sizeof(infoControlador));

    // Meter num sitio mais bonito
    infoControl.pintor = (Pintor*)malloc(sizeof(Pintor));
    if (infoControl.pintor == NULL) {
        fatal(L"Ocorreu um erro ao criar a lista de avioes");
        return NULL;
    }
    infoControl.pintor->hBmp = NULL;
    infoControl.pintor->hdc = NULL;
    infoControl.pintor->memDC = NULL;
    infoControl.pintor->hWnd = NULL;

    // Inicializa a lista de Passageiros e namedpipes
    InfoPassagPipes* infoPassagPipes = inicializaListaPassagPipes();
    if (infoPassagPipes == NULL) {
        return 1;
    }
    infoControl.infoPassagPipes = infoPassagPipes;

    // Variaveis de controlo
    infoControl.terminaControlador = (int*)malloc(sizeof(int));
    if (infoControl.terminaControlador == NULL) {
        encerraControlador(&infoControl);
        return 1;
    }
    infoControl.suspendeNovosAvioes = (int*)malloc(sizeof(int));
    if (infoControl.suspendeNovosAvioes == NULL) {
        encerraControlador(&infoControl);
        return 1;
    }

    *infoControl.suspendeNovosAvioes = 0;
    *infoControl.terminaControlador = 0;

    // Carrega dados do Registry
    if (!controladorRegistry(&infoControl.tamAeroporto, &infoControl.tamAvioes)) {
        encerraControlador(&infoControl);
        return 1;
    }

    _tprintf(L"\nValores do registry:\n Número máximo de Aeroportos => %d\n Número máximo de Aviões => %d\n", infoControl.tamAeroporto, infoControl.tamAvioes);

    // Inicializa a lista de Aeroportos
    aeroporto* aeroportos = inicializaListaAeroportos(infoControl.tamAeroporto);
    if (aeroportos == NULL) {
        encerraControlador(&infoControl);
        return 1;
    }
    infoControl.listaAeroportos = aeroportos;

#ifdef DEBUG
    _tcscpy_s(&infoControl.listaAeroportos[0].nome, STR_TAM, L"porto");
    infoControl.listaAeroportos[0].localizacao.posX = 0;
    infoControl.listaAeroportos[0].localizacao.posY = 0;

    _tcscpy_s(&infoControl.listaAeroportos[1].nome, STR_TAM, L"lisboa");
    infoControl.listaAeroportos[1].localizacao.posX = 50;
    infoControl.listaAeroportos[1].localizacao.posY = 50;

    _tcscpy_s(&infoControl.listaAeroportos[2].nome, STR_TAM, L"faro");
    infoControl.listaAeroportos[2].localizacao.posX = 500;
    infoControl.listaAeroportos[2].localizacao.posY = 500;

    infoControl.indiceAero = 3;
#endif

    // Inicializa a lista de Avioes
    listaAviao* avioes = inicializaListaAviao(infoControl.tamAvioes);
    if (avioes == NULL) {
        encerraControlador(&infoControl);
        return 1;
    }
    infoControl.listaAvioes = avioes;

#ifdef TESTES
    infoControl.listaAvioes[0].isFree = FALSE;
    infoControl.listaAvioes[0].av.procID = 1;
    infoControl.listaAvioes[0].av.atuais.posX = 5;
    infoControl.listaAvioes[0].av.atuais.posY = 5;
#endif

    // Criar Memoria Partilhada do Controlador
    controloBufferCirc bufCirc;
    if (!criaBufferCircular(&bufCirc)) {
        encerraControlador(&infoControl);
        return 1;
    }
    infoControl.bufCirc = &bufCirc;

    criaCriticalSectionControl(&infoControl.criticalSectionControl);

    // Criar a Thread para gerenciar o buffer circular
    HANDLE hThreadBuffer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadControloBuffer, (LPVOID)&infoControl, 0, NULL);
    if (hThreadBuffer == NULL) {
        encerraControlador(&infoControl);
        return 1;
    }

    // Cria a Thread de Timer para a reação às respostas dos avioes
    HANDLE hThreadTimer = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadTimer, (LPVOID)&infoControl, 0, NULL);
    if (hThreadTimer == NULL) {
        *infoControl.terminaControlador = 1;
        WaitForSingleObject(hThreadBuffer, INFINITE);
        CloseHandle(hThreadBuffer);
        encerraControlador(&infoControl);
        return 1;
    }

    // Pipes
    HANDLE hThreadNamedPipes = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadNamedPipes, (LPVOID)&infoControl, 0, NULL);
    if (hThreadTimer == NULL) {
        *infoControl.terminaControlador = 1;
        WaitForSingleObject(hThreadBuffer, INFINITE);
        WaitForSingleObject(hThreadTimer, INFINITE);
        CloseHandle(hThreadTimer);
        CloseHandle(hThreadBuffer);
        encerraControlador(&infoControl);
        return 1;
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CONTROLADORGRAFICO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance, &infoControl);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow, &infoControl))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CONTROLADORGRAFICO));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    WaitForSingleObject(hThreadBuffer, INFINITE);
    WaitForSingleObject(hThreadTimer, INFINITE);
    WaitForSingleObject(hThreadNamedPipes, INFINITE);
    CloseHandle(hThreadBuffer);
    CloseHandle(hThreadTimer);
    CloseHandle(hThreadNamedPipes);
    encerraControlador(&infoControl);

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = sizeof(infoControlador*);
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONTROLADORGRAFICO));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CONTROLADORGRAFICO);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, infoControlador* infoControl)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 1025, 1075, NULL, NULL, hInstance, infoControl);

   if (!hWnd)
   {
      return FALSE;
   }

   SetWindowLongPtr(hWnd, 0, (LONG_PTR) infoControl);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    infoControlador* dados = (infoControlador*)GetWindowLongPtr(hWnd, 0);
    // infoControlador* dados = (infoControlador*)lParam;
    TCHAR listaAux[7000] = _TEXT(" ");
    int indice = -1;
 
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_CRIARAEROPORTO:
                DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CRIARAEROPORTO), hWnd, CriarAeroporto, dados);
                break;
            /*case IDM_LISTAR_AEROPORTOS:
                _tcscpy_s(listaAux, 7000, listaAero(dados->listaAeroportos, dados->indiceAero));
                MessageBox(hWnd, listaAux, L"Listagem de Aeroportos", MB_OK);
                break;*/
            case IDM_LISTAR_AEROPORTOS:
                DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_LISTA_AEROPORTOS), hWnd, ListarAeroportos, dados);
                break;
            case IDM_LISTAR_AVIOES:
               _tcscpy_s(listaAux, 7000, listaAv(dados->listaAvioes,dados->tamAvioes));
                MessageBox(hWnd, listaAux, L"Listagem de Avioes", MB_OK);
                break;
            case IDM_LISTAR_PASSAGEIROS:
                _tcscpy_s(listaAux, 7000, listaPass(dados->infoPassagPipes->listPassag));
                MessageBox(hWnd, listaAux, L"Listagem de Passageiros", MB_OK);
                break;

            case IDM_RETOMAR:
            {
                EnterCriticalSection(&dados->criticalSectionControl);
                *(dados->suspendeNovosAvioes) = 0;
                _stprintf_s(listaAux, 10, L"%d", *(dados->suspendeNovosAvioes));
                MessageBox(hWnd, listaAux, L"Listagem de Passageiros", MB_OK);
                LeaveCriticalSection(&dados->criticalSectionControl);
                break;
            }
            case IDM_SUSPENDER:
            {
                EnterCriticalSection(&dados->criticalSectionControl);
                *(dados->suspendeNovosAvioes) = 1;
                _stprintf_s(listaAux, 10, L"%d", *(dados->suspendeNovosAvioes));
                MessageBox(hWnd, listaAux, L"Listagem de Passageiros", MB_OK);
                LeaveCriticalSection(&dados->criticalSectionControl);
                break;
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_LBUTTONDOWN:
    {
        dados->pintor->xPos = LOWORD(lParam);
        dados->pintor->yPos = HIWORD(lParam);
        EnterCriticalSection(&dados->criticalSectionControl);
        dados->pintor->descAero = TRUE;
        LeaveCriticalSection(&dados->criticalSectionControl);
        RECT rect = { dados->pintor->xPos - 400, dados->pintor->yPos + 400,  dados->pintor->xPos + 400, dados->pintor->yPos - 400 };
        InvalidateRect(hWnd, &rect, TRUE);
        break;
    }
    case WM_MOUSEMOVE:
    {
        dados->pintor->xPos = LOWORD(lParam);
        dados->pintor->yPos = HIWORD(lParam);
        EnterCriticalSection(&dados->criticalSectionControl);
        dados->pintor->descAviao = TRUE;
        LeaveCriticalSection(&dados->criticalSectionControl);
        RECT rect = { dados->pintor->xPos - 400, dados->pintor->yPos + 400,  dados->pintor->xPos + 400, dados->pintor->yPos - 400 };
        InvalidateRect(hWnd, &rect, TRUE);
        break;
    }
    case WM_PAINT:
        displayInfoBitBlt(hWnd, dados, &indice);
        break;
    case WM_ERASEBKGND:
        break;
    case WM_CLOSE:
        *(dados->terminaControlador) = 1;
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        EnterCriticalSection(&dados->criticalSectionControl);
        if (dados->pintor->memDC != NULL) {
            SelectObject(dados->pintor->memDC, NULL);
            DeleteObject(dados->pintor->memDC);
            dados->pintor->hdc = GetDC(hWnd);
            dados->pintor->memDC = CreateCompatibleBitmap(dados->pintor->hdc, LOWORD(lParam), HIWORD(lParam));
            ReleaseDC(hWnd, dados->pintor->hdc);
            SelectObject(dados->pintor->memDC, dados->pintor->memDC);
        }
        LeaveCriticalSection(&dados->criticalSectionControl);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Message handler for CriarAeroporto box.
INT_PTR CALLBACK CriarAeroporto(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    TCHAR nome[STR_TAM] = _TEXT("");
    int posX;
    int posY;
    TCHAR help[512] = _TEXT("");
    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_CRIAR:
        {
            infoControlador* dados = (infoControlador*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            GetDlgItemText(hDlg, MAKEINTRESOURCE(IDC_NOMEAERO), nome, STR_TAM);
            posX = GetDlgItemInt(hDlg, MAKEINTRESOURCE(IDC_COORDX), NULL, FALSE);
            posY = GetDlgItemInt(hDlg, MAKEINTRESOURCE(IDC_COORDY), NULL, FALSE);
            EnterCriticalSection(&dados->criticalSectionControl);
            if (adicionaAero(dados->listaAeroportos, &dados->indiceAero, nome, &posX, &posY)) {
                _stprintf_s(help, 512, L"nome = %s\tx = %d\ty = %d", dados->listaAeroportos[dados->indiceAero - 1].nome,
                    dados->listaAeroportos[dados->indiceAero - 1].localizacao.posX, dados->listaAeroportos[dados->indiceAero - 1].localizacao.posY);
                MessageBox(hDlg, help, L"AERO ADD", MB_OK);
            }
            else {
                MessageBox(hDlg, L"ERROR", L"[ERROR]", MB_OK);
                break;
            }
            LeaveCriticalSection(&dados->criticalSectionControl);
        }
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

// Message handler for CriarAeroporto box.
INT_PTR CALLBACK ListarAeroportos(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        infoControlador* dados = (infoControlador*)lParam;
        HWND hWndListBox = GetDlgItem(hDlg, IDC_LISTBOX_AERO);
        for (int i = 0; i < dados->indiceAero; ++i) {
            int pos = (int)SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)dados->listaAeroportos[i].nome);
            SendMessage(hWndListBox, LB_SETITEMDATA, pos, (LPARAM)i);
        }
        SetFocus(hWndListBox);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDCANCEL:
        case IDC_LAERO_SAIR:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_LIST_AEROPORTOS:
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
            {
                infoControlador* dados = (infoControlador*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
                HWND hWndListBox = GetDlgItem(hDlg, IDC_LISTBOX_AERO);
                int item = (int)SendMessage(hWndListBox, LB_GETCURSEL, 0, 0);
                int i = (int)SendMessage(hWndListBox, LB_GETITEMDATA, item, 0);
                TCHAR string[STR_TAM] = _TEXT("");
                _stprintf_s(string, STR_TAM, L"Nome: %s\nCoordendas:\nx = %d\ty = %d\n",
                    dados->listaAeroportos[i].nome, dados->listaAeroportos[i].localizacao.posX, dados->listaAeroportos[i].localizacao.posY);
                SetDlgItemText(hDlg, IDC_INFO_AEROPORTO, string);
            }
            }
            return (INT_PTR)TRUE;
        }
    }
    return (INT_PTR)FALSE;
}

int infoAviao(infoControlador* dados) {
    for (int i = 0; i < dados->indiceAero; ++i) 
        if ((dados->pintor->xPos - dados->listaAvioes[i].av.atuais.posX) < 10 && (dados->pintor->xPos - dados->listaAvioes[i].av.atuais.posX) > -10) 
            if ((dados->pintor->yPos - dados->listaAvioes[i].av.atuais.posY) < 10 && (dados->pintor->yPos - dados->listaAvioes[i].av.atuais.posY) > -10) 
                return i;
    return -1;
}

void verificaExistenciaAero(HWND hWnd, infoControlador* dados) {
    RECT rectClientWindow = { 0, 0, 0, 0 };
    SIZE sizeTextLogicalSize = { 0, 0 };
    PAINTSTRUCT ps;
    for (int i = 0; i < dados->indiceAero; ++i) {
        if ((dados->pintor->xPos - dados->listaAeroportos[i].localizacao.posX) < 10 && (dados->pintor->xPos - dados->listaAeroportos[i].localizacao.posX) > -10) {
            if ((dados->pintor->yPos - dados->listaAeroportos[i].localizacao.posY) < 10 && (dados->pintor->yPos - dados->listaAeroportos[i].localizacao.posY) > -10) {
                TCHAR infoAero[100] = _TEXT(" ");
                _stprintf_s(infoAero, 100, _TEXT("Aeroporto: %s\n Localização x: %d y: %d\n"),dados->listaAeroportos[i].nome, dados->listaAeroportos[i].localizacao.posX, dados->listaAeroportos[i].localizacao.posY);
                // Função diz qual o tamanho da string em pixeis, valor fica em sizeTextLogicalSize
                GetTextExtentPoint32(dados->pintor->memDC, infoAero, _tcslen(infoAero) , &sizeTextLogicalSize);
                if(dados->pintor->xPos < 500)
                    TextOut(dados->pintor->memDC,dados->pintor->xPos + 15,dados->pintor->yPos + 20, infoAero,_tcslen(infoAero));
                else
                   TextOut(dados->pintor->memDC, dados->pintor->xPos - 300, dados->pintor->yPos + 20, infoAero, _tcslen(infoAero));
                return;
            }
        }
    }
}


 
void verificaExistenciaAviao(HWND hWnd, infoControlador* dados, const int* indice) {
    RECT rectClientWindow = { 0, 0, 0, 0 };
    SIZE sizeTextLogicalSize = { 0, 0 };
    PAINTSTRUCT ps;
    for (int i = 0; i < dados->tamAvioes; ++i) {
        if (!dados->listaAvioes[i].isFree)
            if ((dados->pintor->xPos - dados->listaAvioes[i].av.atuais.posX) < 10 && (dados->pintor->xPos - dados->listaAvioes[i].av.atuais.posX) > -10) {
                if ((dados->pintor->yPos - dados->listaAvioes[i].av.atuais.posY) < 10 && (dados->pintor->yPos - dados->listaAvioes[i].av.atuais.posY) > -10) {
                    {
                        TCHAR infoAviao[100] = _TEXT(" ");
                        _stprintf_s(infoAviao, 100, _TEXT("ID Aviao: %d\n Localização x: %d y: %d\n"), dados->listaAvioes[i].av.procID, dados->listaAvioes[i].av.atuais.posX, dados->listaAvioes[i].av.atuais.posY);
                        GetTextExtentPoint32(dados->pintor->memDC, infoAviao, _tcslen(infoAviao), &sizeTextLogicalSize);
                        if (dados->pintor->xPos < 500)
                            TextOut(dados->pintor->memDC, dados->pintor->xPos + 15, dados->pintor->yPos + 20, infoAviao, _tcslen(infoAviao));
                        else
                            TextOut(dados->pintor->memDC, dados->pintor->xPos - 300, dados->pintor->yPos + 20, infoAviao, _tcslen(infoAviao));
                        return;
                    }
                }
            }
    }
}

int displayInfo(HWND hWnd, infoControlador* dados) {
    RECT rectClientWindow = { 0, 0, 0, 0 };
    SIZE sizeTextLogicalSize = { 0, 0 };
    int tamstrchars = 0;
    // Paint brush
    PAINTSTRUCT ps;
    TCHAR mystring[200] = _TEXT("");

    HDC hDC = BeginPaint(hWnd, &ps);
    SetBkMode(hDC, TRANSPARENT);
    // Colocar cor do texto a azul
    SetTextColor(hDC, RGB(0, 0, 255));

    GetClientRect(hWnd, &rectClientWindow);
    
    TCHAR t[5] = _TEXT("");
    for (int i = 0; i < dados->tamAvioes; ++i) {
        if (dados->listaAvioes[i].isFree == FALSE) {
            if (dados->listaAvioes[i].av.emViagem == TRUE)
                _tcscpy_s(t, 5, L"1");
            else
                _tcscpy_s(t, 5, L" ");

            // Função diz qual o tamanho da string em pixeis, valor fica em sizeTextLogicalSize
            GetTextExtentPoint32(hDC, t, _tcslen(t), &sizeTextLogicalSize);

            TextOut(hDC,
                dados->listaAvioes[i].av.atuais.posX,
                dados->listaAvioes[i].av.atuais.posY,
                t,
                _tcslen(t));
        }
    }

    for (int i = 0; i < dados->tamAeroporto; ++i) {
        // Função diz qual o tamanho da string em pixeis, valor fica em sizeTextLogicalSize
        GetTextExtentPoint32(hDC, L"0", _tcslen(L"0"), &sizeTextLogicalSize);

        TextOut(hDC,
            //(rectClientWindow.right - sizeTextLogicalSize.cx) / 2,
            //rectClientWindow.bottom / 2,
            dados->listaAeroportos[i].localizacao.posX,
            dados->listaAeroportos[i].localizacao.posY,
            L"0",
            _tcslen(L"0"));
    }

    SetBkMode(hDC, OPAQUE);
    EndPaint(hWnd, &ps);
}
    
int displayInfoBitBlt(HWND hWnd, infoControlador* dados, const int* indice) {
	RECT rectClientWindow = { 0, 0, 0, 0 };
	SIZE sizeTextLogicalSize = { 0, 0 };
	int tamstrchars = 0;
	// Paint brush
	PAINTSTRUCT ps;
	TCHAR mystring[200] = _TEXT("");

	dados->pintor->hdc = BeginPaint(hWnd, &ps);
	// dados->pintor->hdc = GetDC(hWnd);
	// Colocar cor do texto a azul
	SetTextColor(dados->pintor->hdc, RGB(0, 0, 255));
	GetClientRect(hWnd, &rectClientWindow);
    // dados->pintor->hBmpBG =(HBITMAP) LoadImage(NULL, _TEXT("C:\\Users\\vascopereira\\source\\repos\\ATC\\ControladorGrafico\\fundo.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    dados->pintor->hBmpAeroporto =(HBITMAP) LoadImage(NULL, _TEXT("C:\\Users\\vascopereira\\source\\repos\\ATC\\ControladorGrafico\\control.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    dados->pintor->hBmpAviao =(HBITMAP) LoadImage(NULL, _TEXT("C:\\Users\\vascopereira\\source\\repos\\ATC\\ControladorGrafico\\airplane.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (dados->pintor->hBmp == NULL)
		dados->pintor->hBmp = CreateCompatibleBitmap(dados->pintor->hdc, rectClientWindow.right, rectClientWindow.bottom);
    dados->pintor->memDC = CreateCompatibleDC(dados->pintor->hdc);
    dados->pintor->memDCBack = CreateCompatibleDC(dados->pintor->memDC);

	// FillRect(dados->pintor->hdcDB, &rectClientWindow, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SelectObject(dados->pintor->memDC, dados->pintor->hBmp);
    SelectObject(dados->pintor->memDCBack, dados->pintor->hBmpAviao);

    BitBlt(dados->pintor->memDC, 0, 0, rectClientWindow.right, rectClientWindow.bottom, NULL, 0, 0, WHITENESS);
    // BitBlt(dados->pintor->memDC, 0, 0, dados->pintor->bmp.bmWidth, dados->pintor->bmp.bmHeight, dados->pintor->memDCBack, 0, 0, SRCCOPY);

	if (dados->pintor->descAero) {
		verificaExistenciaAero(dados->pintor->memDC, dados);
		EnterCriticalSection(&dados->criticalSectionControl);
		dados->pintor->descAero = FALSE;
		LeaveCriticalSection(&dados->criticalSectionControl);
	}

	if (dados->pintor->descAviao) {
		verificaExistenciaAviao(dados->pintor->memDC, dados, indice);
		EnterCriticalSection(&dados->criticalSectionControl);
		dados->pintor->descAviao = FALSE;
		LeaveCriticalSection(&dados->criticalSectionControl);
	}

	for (int i = 0; i < dados->tamAvioes; ++i) {
		if (dados->listaAvioes[i].isFree == FALSE) {
			if (dados->listaAvioes[i].av.emViagem == TRUE)
                BitBlt(dados->pintor->memDC, dados->listaAvioes[i].av.atuais.posX,
                    dados->listaAvioes[i].av.atuais.posY, 10,
                    10, dados->pintor->memDCBack, 0, 0, SRCCOPY);
		}
	}

    SelectObject(dados->pintor->memDCBack, dados->pintor->hBmpAeroporto);

	for (int i = 0; i < dados->tamAeroporto; ++i) {
		// Função diz qual o tamanho da string em pixeis, valor fica em sizeTextLogicalSize
        BitBlt(dados->pintor->memDC, dados->listaAeroportos[i].localizacao.posX,
            dados->listaAeroportos[i].localizacao.posX, 10,
            10, dados->pintor->memDCBack, 0, 0, SRCCOPY);
	}
    

	//BitBlt(dados->pintor->hdc, 0, 0, rectClientWindow.right, rectClientWindow.bottom, dados->pintor->memDC, 0, 0, SRCCOPY);
	//BitBlt(dados->pintor->memDC, 0, 0, rectClientWindow.right, rectClientWindow.bottom, dados->pintor->memDC, 0, 0, SRCCOPY);
	// EndPaint(hWnd, &ps);
    

    BitBlt(dados->pintor->hdc, 0, 0, rectClientWindow.right, rectClientWindow.bottom, dados->pintor->memDC, 0, 0, SRCCOPY);

    DeleteDC(dados->pintor->memDC);
    DeleteDC(dados->pintor->memDCBack);
    
    EndPaint(hWnd, &ps);
    // ReleaseDC(hWnd, dados->pintor->hdc);
}

void testePintura(HWND hWnd, infoControlador* dados) {
    

}