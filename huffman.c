/*
 * Projeto 2 - Codificacao de Huffman
 *
 * INTEGRANTES:
 * 1. Lara Ferreira dos Santos - 10753380
 * 2. Felipe Salles - 10735729
 * 3. Katheling Leite - 10749992
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ALFABETO 256

/* ---------------------------- Estruturas ---------------------------- */

typedef struct No {
    unsigned char c;
    unsigned long freq;
    struct No *esq, *dir;
} No;

typedef struct {
    No *v[ALFABETO];
    int tam;
} Heap;

typedef struct {
    FILE *f;
    unsigned char buf;
    int n;
} BitW;

typedef struct {
    FILE *f;
    unsigned char buf;
    int n;
} BitR;

/* ---------------------------- Bits ---------------------------- */

static void bw_init(BitW *w, FILE *f) { w->f = f; w->buf = 0; w->n = 0; }

static void bw_bit(BitW *w, int b) {
    w->buf = (unsigned char)((w->buf << 1) | (b & 1));
    if (++w->n == 8) { fputc(w->buf, w->f); w->buf = 0; w->n = 0; }
}

static void bw_flush(BitW *w) {
    while (w->n != 0) bw_bit(w, 0);
}

static void br_init(BitR *r, FILE *f) { r->f = f; r->buf = 0; r->n = 0; }

static int br_bit(BitR *r) {
    if (r->n == 0) {
        int c = fgetc(r->f);
        if (c == EOF) return -1;
        r->buf = (unsigned char)c;
        r->n = 8;
    }
    r->n--;
    return (r->buf >> r->n) & 1;
}

/* ---------------------------- Heap ---------------------------- */

static void heap_insere(Heap *h, No *n) {
    int i = h->tam++;
    while (i > 0 && n->freq < h->v[(i - 1) / 2]->freq) {
        h->v[i] = h->v[(i - 1) / 2];
        i = (i - 1) / 2;
    }
    h->v[i] = n;
}

static No *heap_remove(Heap *h) {
    No *min = h->v[0];
    No *ultimo = h->v[--h->tam];
    if (h->tam == 0) return min;

    int i = 0;
    while (2 * i + 1 < h->tam) {
        int esq = 2 * i + 1;
        int dir = 2 * i + 2;
        int filho = esq;
        if (dir < h->tam && h->v[dir]->freq < h->v[esq]->freq) {
            filho = dir;
        }
        if (ultimo->freq <= h->v[filho]->freq) break;
        h->v[i] = h->v[filho];
        i = filho;
    }
    h->v[i] = ultimo;
    return min;
}

/* ---------------------------- Arvore ---------------------------- */

static No *novo_no(unsigned char c, unsigned long freq, No *e, No *d) {
    No *n = malloc(sizeof(No));
    if (!n) { fprintf(stderr, "Erro de memoria.\n"); exit(1); }
    n->c = c; n->freq = freq; n->esq = e; n->dir = d;
    return n;
}

static int eh_folha(const No *n) { return !n->esq && !n->dir; }

static void libera_arvore(No *n) {
    if (!n) return;
    libera_arvore(n->esq);
    libera_arvore(n->dir);
    free(n);
}

/* Fase 2: constroi a arvore de Huffman a partir das frequencias */
static No *constroi_arvore(const unsigned long freq[ALFABETO]) {
    Heap h = { .tam = 0 };
    for (int i = 0; i < ALFABETO; i++)
        if (freq[i] > 0) heap_insere(&h, novo_no((unsigned char)i, freq[i], NULL, NULL));

    if (h.tam == 0) return NULL;

    while (h.tam > 1) {
        No *a = heap_remove(&h);
        No *b = heap_remove(&h);
        heap_insere(&h, novo_no(0, a->freq + b->freq, a, b));
    }
    return heap_remove(&h);
}

static void gera_codigos(const No *n, char *caminho, int prof, char *cod[ALFABETO]) {
    if (eh_folha(n)) {
        if (prof == 0) { caminho[0] = '0'; prof = 1; } /* texto com 1 simbolo */
        caminho[prof] = '\0';
        cod[n->c] = strdup(caminho);
        return;
    }
    caminho[prof] = '0'; gera_codigos(n->esq, caminho, prof + 1, cod);
    caminho[prof] = '1'; gera_codigos(n->dir, caminho, prof + 1, cod);
}

static void grava_arvore(BitW *w, const No *n) {
    if (eh_folha(n)) {
        bw_bit(w, 1);
        for (int i = 7; i >= 0; i--) bw_bit(w, (n->c >> i) & 1);
    } else {
        bw_bit(w, 0);
        grava_arvore(w, n->esq);
        grava_arvore(w, n->dir);
    }
}

