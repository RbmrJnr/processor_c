#include <stdio.h>
#include <stdint.h>

typedef struct{
    uint16_t Registradores[8];
    uint16_t PC;
    uint16_t IR;
    uint16_t SP;

    uint16_t Memoria[1024];
    uint16_t Pilha[256];

    struct{
        uint16_t Zero;
        uint16_t Negative;
        uint16_t Carry;
        uint16_t Overflow;
    } FLAGS;
    
} processorsimulator;

void readArchive(processorsimulator *processador, const char *Arquivo) {
    uint16_t endereco, conteudo;
    char linha[50];
    FILE *arq;

    arq = fopen(Arquivo, "rt");
    if(arq == NULL){
        printf("Erro ao abrir o arquivo: %s\n", Arquivo);
        return;
    }
    printf("\nConteúdo do arquivo:\n");
    while (fgets(linha, sizeof(linha), arq)){
        if(sscanf(linha, "%hx: %hx", &endereco, &conteudo) == 2){
            if (endereco < 1024) {
                processador->Memoria[endereco / 2] = conteudo;
                printf("Endereco: 0x%04X | Conteudo: 0x%04X\n", endereco, conteudo);
            }else{
                printf("Erro: Endereco 0x%04X fora dos limites!\n", endereco);
            }
        }else{
            printf("Erro na linha: %s", linha);
        }
    }
    fclose(arq);
}

void executeInstruction(processorsimulator *processador){
    
}

void zerarStruct(processorsimulator *processador){
    for(int i = 0; i < 1024; i++){
        processador->Memoria[i] = 0;
    }
    for(int i = 0; i < 256; i++){
        processador->Pilha[i] = 0;
    }
    for(int i = 0; i < 8; i++){
        processador->Registradores[i] = 0;
    }
    processador->IR = 0;
    processador->SP = 0x8200;
    processador->PC = 0;
    processador->FLAGS.Carry = 0;
    processador->FLAGS.Negative = 0;
    processador->FLAGS.Overflow = 0;
    processador->FLAGS.Zero = 0;
}

int main(){
    processorsimulator processador;
    zerarStruct(&processador);
    readArchive(&processador,"../arquivo.txt");
}