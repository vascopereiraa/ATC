
#ifndef __CONTROLADOR_H__
#define __CONTROLADOR_H__

#include <Windows.h>

#include "../Aviao/Aviao.h"
#include "Aviao.h"
#include "Aeroporto.h"
#include "Passageiro.h"

typedef struct {
	HWND hWnd;      // Handle para a janela principal
	HDC hdc;        // HDC principal para display da info
	HDC memDC;      // HDC para double buffer,
	HDC memDCBack;  // HDC para as imagens do avião e aeroporto
	int xPos;		// Armazenar coordenadas X do rato
	int yPos;	    // Armazenar coordenadas Y do rato
	BOOL descAero;  // Flag para WM_PAINT saber quando imprimir descrição Aero
	BOOL descAviao; // Flag para WM_PAINT saber quando imprimir descrição Avião
	// Pinturas
	HBITMAP hBmp;		  // Bitmap compativel com o HDC
	HBITMAP hBmpAviao;	  // BitMap para a imagem do Aviao
	HBITMAP hBmpAeroporto;// BitMap para a imagem do Aeroporto

} Pintor;

typedef struct {
	HANDLE hFileMap;		// Handle para a zona de memória partilhada
	HANDLE hSemItens;	    // Handle para o semaforo de Itens colocados no buffer
	HANDLE hSemMutexProd;   // Handle para o semaforo binário usado para sincronização entre aviões
	bufferCircular* pBuf;   // Ponteiro para a vista partilhada em memória
} controloBufferCirc;

typedef struct {
	InfoPassagPipes* infoPassagPipes;
	controloBufferCirc* bufCirc;	// Estrutura de dados do buffer circular
	listaAviao* listaAvioes;		// Array de avioes
	aeroporto* listaAeroportos;		// Array de aeroportos
	Pintor* pintor;					// Estrutura para a parte gráfica

	int tamAvioes;					// Tamanho do array de avioes
	int tamAeroporto;				// Tamanho do array de aeroportos
	int indiceAero;					// Indice da proxima posicao livre no array de aeroportos

	int* terminaControlador;					// Flag para terminar o controlador
	int* suspendeNovosAvioes;					// Flag para suspender/aceitar novos avioes
	CRITICAL_SECTION criticalSectionControl;	// Garante sincronização entre threads
	// Handle threads
	HANDLE hThreadBuffer;
	HANDLE hThreadTimer;
	HANDLE hThreadNamedPipes;
} infoControlador;

// Funcoes de Controlo do Buffer Circular em SHMem
BOOL criaBufferCircular(controloBufferCirc* bufCirc);
void encerraBufferCircular(controloBufferCirc* controlo);

void encerraControlador(infoControlador* infoControl);
void menu(infoControlador* infoControl);

// Threads do controlador
void WINAPI threadControloBuffer(LPVOID lpParam);
void WINAPI threadTimer(LPVOID lpParam);
DWORD WINAPI threadNamedPipes(LPVOID lpParam);

// Funcoes - Registry
BOOL controladorRegistry(int* maxAeroportos, int* maxAvioes);

// Objetos de sincronização
void criaCriticalSectionControl(LPCRITICAL_SECTION lpCriticalSectionControl);
void encerraCriticalSectionControl(LPCRITICAL_SECTION lpCriticalSectionControl);

#endif // !__CONTROLADOR_H__