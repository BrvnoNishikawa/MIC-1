#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== DEFINIÇÃO DE TIPOS =====
// Um byte tem 8 bits
typedef unsigned char byte;
// Uma palavra tem 32 bits
typedef unsigned int palavra;
// Uma microinstrução tem 64 bits (dos quais só usamos 36)
typedef unsigned long int microinstrucao;

// ===== TAMANHO DA MEMÓRIA =====
// Reduzido para um valor mais razoável
#define TAMANHO_MEMORIA 10000000

// ===== ENDEREÇOS IMPORTANTES =====
#define ENDERECO_INICIO_PROGRAMA 0x0401

// ===== REGISTRADORES =====
// Registradores de acesso à memória
palavra MAR = 0;  // Memory Address Register - Endereço de memória
palavra MDR = 0;  // Memory Data Register - Dados da memória
palavra PC = 0;   // Program Counter - Contador de programa
byte MBR = 0;     // Memory Buffer Register - Buffer de memória

// Registradores de operação da ULA (Unidade Lógica e Aritmética)
palavra SP = 0;   // Stack Pointer - Ponteiro da pilha
palavra LV = 0;   // Local Variable - Variáveis locais
palavra TOS = 0;  // Top Of Stack - Topo da pilha
palavra OPC = 0;  // Operand - Operando
palavra CPP = 0;  // Constant Pool Pointer - Ponteiro para constantes
palavra H = 0;    // Auxiliar para operações

// Registrador de controle
microinstrucao MIR;  // Contém a microinstrução atual
palavra MPC = 0;     // Contém o endereço da próxima microinstrução

// ===== BARRAMENTOS =====
palavra Barramento_B, Barramento_C;

// ===== FLAGS (FLIP-FLOPS) =====
byte flag_negativo = 0;  // Antes era apenas N
byte flag_zero = 0;      // Antes era apenas Z

// ===== DECODIFICAÇÃO DE MICROINSTRUÇÃO =====
byte MIR_B, MIR_Operacao, MIR_Deslocador, MIR_MEM, MIR_pulo;
palavra MIR_C;

// ===== ARMAZENAMENTO =====
microinstrucao Armazenamento[512];  // Memória de controle
byte Memoria[TAMANHO_MEMORIA];      // Memória principal

// ===== CONTROLE DE EXECUÇÃO =====
int continuar_executando = 1;  // Flag para controlar o loop principal

// ===== PROTÓTIPOS DAS FUNÇÕES =====
/**
 * Carrega o microprograma de controle do arquivo para a memória
 * Retorna: 1 se bem sucedido, 0 se houver erro
 */
int carregar_microprogram_de_controle();

/**
 * Carrega o programa a ser executado do arquivo para a memória
 * Parâmetros:
 *   - programa: Nome do arquivo do programa
 * Retorna: 1 se bem sucedido, 0 se houver erro
 */
int carregar_programa(const char *programa);

/**
 * Exibe o estado atual dos registradores, memória e pilha
 */
void exibir_processos();

/**
 * Extrai os campos da microinstrução atual
 */
void decodificar_microinstrucao();

/**
 * Configura o Barramento B de acordo com a microinstrução
 */
void atribuir_barramento_B();

/**
 * Realiza a operação da ULA conforme especificado na microinstrução
 */
void realizar_operacao_ALU();

/**
 * Atribui o valor do Barramento C aos registradores
 */
void atribuir_barramento_C();

/**
 * Executa operações de memória (leitura/escrita)
 */
void operar_memoria();

/**
 * Calcula o próximo endereço de microinstrução
 */
void pular();

/**
 * Exibe um valor em formato binário
 * Parâmetros:
 *   - valor: Ponteiro para o valor a ser exibido
 *   - tipo: Tipo de exibição (1-5)
 */
void binario(void* valor, int tipo);

/**
 * Pausa a execução até o usuário pressionar Enter
 */
void pausar_execucao();

/**
 * Verifica se é hora de encerrar o programa
 */
void verificar_condicao_termino();

