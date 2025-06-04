#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MEM_SIZE 1024
#define REG_COUNT 8
#define STACK_SIZE 256

typedef struct{
    uint16_t reg[REG_COUNT];
    uint16_t mem[MEM_SIZE];
    uint16_t stack[STACK_SIZE];
    
    uint16_t PC;
    uint16_t IR;
    uint16_t SP;
    
    struct{
        uint8_t c;
        uint8_t ov;
        uint8_t z;
        uint8_t s;
    }flags;
    
    bool mem_accessed[MEM_SIZE];
    bool stack_accessed[STACK_SIZE];
    
}CPUState;

void initCPU(CPUState *cpu);
int loadProgram(CPUState *cpu, const char *filename);
void executeCycle(CPUState *cpu);
void executeInstruction(CPUState *cpu, uint16_t instr);
void setFlags(CPUState *cpu, uint16_t result, uint16_t a, uint16_t b, char op);
void dumpState(CPUState *cpu);
bool validReg(uint16_t r);

void initCPU(CPUState *cpu){
    memset(cpu->reg, 0, sizeof(cpu->reg));
    memset(cpu->mem, 0, sizeof(cpu->mem));
    memset(cpu->stack, 0, sizeof(cpu->stack));
    
    cpu->PC = 0;
    cpu->IR = 0;
    cpu->SP = 0x8200;
    
    cpu->flags.c = 0;
    cpu->flags.ov = 0;
    cpu->flags.z = 0;
    cpu->flags.s = 0;
    
    memset(cpu->mem_accessed, false, sizeof(cpu->mem_accessed));
    memset(cpu->stack_accessed, false, sizeof(cpu->stack_accessed));
}

int loadProgram(CPUState *cpu, const char *filename){
    FILE *file = fopen(filename, "r");
    if(!file){
        printf("Erro ao abrir arquivo '%s'\n", filename);
        return 0;
    }

    memset(cpu->mem, 0xFF, sizeof(cpu->mem));

    char buffer[100];
    uint16_t addr, data;
    int count = 0;
    
    printf("Carregando programa:\n");
    printf("==========================================\n");
    
    while(fgets(buffer, sizeof(buffer), file)){
        if(buffer[0] == '\n' || buffer[0] == '#' || buffer[0] == '/') 
            continue;
            
        if(sscanf(buffer, "%4hx: 0x%4hx%*[^;]", &addr, &data) == 2){
            if(addr < MEM_SIZE * 2 && addr % 2 == 0){
                cpu->mem[addr / 2] = data;
                printf("[%04X] = 0x%04X\n", addr, data);
                count++;
            }else{
                printf("AVISO: Endereço 0x%04X inválido\n", addr);
                dumpState(cpu);
                return 0;
            }
        }else{
            printf("AVISO: Formato inválido: %s", buffer);
            dumpState(cpu);
            return 0;
        }
    }
    
    fclose(file);
    printf("==========================================\n");
    printf("%d instruções carregadas com sucesso.\n\n", count);
    
    return count > 0;
}

bool validReg(uint16_t r){
    return r < REG_COUNT;
}

void setFlags(CPUState *cpu, uint16_t result, uint16_t a, uint16_t b, char op){
    cpu->flags.z = (result == 0) ? 1 : 0;
    cpu->flags.s = (result >> 15) & 0x1;
    
    switch(op){
        case '+':
            cpu->flags.c = ((uint32_t)a + (uint32_t)b) > 0xFFFF;
            cpu->flags.ov = ((a & 0x8000) == (b & 0x8000)) && ((result & 0x8000) != (a & 0x8000));
            break;
            
        case '-':
            cpu->flags.c = a >= b;
            cpu->flags.ov = ((a & 0x8000) != (b & 0x8000)) && ((result & 0x8000) != (a & 0x8000));
            break;
            
        case '*':
            {
                uint32_t full_result = (uint32_t)a * (uint32_t)b;
                cpu->flags.c = (full_result > 0xFFFF);
                cpu->flags.ov = (full_result > 0xFFFF);
            }
            break;
            
        default:
            cpu->flags.c = 0;
            cpu->flags.ov = 0;
            break;
    }
}

