#include <stdio.h>
#include <stdint.h>

typedef struct{
    uint16_t Registradores[8];
    uint16_t PC;
    uint16_t IR;
    uint16_t SP;
    uint16_t FLAGS;

    uint16_t Memoria[1024];
    uint16_t Pilha[256];
} processorsimulator;

void readArchive(const char *Arquivo) {
    char endereco[50], conteudo[50];
    FILE *arq;

    arq = fopen(Arquivo, "rt");
    if(arq == NULL){
        printf("Erro ao abrir o arquivo: %s\n", Arquivo);
        return;
    }
    printf("\nConteúdo do arquivo:\n");
    while(fscanf(arq, "%30[^:]: %30[^\n]\n", endereco, conteudo) == 2){
        printf("Endereco: %s | Conteudo: %s\n", endereco, conteudo);
    }

    fclose(arq);
}

void executeInstruction(processorsimulator *processador){

}

int main(){
    readArchive("../arquivo.txt");
}