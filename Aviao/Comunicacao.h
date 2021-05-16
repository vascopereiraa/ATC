
#ifndef __COMUNICACAO_H__
#define __COMUNICACAO_H__

// Definição da funcao da DLL
typedef int (*fcnDLL)(int, int, int, int, int*, int*);

// Funcoes de Comunicacao
fcnDLL carregaDLL(TCHAR* dllLocation);
void libertaDLL(TCHAR* dllLocation);
DWORD WINAPI threadViagem(LPVOID lpParam);


#endif // !__COMUNICACAO_H__