void dumpState(CPUState *cpu){
    printf("\n--- ESTADO ATUAL DA CPU ---\n");
    
    printf("Registradores:\n");
    for(int i = 0; i < REG_COUNT; i++){
        printf("R%d=0x%04X ", i, cpu->reg[i]);
        if(i == 3) printf("\n");
    }
    
    printf("\nPC=0x%04X IR=0x%04X SP=0x%04X\n", cpu->PC, cpu->IR, cpu->SP);
    
    printf("Flags: C=%d Ov=%d Z=%d S=%d\n", cpu->flags.c, cpu->flags.ov, cpu->flags.z, cpu->flags.s);
           
    printf("\nMemória de Dados:\n");
    bool any_accessed = false;
    for(int i = 0; i < MEM_SIZE; i++){
        if(cpu->mem_accessed[i]){
            printf("0x%04X: 0x%04X\n", i*2, cpu->mem[i]);
            any_accessed = true;
        }
    }
    if(!any_accessed){
        printf("Nenhuma posição de memória acessada\n");
    }
    
    printf("\nPilha:\n");
    any_accessed = false;
    uint16_t base_addr = 0x8200;
    for(int i = 0; i < STACK_SIZE; i++){
        uint16_t addr = base_addr - i*2;
        if(cpu->stack_accessed[i]){
            printf("0x%04X: 0x%04X\n", addr, cpu->stack[i]);
            any_accessed = true;
        }
    }
    if(!any_accessed){
        printf("Pilha Vazia\n");
    }
    
    printf("---------------------------\n");
}

void pushStack(CPUState *cpu, uint16_t value) {
    if ((0x8200 - cpu->SP) / 2 >= STACK_SIZE) {
        printf("ERROR: Stack overflow (SP=0x%04X)\n", cpu->SP);
        return;
    }
    cpu->SP -= 2; // Decrementa SP primeiro
    uint16_t stack_index = (0x8200 - cpu->SP) / 2; // Calcula índice após decrementar
    cpu->stack[stack_index] = value;
    cpu->stack_accessed[stack_index] = true;
}

uint16_t popStack(CPUState *cpu) {
    if (cpu->SP >= 0x8200) { // Verifica se a pilha está vazia
        printf("ERROR: Stack underflow (SP=0x%04X)\n", cpu->SP);
        return 0;
    }
    uint16_t stack_index = (0x8200 - cpu->SP) / 2; // Índice antes de incrementar
    uint16_t value = cpu->stack[stack_index];
    cpu->SP += 2; // Incrementa SP após ler
    return value;
}

