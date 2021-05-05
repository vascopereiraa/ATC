#pragma once

// Constantes 
#define MAX_AIRPORT 20
#define MAX_AIRPLANES 30
#define TAM 200

#define CAMINHO_REGISTRY _TEXT("Software\\TrabalhoPratico")
#define MAX_AEROPORTOS _TEXT("MAX_AIRPORT")
#define MAX_AVIOES _TEXT("MAX_AIRPLANES")

// Funcoes - Registry
HKEY abreOuCriaChave();
void obtemValoresRegistry(HKEY chave, int* maxAeroportos, int* maxAvioes);
void criaValoresRegistry(HKEY chave);
BOOL controladorRegistry(int* maxAeroportos, int* maxAvioes);
