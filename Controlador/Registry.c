#include <windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Controlador.h"
#include "Constantes.h"

HKEY abreOuCriaChave() {
	HKEY chave;
	DWORD dwResult;

	if (RegOpenKey(HKEY_CURRENT_USER, CAMINHO_REGISTRY, &chave) != ERROR_SUCCESS) {
		_tprintf(_TEXT("A chave ainda nao foi criada!\n"));
		if (RegCreateKeyEx(HKEY_CURRENT_USER, CAMINHO_REGISTRY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &chave, &dwResult) == ERROR_SUCCESS) {
			if (dwResult == REG_CREATED_NEW_KEY) {
				_tprintf(_TEXT("A chave foi criada!\n"));
			}
			else if (dwResult == REG_OPENED_EXISTING_KEY) {
				_tprintf(_TEXT("A chave já existia!\n"));
			}
		}
		else {
			_tcprintf(_TEXT("Não foi possível criar a chave!\n"));
		}
	}
	else {
		_tcprintf(_TEXT("A chave que procura já foi criada!\n"));
	}

	return chave;
}

void obtemValoresRegistry(HKEY chave, int* maxAeroportos, int* maxAvioes) {
	DWORD dwValues, dwMaxValueNameLen, dwMaxValueLen;
	TCHAR chave_nome[TAM];
	TCHAR valueName[TAM];
	DWORD valueContent;

	// Listar as named keys do handle em uso - alinea e)
	if (RegQueryInfoKey(chave, NULL, NULL, NULL, NULL, NULL, NULL, &dwValues, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL) != ERROR_SUCCESS) {
		_tprintf(_TEXT("Ocorreu um erro a procurar os named pairs do registry indicado!\n"));

	}
	else {
		for (int i = 0; i < dwValues; ++i) {
			DWORD dwNameSize = dwMaxValueNameLen + 1;
			if (RegEnumValue(chave, i, valueName, &dwNameSize, 0, NULL, &valueContent, &dwMaxValueLen) == ERROR_SUCCESS) {
				if (!_tcscmp(valueName, MAX_AEROPORTOS))
					*maxAeroportos = (int)valueContent;
				if (!_tcscmp(valueName, MAX_AVIOES))
					*maxAvioes = (int)valueContent;
				_tprintf(_TEXT("Par %d: %s -> %i\n"), i + 1, valueName, valueContent);
			}
		}
	}
}

void criaValoresRegistry(HKEY chave) {

	//Criação do primeiro define em registry para o MAX_AIRPORT
	if (RegSetValueEx(chave, MAX_AEROPORTOS, 0, REG_DWORD, MAX_AIRPORT, sizeof(MAX_AIRPORT)) == ERROR_SUCCESS)
		_tprintf(_T("Foi criada a chave %s > %i no registry!"), MAX_AEROPORTOS, MAX_AIRPORT);

	//Criação do primeiro define em registry para o MAX_AVIOES
	if (RegSetValueEx(chave, MAX_AVIOES, 0, REG_DWORD, MAX_AIRPLANES, sizeof(MAX_AIRPLANES)) == ERROR_SUCCESS)
		_tprintf(_T("Foi criada a chave %s > %i no registry!"), MAX_AVIOES, MAX_AIRPLANES);

}

BOOL controladorRegistry(int* maxAeroportos, int* maxAvioes) {
	HKEY chave;
	TCHAR chave_nome[TAM] = TEXT("Software\\TrabalhoPratico");
	TCHAR par_nome[TAM] = TEXT("MAX_AIRPORT"); /* nome da chave */
	DWORD par_valor = MAX_AIRPORT; /* valor da chave*/
	DWORD dwResult;
	DWORD dwValues, dwMaxValueNameLen, dwMaxValueLen;
	TCHAR valueName[TAM];
	DWORD valueContent;

	//RegOpenKey(tipo de registry, caminho da chave, ponteiro para a handle aberta)
	if (RegOpenKey(HKEY_CURRENT_USER, chave_nome, &chave) != ERROR_SUCCESS) {
		_tprintf(_TEXT("A chave ainda nao foi criada!\n"));
		if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_nome, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &chave, &dwResult) == ERROR_SUCCESS) {
			if (dwResult == REG_CREATED_NEW_KEY) {
				_tprintf(_TEXT("A chave foi criada!\n"));
			}
			else if (dwResult == REG_OPENED_EXISTING_KEY) {
				_tprintf(_TEXT("A chave já existia!\n"));
			}
		}
		else {
			_tcprintf(_TEXT("Não foi possível criar a chave!\n"));
		}
	}
	else {
		_tcprintf(_TEXT("A chave que procura já foi criada!\n"));
	}

	//Criação do primeiro define em registry para o MAX_AIRPORT
	if (RegSetValueEx(chave, par_nome, 0, REG_DWORD, &par_valor, sizeof(par_valor)) == ERROR_SUCCESS)
		_tprintf(_T("Foi criada a chave %s > %i no registry!"), par_nome, par_valor);

	//Criação do primeiro define em registry para o MAX_AVIOES
	_tcscpy_s(par_nome, TAM, TEXT("MAX_AIRPLANES"));
	par_valor = MAX_AIRPLANES;
	if (RegSetValueEx(chave, par_nome, 0, REG_DWORD, &par_valor, sizeof(par_valor)) == ERROR_SUCCESS)
		_tprintf(_T("Foi criada a chave %s > %i no registry!"), par_nome, par_valor);

	// Listar as named keys do handle em uso - alinea e)
	if (RegQueryInfoKey(chave, NULL, NULL, NULL, NULL, NULL, NULL, &dwValues, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL) != ERROR_SUCCESS) {
		_tprintf(_TEXT("Ocorreu um erro a procurar os named pairs do registry indicado!\n"));

	}
	else {
		_tprintf(_TEXT("\nListar todos os pares valores de %s\n"), chave_nome);
		for (int i = 0; i < dwValues; ++i) {
			DWORD dwNameSize = dwMaxValueNameLen + 1;
			if (RegEnumValue(chave, i, valueName, &dwNameSize, 0, NULL, &valueContent, &dwMaxValueLen) == ERROR_SUCCESS) {
				if (i == 0) {
					*maxAeroportos = (int)valueContent;
				}
				else
					*maxAvioes = (int)valueContent;
				_tprintf(_TEXT("Par %d: %s -> %i\n"), i + 1, valueName, valueContent);
			}
		}
	}

	RegCloseKey(chave);
}