// ===== FUNÇÃO PRINCIPAL =====
int main(int argc, const char *argv[]) {
    // Verificar se o programa foi chamado com o nome do arquivo
    if (argc < 2) {
        printf("Erro: Faltou especificar o arquivo do programa.\n");
        printf("Uso: %s <arquivo_do_programa>\n", argv[0]);
        return 1;
    }

    // Carregar o microprograma de controle
    if (!carregar_microprogram_de_controle()) {
        printf("Erro ao carregar o microprograma de controle.\n");
        return 1;
    }
    
    // Carregar o programa
    if (!carregar_programa(argv[1])) {
        printf("Erro ao carregar o programa %s.\n", argv[1]);
        return 1;
    }

    printf("Programa carregado com sucesso! Iniciando execução...\n\n");
    
    // Loop principal - agora com condição de saída
    while (continuar_executando) {
        exibir_processos();
        MIR = Armazenamento[MPC];
        
        decodificar_microinstrucao();
        atribuir_barramento_B();
        realizar_operacao_ALU();
        atribuir_barramento_C();
        operar_memoria();
        pular();
        
        // Verificar se é hora de encerrar o programa
        verificar_condicao_termino();
    }

    printf("Programa finalizado.\n");
    return 0;
}

// ===== IMPLEMENTAÇÃO DAS FUNÇÕES =====

/**
 * Verificar se é hora de encerrar o programa
 * Por exemplo, ao encontrar uma instrução de HALT
 */
void verificar_condicao_termino() {
    // Exemplo: terminar se MBR contém instrução de HALT (0xFF)
    if (MBR == 0xFF) {
        continuar_executando = 0;
        printf("\nInstrução de HALT encontrada. Finalizando programa...\n");
    }
    
    // Outra condição: se PC apontar para uma região inválida
    if (PC >= TAMANHO_MEMORIA) {
        continuar_executando = 0;
        printf("\nErro: PC apontando para endereço inválido. Finalizando programa...\n");
    }
}

/**
 * Pausa a execução até o usuário pressionar Enter
 */
void pausar_execucao() {
    printf("\nPressione Enter para continuar para o próximo ciclo...");
    getchar();
}

/**
 * Carrega o microprograma de controle do arquivo para a memória
 */
int carregar_microprogram_de_controle() {
    FILE* MicroPrograma;
    MicroPrograma = fopen("microprog.rom", "rb");

    if (MicroPrograma == NULL) {
        printf("Erro: Não foi possível abrir o arquivo 'microprog.rom'\n");
        return 0;  // Falha
    }

    size_t lidos = fread(Armazenamento, sizeof(microinstrucao), 512, MicroPrograma);
    fclose(MicroPrograma);
    
    printf("Microprograma carregado: %zu instruções lidas.\n", lidos);
    return 1;  // Sucesso
}

/**
 * Carrega o programa a ser executado do arquivo para a memória
 */
int carregar_programa(const char* prog) {
    if (prog == NULL) {
        printf("Erro: Nome do programa não especificado.\n");
        return 0;  // Falha
    }
    
    FILE* Programa;
    palavra tamanho;
    byte tamanho_temp[4];
    
    Programa = fopen(prog, "rb");
    if (Programa == NULL) {
        printf("Erro: Não foi possível abrir o arquivo '%s'\n", prog);
        return 0;  // Falha
    }

    // Ler tamanho do programa
    if (fread(tamanho_temp, sizeof(byte), 4, Programa) != 4) {
        printf("Erro: Arquivo corrompido ou formato inválido.\n");
        fclose(Programa);
        return 0;  // Falha
    }
    memcpy(&tamanho, tamanho_temp, 4);
    
    // Verificar se o programa cabe na memória
    if (tamanho > TAMANHO_MEMORIA) {
        printf("Erro: Programa muito grande para a memória disponível.\n");
        fclose(Programa);
        return 0;  // Falha
    }

    // Ler os 20 bytes de inicialização
    if (fread(Memoria, sizeof(byte), 20, Programa) != 20) {
        printf("Erro: Falha ao ler bytes de inicialização.\n");
        fclose(Programa);
        return 0;  // Falha
    }

    // Ler o resto do programa
    size_t bytes_lidos = fread(&Memoria[ENDERECO_INICIO_PROGRAMA], sizeof(byte), tamanho - 20, Programa);
    fclose(Programa);
    
    printf("Programa carregado: %zu bytes lidos de um total de %u bytes.\n", 
           bytes_lidos, tamanho - 20);
    return 1;  // Sucesso
}