void executeInstruction(CPUState *cpu, uint16_t instr){
    if(instr != 0xFFFF){
        cpu->IR = instr;
    }
    
    uint16_t op = (instr >> 12) & 0xF;
    printf("[PC=0x%04X] Instruction=0x%04X: ", cpu->PC, instr);
    bool immediate = (instr >> 11) & 0x1;
    uint16_t rd = (instr >> 8) & 0x7;   // Bits 10-8 (0-7)
    uint16_t rm = (instr >> 5) & 0x7;   // Bits 7-5 (0-7)
    uint16_t rn = (instr >> 2) & 0x7;   // Bits 4-2 (0-7)

    
    switch(op){
        case 0x0:
            if(immediate){
                int16_t immediate = ((int16_t) (instr << 5)) >> 7;
                uint16_t original_pc = cpu->PC;
                
                switch(instr & 0x3){
                    case 0x0: // JMP
                        printf("JMP %+d (PC=%04X", immediate, original_pc);
                        cpu->PC = original_pc + immediate;
                        printf(" → %04X)\n", cpu->PC);
                        break;
                    case 0x1: // JEQ
                        printf("JEQ %+d (PC=%04X", immediate, original_pc);
                        if(cpu->flags.z && !cpu->flags.s){
                            cpu->PC = original_pc + immediate;
                            printf(" → %04X)\n", cpu->PC);
                        }else{
                            cpu->PC = original_pc + 2;
                            printf(" → %04X [no jump])\n", cpu->PC);
                        }
                        break;
                    case 0x2: // JLT
                        printf("JLT %+d (PC=%04X", immediate, original_pc);
                        if(cpu->flags.s && !cpu->flags.z){
                            cpu->PC = original_pc + immediate;
                            printf(" → %04X)\n", cpu->PC);
                        }else{
                            cpu->PC = original_pc + 2;
                            printf(" → %04X [no jump])\n", cpu->PC);
                        }
                        break;
                    default:   // JGT
                        printf("JGT %+d (PC=%04X", immediate, original_pc);
                        if(!cpu->flags.z && !cpu->flags.s){
                            cpu->PC = original_pc + immediate;
                            printf(" → %04X)\n", cpu->PC);
                        }else{
                            cpu->PC = original_pc + 2;
                            printf(" → %04X [no jump])\n", cpu->PC);
                        }
                        break;
                }
            }
            else{
                switch(instr & 0x3){
                    case 0x1: // PUSH
                        printf("PUSH R%d [0x%04X]\n", rn, cpu->reg[rn]);
                        pushStack(cpu, cpu->reg[rn]);
                        break;
                    case 0x2: // POP
                        printf("POP R%d", rd);
                        cpu->reg[rd] = popStack(cpu);
                        printf(" [0x%04X]\n", cpu->reg[rd]);
                        break;
                    case 0x3: {
                        printf("CMP R%d, R%d [0x%04X cmp 0x%04X]\n", rm, rn, cpu->reg[rm], cpu->reg[rn]);
                        uint16_t cmp_result = cpu->reg[rm] - cpu->reg[rn];
                        setFlags(cpu, cmp_result, cpu->reg[rm], cpu->reg[rn], '-');
                        break;
                    }
                    default: //NOP
                        if(op == 0x0 && instr == 0x0000) {
                            printf("NOP\n");
                            dumpState(cpu);
                            break;
                        } else {
                            printf("INSTRUÇÃO DESCONHECIDA (0x%X)\n", instr);
                            dumpState(cpu);
                            exit(1);
                        }
               }
            }
            break;
        
        case 0x1:
        if(immediate){
            uint16_t imm = instr & 0xFF;
            printf("MOV R%d, #%d [0x%02X]\n", rd, imm, imm);
            cpu->reg[rd] = imm;
        }else{
            printf("MOV R%d, R%d [0x%04X]\n", rd, rm, cpu->reg[rm]);
            cpu->reg[rd] = cpu->reg[rm];
        }
        break;
        
        case 0x2:
        if(immediate){
            uint16_t imm = instr & 0xFF;
            printf("STR [R%d], #%d  [mem[0x%04X] = 0x%04X]\n", rd, imm, cpu->reg[rd], imm);
            if(cpu->reg[rd] < MEM_SIZE * 2 && cpu->reg[rd] % 2 == 0){
                cpu->mem[cpu->reg[rd] / 2] = imm;
                cpu->mem_accessed[cpu->reg[rd] / 2] = true;
            }else{
                printf("AVISO: Endereço 0x%04X fora dos limites\n", cpu->reg[rd]);
            }
        }else{
            printf("STR [R%d], R%d  [mem[0x%04X] = 0x%04X]\n", rm, rn, cpu->reg[rm], cpu->reg[rn]);
            if(cpu->reg[rm] < MEM_SIZE * 2 && cpu->reg[rm] % 2 == 0){
                cpu->mem[cpu->reg[rm] / 2] = cpu->reg[rn];
                cpu->mem_accessed[cpu->reg[rm] / 2] = true;
            }else{
                printf("AVISO: Endereço 0x%04X fora dos limites\n", cpu->reg[rm]);
            }
        }
        break;
        
        case 0x3:
        printf("LDR R%d, [R%d]", rd, rm);
        if(cpu->reg[rm] < MEM_SIZE * 2 && cpu->reg[rm] % 2 == 0){
            cpu->reg[rd] = cpu->mem[cpu->reg[rm] / 2];
            printf("  [R%d = mem[0x%04X] = 0x%04X]\n", rd, cpu->reg[rm], cpu->reg[rd]);
        }else{
            printf("  [ERRO: Endereço inválido 0x%04X]\n", cpu->reg[rm]);
        }
        break;
        
        case 0x4:
        printf("ADD R%d, R%d, R%d  [%d + %d = %d]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] + cpu->reg[rn]);
        cpu->reg[rd] = cpu->reg[rm] + cpu->reg[rn];
        setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '+');
        break;
        
        case 0x5:
        printf("SUB R%d, R%d, R%d  [%d - %d = %d]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] - cpu->reg[rn]);
        cpu->reg[rd] = cpu->reg[rm] - cpu->reg[rn];
        setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '-');
        break;
        
        case 0x6:
        printf("MUL R%d, R%d, R%d  [%d * %d = %d]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] * cpu->reg[rn]);
               cpu->reg[rd] = cpu->reg[rm] * cpu->reg[rn];
               setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '*');
               break;
               
        case 0x7:
            printf("AND R%d, R%d, R%d  [0x%04X & 0x%04X = 0x%04X]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] & cpu->reg[rn]);
            cpu->reg[rd] = cpu->reg[rm] & cpu->reg[rn];
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '&');
            break;
               
        case 0x8:
            printf("ORR R%d, R%d, R%d  [0x%04X | 0x%04X = 0x%04X]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] | cpu->reg[rn]);
            cpu->reg[rd] = cpu->reg[rm] | cpu->reg[rn];
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '|');
            break;
               
        case 0x9:
            uint16_t not_result = ~cpu->reg[rm] & 0xFFFF; // Mascarar para 16 bits
            printf("NOT R%d, R%d  [~0x%04X = 0x%04X]\n", rd, rm, cpu->reg[rm], not_result);
            cpu->reg[rd] = not_result;
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], 0, '~');
            break;
               
        case 0xA:
            printf("XOR R%d, R%d, R%d  [0x%04X ^ 0x%04X = 0x%04X]\n", rd, rm, rn, cpu->reg[rm], cpu->reg[rn], cpu->reg[rm] ^ cpu->reg[rn]);
            cpu->reg[rd] = cpu->reg[rm] ^ cpu->reg[rn];
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], cpu->reg[rn], '^');
            break;
               
        case 0xB:
            if(immediate){
                uint16_t shamt = instr & 0x7;
                printf("SHR R%d, R%d, #%d  [0x%04X >> %d = 0x%04X]\n", rd, rm, shamt, cpu->reg[rm], shamt, cpu->reg[rm] >> shamt);
                cpu->reg[rd] = cpu->reg[rm] >> shamt;
            }else{
                printf("SHR R%d, R%d  [0x%04X >> 1 = 0x%04X]\n", rd, rm, cpu->reg[rm], cpu->reg[rm] >> 1);
                cpu->reg[rd] = cpu->reg[rm] >> 1;
            }
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], 0, '>');
            break;
               
        case 0xC:
            if(immediate){
                uint16_t shamt = instr & 0x7;
                printf("SHL R%d, R%d, #%d  [0x%04X << %d = 0x%04X]\n", rd, rm, shamt, cpu->reg[rm], shamt, cpu->reg[rm] << shamt);
                cpu->reg[rd] = cpu->reg[rm] << shamt;
            }else{
                printf("SHL R%d, R%d  [0x%04X << 1 = 0x%04X]\n", rd, rm, cpu->reg[rm], cpu->reg[rm] << 1);
                cpu->reg[rd] = cpu->reg[rm] << 1;
            }
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], 0, '<');
            break;
               
        case 0xD:
            printf("ROR R%d, R%d\n", rd, rm);
            cpu->reg[rd] = (cpu->reg[rm] >> 1) | ((cpu->reg[rm] & 0x0001) << 15);
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], 0, 'r');
            break;
            
        case 0xE:
            printf("ROL R%d, R%d\n", rd, rm);
            cpu->reg[rd] = (cpu->reg[rm] << 1) | ((cpu->reg[rm] & 0x8000) >> 15);
            setFlags(cpu, cpu->reg[rd], cpu->reg[rm], 0, 'l');
            break;
               
        case 0xF:
            if(instr == 0xFFFF){
                printf("HALT\n");
                return;
            }
            break;
            
        default:
            printf("INSTRUÇÃO DESCONHECIDA (0x%X)\n", op);
            dumpState(cpu);
            exit(1);
    }
}
       
