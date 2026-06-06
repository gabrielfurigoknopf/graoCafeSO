// Integrantes: Cristian Willian Dos Santos (159254), Gabriel Furigo Knopf (199030)

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* Estrutura do buffer compartilhado (Hooper) */
typedef struct {
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

int P; /* provadores */
int H; /* hooper capacidade */
int T; /* total tokens a gerar */
int Min; /* intervalo mínimo (unidade: milissegundos) */
int Max; /* intervalo máximo (unidade: milissegundos) */

int tokens_gerados = 0;
int tokens_processados = 0;
int tokens_descartados = 0;

int produtor_finalizou = 0;
int provadores_ocupados = 0;

/* Protótipos */
int inserir_token(int token); /* retorna 0 em sucesso, -1 se cheio */
int remover_token(int *token); /* retorna 0 em sucesso, -1 se vazio */

void *provador(void *arg);
void *mestre_torra(void *arg);

int validar_entrada(void);

/* Insere token no buffer (chamar com mutex travado) */
int inserir_token(int token)
{
    if (hooper.quantidade >= hooper.capacidade) {
        return -1; /* cheio */
    }
    hooper.buffer[hooper.fim] = token;
    hooper.fim = (hooper.fim + 1) % hooper.capacidade;
    hooper.quantidade++;
    return 0;
}

/* Remove token do buffer (chamar com mutex travado) */
int remover_token(int *token)
{
    if (hooper.quantidade == 0) {
        return -1; /* vazio */
    }
    *token = hooper.buffer[hooper.inicio];
    hooper.inicio = (hooper.inicio + 1) % hooper.capacidade;
    hooper.quantidade--;
    return 0;
}

/* Thread dos provadores */
void *provador(void *arg)
{
    int id = *(int *)arg;
    free(arg);

    /* seed local para rand_r */
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(id * 0x9e3779b9);

    printf("[Provador %d] Inicializado.\n", id);

    while (1) {
        pthread_mutex_lock(&mutex);

        /* Espera por tokens enquanto produtor não finalizou */
        while (hooper.quantidade == 0 && !produtor_finalizou) {
            /* Entrando em hibernação */
            pthread_cond_wait(&cond_tokens, &mutex);
        }

        /* Se não há tokens e produtor finalizou, encerra */
        if (hooper.quantidade == 0 && produtor_finalizou) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        /* Há token(s) no Hooper: remover e processar */
        int token_id;
        if (remover_token(&token_id) != 0) {
            /* inesperado: sem token */
            pthread_mutex_unlock(&mutex);
            continue;
        }

        provadores_ocupados++;
        printf("[Provador %d] Acionado para Token %d. Hooper agora: %d/%d\n",
               id, token_id, hooper.quantidade, hooper.capacidade);

        pthread_mutex_unlock(&mutex);

        /* Processamento: tempo aleatório entre 10 e 50 (ms) */
        int tempo_processo = (rand_r(&seed) % 41) + 10; /* 10..50 */
        printf("[Provador %d] Processando Token %d por %d ms...\n", id, token_id, tempo_processo);
        usleep((useconds_t)tempo_processo * 1000);

        pthread_mutex_lock(&mutex);
        tokens_processados++;
        provadores_ocupados--;
        printf("[Provador %d] Finalizou Token %d. Total processados: %d\n",
               id, token_id, tokens_processados);
        pthread_mutex_unlock(&mutex);
    }

    printf("[Provador %d] Finalizado.\n", id);
    return NULL;
}

/* Thread do mestre de torra */
void *mestre_torra(void *arg)
{
    (void)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ 0xabcdef;

    for (int i = 1; i <= T; i++) {
        /* intervalo aleatório entre Min e Max (ms) */
        int espera = Min + (rand_r(&seed) % (Max - Min + 1));
        usleep((useconds_t)espera * 1000);

        pthread_mutex_lock(&mutex);
        tokens_gerados++;
        printf("[Mestre] Chegada do Token %d. ", i);

        /* Estado atual */
        int disponivel = (provadores_ocupados < P);
        printf(disponivel ? "Provador disponível. " : "Todos os Provadores ocupados. ");
        printf("Hooper: %d/%d. ", hooper.quantidade, hooper.capacidade);

        /* Decisão: inserir se houver espaço; caso contrário descartar */
        if (hooper.quantidade < hooper.capacidade) {
            if (inserir_token(i) == 0) {
                printf("Token %d adicionado ao Hooper. Hooper agora: %d/%d\n",
                       i, hooper.quantidade, hooper.capacidade);
                /* Aciona um provador (se houver algum hibernando) */
                pthread_cond_signal(&cond_tokens);
            } else {
                /* não deveria ocorrer por checagem anterior, mas tratar */
                tokens_descartados++;
                printf("DESCARTE inesperado do Token %d (inserir falhou).\n", i);
            }
        } else {
            /* Hooper cheio e todos ocupados -> descarte */
            tokens_descartados++;
            printf("DESCARTE do Token %d! (Hooper cheio e todos ocupados)\n", i);
        }

        pthread_mutex_unlock(&mutex);
    }

    /* sinalizar finalização e acordar provadores */
    pthread_mutex_lock(&mutex);
    produtor_finalizou = 1;
    pthread_cond_broadcast(&cond_tokens);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* Validação dos parâmetros conforme PDF */
int validar_entrada(void)
{
    if (P <= 0 || P >= 5) return 0;   /* 1..4 */
    if (H <= 0 || H >= 10) return 0;  /* 1..9 */
    if (T <= 1 || T >= 100) return 0; /* 2..99 */
    if (Min <= 5 || Min >= 10) return 0; /* 6..9 */
    if (Max <= Min || Max >= 50) return 0; /* Min+1 .. 49 */
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc != 6) {
        fprintf(stderr, "Uso: %s P H T Min Max\n", argv[0]);
        fprintf(stderr, "Onde: P(1..4) H(1..9) T(2..99) Min(6..9) Max(Min+1..49)\n");
        return EXIT_FAILURE;
    }

    P   = atoi(argv[1]);
    H   = atoi(argv[2]);
    T   = atoi(argv[3]);
    Min = atoi(argv[4]);
    Max = atoi(argv[5]);

    if (!validar_entrada()) {
        fprintf(stderr, "Parâmetros inválidos. Verifique os limites no enunciado.\n");
        return EXIT_FAILURE;
    }

    /* Inicializar Hooper */
    hooper.capacidade = H;
    hooper.buffer = malloc(sizeof(int) * hooper.capacidade);
    if (!hooper.buffer) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    hooper.inicio = 0;
    hooper.fim = 0;
    hooper.quantidade = 0;

    srand((unsigned int)time(NULL));

    pthread_t th_mestre;
    pthread_t *th_provadores = malloc(sizeof(pthread_t) * P);
    if (!th_provadores) {
        perror("malloc");
        free(hooper.buffer);
        return EXIT_FAILURE;
    }

    /* Criar threads provadores */
    for (int i = 0; i < P; i++) {
        int *id = malloc(sizeof(int));
        if (!id) {
            perror("malloc");
            return EXIT_FAILURE;
        }
        *id = i + 1;
        if (pthread_create(&th_provadores[i], NULL, provador, id) != 0) {
            perror("pthread_create provador");
            return EXIT_FAILURE;
        }
    }

    /* Criar thread mestre */
    if (pthread_create(&th_mestre, NULL, mestre_torra, NULL) != 0) {
        perror("pthread_create mestre");
        return EXIT_FAILURE;
    }

    /* Aguardar término */
    if (pthread_join(th_mestre, NULL) != 0) {
        perror("pthread_join mestre");
    }
    for (int i = 0; i < P; i++) {
        if (pthread_join(th_provadores[i], NULL) != 0) {
            perror("pthread_join provador");
        }
    }

    /* Relatório final */
    printf("\n--- RELATÓRIO FINAL ---\n");
    printf("Tokens Gerados   : %d\n", tokens_gerados);
    printf("Tokens Processados: %d\n", tokens_processados);
    printf("Tokens Descartados: %d\n", tokens_descartados);
    printf("Prejuízo Estimado : US$ %d\n", tokens_descartados * 100);

    free(th_provadores);
    free(hooper.buffer);
    return EXIT_SUCCESS;
}
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