/**
 * Extrai os campos da microinstrução atual
 */
void decodificar_microinstrucao() {
    // Extrair cada campo da microinstrução usando máscaras de bits
    MIR_B = (MIR) & 0b1111;                  // 4 bits menos significativos
    MIR_MEM = (MIR >> 4) & 0b111;            // Próximos 3 bits
    MIR_C = (MIR >> 7) & 0b111111111;        // Próximos 9 bits
    MIR_Operacao = (MIR >> 16) & 0b111111;   // Próximos 6 bits
    MIR_Deslocador = (MIR >> 22) & 0b11;     // Próximos 2 bits
    MIR_pulo = (MIR >> 24) & 0b111;          // Próximos 3 bits
    MPC = (MIR >> 27) & 0b111111111;         // 9 bits mais significativos
}

/**
 * Configura o Barramento B de acordo com a microinstrução
 */
void atribuir_barramento_B() {
    switch(MIR_B) {
        case 0: Barramento_B = MDR; break;
        case 1: Barramento_B = PC; break;
        case 2: 
            // Carrega MBR com extensão de sinal
            Barramento_B = MBR;
            // Verifica se o bit mais significativo é 1
            if(MBR & (0b10000000)) {
                // Estende o sinal (copia o bit mais significativo)
                Barramento_B = Barramento_B | (0b111111111111111111111111 << 8);
            }
            break;
        case 3: Barramento_B = MBR; break;
        case 4: Barramento_B = SP; break;
        case 5: Barramento_B = LV; break;
        case 6: Barramento_B = CPP; break;
        case 7: Barramento_B = TOS; break;
        case 8: Barramento_B = OPC; break;
        default: 
            printf("Aviso: Código de registrador B inválido: %d\n", MIR_B);
            Barramento_B = -1; 
            break; 
    }
}

/**
 * Realiza a operação da ULA conforme especificado na microinstrução
 */
void realizar_operacao_ALU() {
    // Realizar operação conforme código da microinstrução
    switch(MIR_Operacao) {
        case 12: Barramento_C = H & Barramento_B; break;           // AND
        case 17: Barramento_C = 1; break;                          // Constante 1
        case 18: Barramento_C = -1; break;                         // Constante -1
        case 20: Barramento_C = Barramento_B; break;               // Passa B
        case 24: Barramento_C = H; break;                          // Passa H
        case 26: Barramento_C = ~H; break;                         // NOT H
        case 28: Barramento_C = H | Barramento_B; break;           // OR
        case 44: Barramento_C = ~Barramento_B; break;              // NOT B
        case 53: Barramento_C = Barramento_B + 1; break;           // B + 1
        case 54: Barramento_C = Barramento_B - 1; break;           // B - 1
        case 57: Barramento_C = H + 1; break;                      // H + 1
        case 59: Barramento_C = -H; break;                         // -H
        case 60: Barramento_C = H + Barramento_B; break;           // H + B
        case 61: Barramento_C = H + Barramento_B + 1; break;       // H + B + 1
        case 63: Barramento_C = Barramento_B - H; break;           // B - H
        default:
            printf("Aviso: Código de operação ALU inválido: %d\n", MIR_Operacao);
            break;
    }

    // Atualizar flags de condição
    if (Barramento_C == 0) {
        flag_negativo = 0;
        flag_zero = 1;
    } else {
        flag_negativo = 1;
        flag_zero = 0;
    }

    // Aplicar deslocamento se necessário
    switch(MIR_Deslocador) {
        case 0: break;                               // Sem deslocamento
        case 1: Barramento_C = Barramento_C << 8; break;  // Desloca 8 bits para esquerda
        case 2: Barramento_C = Barramento_C >> 1; break;  // Desloca 1 bit para direita
        default:
            printf("Aviso: Código de deslocador inválido: %d\n", MIR_Deslocador);
            break;
    }
}

