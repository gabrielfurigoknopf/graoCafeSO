//Integrantes: Cristian Willian Dos Santos (159254), Gabriel Furigo Knopf (199030)

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* Estrutura do buffer compartilhado (Hooper) */
typedef struct
{
    int *buffer;
    int capacidade;
    int inicio;
    int fim;
    int quantidade;
} Hooper;

/* Variáveis globais */
Hooper hooper;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_tokens = PTHREAD_COND_INITIALIZER;

int P;
int H;
int T;
int Min;
int Max;

int tokens_gerados = 0;
int tokens_processados = 0;
int tokens_descartados = 0;

int produtor_finalizou = 0;
int provadores_ocupados = 0;

/* Protótipos */
void inserir_token(int token);
int remover_token();

void *provador(void *arg);
void *mestre_torra(void *arg);

int validar_entrada(void);

/* Implementações */

/* Insere token no buffer */
void inserir_token(int token)
{
    /* TODO */
}

/* Remove token do buffer */
int remover_token()
{
    /* TODO */
    return 0;
}

/* Thread dos provadores */
void *provador(void *arg)
{
    /* TODO */
    return NULL;
}

/* Thread do mestre de torra */
void *mestre_torra(void *arg)
{
    /* TODO */
    return NULL;
}

/* Validação dos parâmetros */
int validar_entrada(void)
{
    /* TODO */
    return 1;
}

int main(int argc, char *argv[])
{
    printf("Projeto GraoCafe iniciado.\n");

    /* TODO:
     * Ler parâmetros
     * Inicializar buffer
     * Criar threads
     * Aguardar execução
     * Exibir relatório
     */

    return 0;
}