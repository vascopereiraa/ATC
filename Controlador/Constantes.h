#ifndef __CONSTANTES_H__
#define __CONSTANTES_H__

// Controlador
#define SHM_CONTROL _TEXT("SHM_CONTROL")
#define SEM_MUTEX_PROD _TEXT("SEM_MUTEX_CONS")
#define SEM_ITENS _TEXT("SEM_ITENS")
#define MAX_BUF 10
#define STR_TAM 50

// Aviao
#define SHM_AVIAO _TEXT("SHM_%d")
#define EVNT_AVIAO _TEXT("EVNT_%d")
#define EVNT_VIAGEM _TEXT("EVNT_VIAGEM")

// Registry
#define MAX_AIRPORT 20
#define MAX_AIRPLANES 30
#define TAM 200

#define CAMINHO_REGISTRY _TEXT("Software\\TrabalhoPratico")
#define MAX_AEROPORTOS _TEXT("MAX_AIRPORT")
#define MAX_AVIOES _TEXT("MAX_AIRPLANES")

#endif // !__CONSTANTES_H__