/**
 * Atribui o valor do Barramento C aos registradores
 */
void atribuir_barramento_C() {
    // Cada bit em MIR_C controla se um registrador recebe o valor
    if(MIR_C & 0b000000001) MAR = Barramento_C;
    if(MIR_C & 0b000000010) MDR = Barramento_C;
    if(MIR_C & 0b000000100) PC  = Barramento_C;
    if(MIR_C & 0b000001000) SP  = Barramento_C;
    if(MIR_C & 0b000010000) LV  = Barramento_C;
    if(MIR_C & 0b000100000) CPP = Barramento_C;
    if(MIR_C & 0b001000000) TOS = Barramento_C;
    if(MIR_C & 0b010000000) OPC = Barramento_C;
    if(MIR_C & 0b100000000) H   = Barramento_C;
}

/**
 * Executa operações de memória (leitura/escrita)
 */
void operar_memoria() {
    // Verificar limites de memória para evitar acessos inválidos
    if (PC >= TAMANHO_MEMORIA) {
        printf("Erro: Tentativa de acesso fora dos limites da memória (PC=%X)\n", PC);
        continuar_executando = 0;
        return;
    }
    
    if (MAR*4 >= TAMANHO_MEMORIA) {
        printf("Erro: Tentativa de acesso fora dos limites da memória (MAR=%X)\n", MAR);
        continuar_executando = 0;
        return;
    }
    
    // Operações de memória conforme bits em MIR_MEM
    if(MIR_MEM & 0b001) MBR = Memoria[PC];                  // Lê byte da memória para MBR
    if(MIR_MEM & 0b010) memcpy(&MDR, &Memoria[MAR*4], 4);   // Lê palavra da memória para MDR
    if(MIR_MEM & 0b100) memcpy(&Memoria[MAR*4], &MDR, 4);   // Escreve MDR na memória
}

/**
 * Calcula o próximo endereço de microinstrução
 */
void pular() {
    // Modificar MPC com base nas flags e bits de pulo
    if(MIR_pulo & 0b001) MPC = MPC | (flag_negativo << 8);
    if(MIR_pulo & 0b010) MPC = MPC | (flag_zero << 8);
    if(MIR_pulo & 0b100) MPC = MPC | (MBR);
}

/**
 * Exibe o estado atual dos registradores, memória e pilha
 */