void executeCycle(CPUState *cpu){
    printf("\n=== CICLO DE EXECUÇÃO ===\n");
    
    while(cpu->PC < MEM_SIZE * 2 && cpu->mem[cpu->PC / 2] != 0xFFFF){
        uint16_t instr = cpu->mem[cpu->PC / 2];
        
        executeInstruction(cpu, instr);
        cpu->PC += 2;
        
        printf("PRÓXIMO: 0x%04X\n", cpu->mem[cpu->PC / 2]);
    }
    
    if(cpu->mem[cpu->PC / 2] == 0xFFFF){
        executeInstruction(cpu, 0xFFFF);
    }
    
    dumpState(cpu);
}

int main(int argc, char *argv[]){
    CPUState cpu;
    char *filename = "./arquivo.txt";
    
    if(argc > 1){
        filename = argv[1];
    }
    
    printf("\n\n==========================\n");
    printf("===== CPU SIMULATOR =====\n");
    printf("==========================\n\n");
    
    initCPU(&cpu);
    
    if(!loadProgram(&cpu, filename)){
        printf("Erro ao carregar programa. Abortando.\n");
        return 1;
    }
    
    printf("Iniciando a execução do programa...\n");
    executeCycle(&cpu);
    
    printf("\nSimulação completa.\n");
    return 0;
}