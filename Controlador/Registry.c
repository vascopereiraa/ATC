#include <windows.h>
#include <tchar.h>
#include <fcntl.h>

#include "Controlador.h"
#include "Constantes.h"
#include "Utils.h"

BOOL controladorRegistry(int* maxAeroportos, int* maxAvioes) {
	HKEY chave;
	TCHAR chave_nome[TAM] = TEXT("Software\\TrabalhoPratico");
	TCHAR par_nome[TAM] = TEXT("MAX_AIRPORT"); /* nome da chave */
	DWORD par_valor = MAX_AIRPORT; /* valor da chave*/
	DWORD dwResult;
	DWORD dwValues, dwMaxValueNameLen, dwMaxValueLen;
	TCHAR valueName[TAM];
	DWORD valueContent;

	if (RegOpenKey(HKEY_CURRENT_USER, chave_nome, &chave) != ERROR_SUCCESS) {
		debug(_TEXT("A chave ainda nao foi criada!\n"));
		if (RegCreateKeyEx(HKEY_CURRENT_USER, chave_nome, 0, NULL, REG_OPTION_VOLATILE, KEY_WRITE, NULL, &chave, &dwResult) == ERROR_SUCCESS) {
			if (dwResult == REG_CREATED_NEW_KEY) {
				debug(_TEXT("A chave foi criada!\n"));

				//Criação do primeiro define em registry para o MAX_AIRPORT
				if (RegSetValueEx(chave, par_nome, 0, REG_DWORD, &par_valor, sizeof(par_valor)) == ERROR_SUCCESS)
					debug(_T("Foi criada a chave %s > %i no registry!"), par_nome, par_valor);

				//Criação do primeiro define em registry para o MAX_AVIOES
				_tcscpy_s(par_nome, TAM, TEXT("MAX_AIRPLANES"));
				par_valor = MAX_AIRPLANES;
				if (RegSetValueEx(chave, par_nome, 0, REG_DWORD, &par_valor, sizeof(par_valor)) == ERROR_SUCCESS)
					debug(_T("Foi criada a chave %s > %i no registry!"), par_nome, par_valor);

			}
			else if (dwResult == REG_OPENED_EXISTING_KEY) {
				debug(_TEXT("A chave já existia!\n"));
			}
		}
		else {
			debug(_TEXT("Não foi possível criar a chave!\n"));
			return FALSE;
		}
	}
	else {
		debug(_TEXT("A chave que procura já foi criada!\n"));
	}

	// Listar as named keys do handle em uso
	if (RegQueryInfoKey(chave, NULL, NULL, NULL, NULL, NULL, NULL, &dwValues, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL) != ERROR_SUCCESS) {
		fatal(_TEXT("Ocorreu um erro a procurar os named pairs do registry indicado!\n"));
		return FALSE;
	}
	else {
		// _tprintf(_TEXT("\nListar todos os pares valores de %s\n"), chave_nome);
		for (int i = 0; i < dwValues; ++i) {
			DWORD dwNameSize = dwMaxValueNameLen + 1;
			if (RegEnumValue(chave, i, valueName, &dwNameSize, 0, NULL, &valueContent, &dwMaxValueLen) == ERROR_SUCCESS) {
				if (i == 0) {
					*maxAeroportos = (int)valueContent;
				}
				else
					*maxAvioes = (int)valueContent;
				// _tprintf(_TEXT("Par %d: %s -> %i\n"), i + 1, valueName, valueContent);
			}
		}
	}

	RegCloseKey(chave);
	return TRUE;
}