static No *le_arvore(BitR *r) {
    int b = br_bit(r);
    if (b < 0) return NULL;
    if (b == 1) {
        unsigned char c = 0;
        for (int i = 0; i < 8; i++) {
            int bit = br_bit(r);
            if (bit < 0) return NULL;
            c = (unsigned char)((c << 1) | bit);
        }
        return novo_no(c, 0, NULL, NULL);
    }
    No *e = le_arvore(r);
    No *d = le_arvore(r);
    if (!e || !d) { libera_arvore(e); libera_arvore(d); return NULL; }
    return novo_no(0, 0, e, d);
}

/* ---------------------------- Codificar e Decodificar ---------------------------- */

static void mostra_char(unsigned char c) {
    if (c == '\n') printf("'\\n'");
    else if (c == '\t') printf("'\\t'");
    else if (c == '\r') printf("'\\r'");
    else if (c == ' ') printf("' '");
    else if (c < 32 || c > 126) printf("0x%02X", c);
    else printf("'%c'", c);
}

static int codifica(const char *entrada, const char *saida) {
    FILE *in = fopen(entrada, "rb");
    if (!in) { perror("Erro ao abrir entrada"); return 1; }

    /* Fase 1: frequencias */
    unsigned long freq[ALFABETO] = {0};
    uint64_t total = 0;
    int c;
    while ((c = fgetc(in)) != EOF) { freq[c]++; total++; }

    FILE *out = fopen(saida, "wb");
    if (!out) { perror("Erro ao abrir saida"); fclose(in); return 1; }

    /* Grava o tamanho original (8 bytes) */
    for (int i = 0; i < 8; i++) fputc((int)((total >> (8 * i)) & 0xFF), out);

    if (total == 0) {
        fclose(in); fclose(out);
        printf("Arquivo vazio.\n");
        return 0;
    }

    No *raiz = constroi_arvore(freq);
    char *cod[ALFABETO] = {0};
    char caminho[ALFABETO + 1];
    gera_codigos(raiz, caminho, 0, cod);

    /* Tabela de frequencias e codigos */
    printf("Simbolo    Freq        Codigo\n");
    for (int i = 0; i < ALFABETO; i++) {
        if (cod[i]) {
            mostra_char((unsigned char)i);
            printf("\t  %-10lu  %s\n", freq[i], cod[i]);
        }
    }

    /* Fase 3: arvore + texto codificado */
    BitW w; bw_init(&w, out);
    grava_arvore(&w, raiz);

    rewind(in);
    while ((c = fgetc(in)) != EOF) {
        for (const char *p = cod[c]; *p; p++) bw_bit(&w, *p - '0');
    }
    bw_flush(&w);

    long tam_saida = ftell(out);
    printf("\nTamanho original: %llu bytes\n", (unsigned long long)total);
    printf("Arquivo gerado  : %ld bytes (inclui cabecalho e arvore)\n", tam_saida);

    for (int i = 0; i < ALFABETO; i++) free(cod[i]);
    libera_arvore(raiz);
    fclose(in); fclose(out);
    printf("Codificacao concluida com sucesso!\n");
    return 0;
}

static int decodifica(const char *entrada, const char *saida) {
    FILE *in = fopen(entrada, "rb");
    if (!in) { perror("Erro ao abrir entrada"); return 1; }

    uint64_t total = 0;
    for (int i = 0; i < 8; i++) {
        int c = fgetc(in);
        if (c == EOF) {
            fprintf(stderr, "Arquivo corrompido (cabecalho).\n");
            fclose(in);
            return 1;
        }
        total |= (uint64_t)c << (8 * i);
    }

    FILE *out = fopen(saida, "wb");
    if (!out) { perror("Erro ao abrir saida"); fclose(in); return 1; }

    if (total == 0) { fclose(in); fclose(out); return 0; }

    BitR r; br_init(&r, in);
    No *raiz = le_arvore(&r);
    if (!raiz) {
        fprintf(stderr, "Arquivo corrompido (arvore).\n");
        fclose(in); fclose(out);
        return 1;
    }

    for (uint64_t i = 0; i < total; i++) {
        const No *n = raiz;
        while (!eh_folha(n)) {
            int b = br_bit(&r);
            if (b < 0) {
                fprintf(stderr, "Arquivo corrompido (dados incompletos).\n");
                libera_arvore(raiz); fclose(in); fclose(out);
                return 1;
            }
            n = b ? n->dir : n->esq;
        }
        fputc(n->c, out);
    }

    libera_arvore(raiz);
    fclose(in); fclose(out);
    printf("Decodificacao concluida com sucesso!\n");
    return 0;
}

/* ---------------------------- Main ---------------------------- */

int main(int argc, char **argv) {
    if (argc != 4 || (strcmp(argv[1], "c") != 0 && strcmp(argv[1], "d") != 0)) {
        fprintf(stderr, "Uso: %s [c/d] <entrada> <saida>\n", argv[0]);
        return 1;
    }
    return argv[1][0] == 'c' ? codifica(argv[2], argv[3]) : decodifica(argv[2], argv[3]);
}