
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
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
#ifdef UNICODE
    (void) _setmode(_fileno(stdin), _O_WTEXT);
    (void)_setmode(_fileno(stdout), _O_WTEXT);
    (void)_setmode(_fileno(stderr), _O_WTEXT);
#endif

    infoControlador infoControl;
    ZeroMemory(&infoControl, sizeof(infoControlador));

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

    infoControl.indiceAero = 2;
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
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
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
    wcex.cbWndExtra     = 0;
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
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
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