void exibir_processos() {
    // Exibir a pilha de operandos se LV e SP estiverem definidos
    if(LV && SP) {
        printf("\t\t  PILHA DE OPERANDOS\n");
        printf("========================================\n");
        printf("     END");
        printf("\t   BINARIO DO VALOR");
        printf(" \t\tVALOR\n");
        
        // Verificar limites da pilha para evitar acessos inválidos
        if (SP*4 >= TAMANHO_MEMORIA || LV*4 >= TAMANHO_MEMORIA) {
            printf("Erro: Ponteiros de pilha inválidos (SP=%X, LV=%X)\n", SP, LV);
        } else {
            // Exibir elementos da pilha
            for(int i = SP; i >= LV; i--) {
                palavra valor;
                memcpy(&valor, &Memoria[i*4], 4);            
                
                if(i == SP) printf("SP ->");
                else if(i == LV) printf("LV ->");
                else printf("     ");

                printf("%X ", i);
                binario(&valor, 1); printf(" ");            
                printf("%d\n", valor);
            }
        }
        printf("========================================\n");
    }

    // Exibir a área do programa
    if(PC >= ENDERECO_INICIO_PROGRAMA) {
        printf("\n\t\t\tArea do Programa\n");
        printf("========================================\n");
        printf("\t\tBinario");
        printf("\t HEX");
        printf("  END DE BYTE\n");
        
        // Verificar limites do PC para evitar acessos inválidos
        if (PC+3 >= TAMANHO_MEMORIA || PC-2 < 0) {
            printf("Aviso: PC próximo dos limites da memória (%X)\n", PC);
        } else {
            // Exibir instruções ao redor do PC
            for(int i = PC - 2; i <= PC + 3; i++) {
                if(i == PC) printf("Em execução >>  ");
                else printf("\t\t");
                
                binario(&Memoria[i], 2);
                printf(" 0x%02X ", Memoria[i]);
                printf("\t%X\n", i);
            }
        }
        printf("========================================\n\n");
    }

    // Exibir registradores
    printf("\t\tREGISTRADORES\n");
    printf("\tBINARIO");
    printf("\t\t\t\tHEX\n");
    printf("MAR: "); binario(&MAR, 3); printf("\t%x\n", MAR);
    printf("MDR: "); binario(&MDR, 3); printf("\t%x\n", MDR);
    printf("PC:  "); binario(&PC, 3); printf("\t%x\n", PC);
    printf("MBR: \t\t"); binario(&MBR, 2); printf("\t\t%x\n", MBR);
    printf("SP:  "); binario(&SP, 3); printf("\t%x\n", SP);
    printf("LV:  "); binario(&LV, 3); printf("\t%x\n", LV);
    printf("CPP: "); binario(&CPP, 3); printf("\t%x\n", CPP);
    printf("TOS: "); binario(&TOS, 3); printf("\t%x\n", TOS);
    printf("OPC: "); binario(&OPC, 3); printf("\t%x\n", OPC);
    printf("H:   "); binario(&H, 3); printf("\t%x\n", H);
    printf("MPC: \t\t"); binario(&MPC, 5); printf("\t%x\n", MPC);
    printf("MIR: "); binario(&MIR, 4); printf("\n");

    // Flags de condição
    printf("Flags: Negativo=%d, Zero=%d\n", flag_negativo, flag_zero);
    
    pausar_execucao();
}

/**
 * Exibe um valor em formato binário
 */
void binario(void* valor, int tipo) {
    switch(tipo) {
        case 1: {
            // Imprime o binário de 4 bytes seguidos
            printf(" ");
            byte aux;
            byte* valorAux = (byte*)valor;
                
            for(int i = 3; i >= 0; i--) {
                aux = *(valorAux + i);
                for(int j = 0; j < 8; j++) {
                    printf("%d", (aux >> 7) & 0b1);
                    aux = aux << 1;
                }
                printf(" ");
            }        
        } break;
        
        case 2: {
            // Imprime o binário de 1 byte
            byte aux;
            aux = *((byte*)(valor));
            for(int j = 0; j < 8; j++) {
                printf("%d", (aux >> 7) & 0b1);
                aux = aux << 1;
            }
        } break;

        case 3: {
            // Imprime o binário de uma palavra
            palavra aux;            
            aux = *((palavra*)(valor));
            for(int j = 0; j < 32; j++) {
                printf("%d", (aux >> 31) & 0b1);
                aux = aux << 1;
            }
        } break;

        case 4: {
            // Imprime o binário de uma microinstrução
            microinstrucao aux;        
            aux = *((microinstrucao*)(valor));
            for(int j = 0; j < 36; j++) {
                if (j == 9 || j == 12 || j == 20 || j == 29 || j == 32) printf(" ");
                printf("%ld", (aux >> 35) & 0b1);
                aux = aux << 1;
            }
        } break;
        
        case 5: {
            // Imprime o binário dos 9 bits do MPC
            palavra aux;
            aux = *((palavra*)(valor)) << 23;
            for(int j = 0; j < 9; j++) {
                printf("%d", (aux >> 31) & 0b1);
                aux = aux << 1;
            }
        } break;
        
        default:
            printf("Erro: Tipo de exibição binária inválido: %d\n", tipo);
            break;
    }
}
