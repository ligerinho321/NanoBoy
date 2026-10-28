#include "gba.h"

#include "../third_party/cJSON/cJSON.h"


typedef struct _gba_cpu_state_t {
    uint32_t r[16];
    uint32_t r_fiq[7];
    uint32_t r_svc[2];
    uint32_t r_abt[2];
    uint32_t r_irq[2];
    uint32_t r_und[2];
    uint32_t cpsr;
    uint32_t spsr[5];
    uint32_t pipeline[2];
} gba_cpu_state_t;

typedef struct _gba_cpu_transaction_t {
    uint32_t type;
    uint32_t size;
    uint32_t address;
    uint32_t data;
    uint32_t access;
} gba_cpu_transaction_t;

typedef struct _gba_cpu_transactions_t {
    gba_cpu_transaction_t* data;
    uint32_t length;
} gba_cpu_transactions_t;


void gba_cpu_load_state(gba_cpu_state_t* state,cJSON* object){
    int index = 0;
    cJSON* item = NULL;

    cJSON* r = cJSON_GetObjectItemCaseSensitive(object,"R");
    if(!r || !cJSON_IsArray(r) || cJSON_GetArraySize(r) != 16){
        printf("failed load R\n");
    }

    index = 0;
    item = r->child;
    while(item != NULL){
        state->r[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* r_fiq = cJSON_GetObjectItemCaseSensitive(object,"R_fiq");
    if(!r_fiq || !cJSON_IsArray(r_fiq) || cJSON_GetArraySize(r_fiq) != 7){
        printf("failed load R_fiq\n");
    }

    index = 0;
    item = r_fiq->child;
    while(item != NULL){
        state->r_fiq[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* r_svc = cJSON_GetObjectItemCaseSensitive(object,"R_svc");
    if(!r_svc || !cJSON_IsArray(r_svc) || cJSON_GetArraySize(r_svc) != 2){
        printf("faild load R_svc\n");
    }

    index = 0;
    item = r_svc->child;
    while(item != NULL){
        state->r_svc[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* r_abt = cJSON_GetObjectItemCaseSensitive(object,"R_abt");
    if(!r_abt || !cJSON_IsArray(r_abt) || cJSON_GetArraySize(r_abt) != 2){
        printf("failed load R_abt\n");
    }

    index = 0;
    item = r_abt->child;
    while(item != NULL){
        state->r_abt[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* r_irq = cJSON_GetObjectItemCaseSensitive(object,"R_irq");
    if(!r_irq || !cJSON_IsArray(r_irq) || cJSON_GetArraySize(r_irq) != 2){
        printf("failed load R_irq\n");
    }

    index = 0;
    item = r_irq->child;
    while(item != NULL){
        state->r_irq[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* r_und = cJSON_GetObjectItemCaseSensitive(object,"R_und");
    if(!r_und || !cJSON_IsArray(r_und) || cJSON_GetArraySize(r_und) != 2){
        printf("failed load R_und\n");
    }

    index = 0;
    item = r_und->child;
    while(item != NULL){
        state->r_und[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* cpsr = cJSON_GetObjectItemCaseSensitive(object,"CPSR");
    if(!cpsr || !cJSON_IsNumber(cpsr)){
        printf("faild load CPSR\n");
    }

    state->cpsr = cJSON_GetNumberValue(cpsr);

    cJSON* spsr = cJSON_GetObjectItemCaseSensitive(object,"SPSR");
    if(!spsr || !cJSON_IsArray(spsr) || cJSON_GetArraySize(spsr) != 5){
        printf("failed load SPSR\n");
    }

    index = 0;
    item = spsr->child;
    while(item != NULL){
        state->spsr[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }

    cJSON* pipeline = cJSON_GetObjectItemCaseSensitive(object,"pipeline");
    if(!pipeline || !cJSON_IsArray(pipeline) || cJSON_GetArraySize(pipeline) != 2){
        printf("failed load Pipeline\n");
    }

    index = 0;
    item = pipeline->child;
    while(item != NULL){
        state->pipeline[index++] = cJSON_GetNumberValue(item);
        item = item->next;
    }
}

void gba_cpu_load_transactions(gba_cpu_transactions_t* transactions,cJSON* object){
    if(transactions->data != NULL){
        free(transactions->data);
    }

    transactions->length = cJSON_GetArraySize(object);
    transactions->data = malloc(sizeof(gba_cpu_transaction_t) * transactions->length);

    cJSON* transaction_object = object->child;

    gba_cpu_transaction_t* transaction = transactions->data;

    while(transaction_object != NULL){

        cJSON* type = cJSON_GetObjectItemCaseSensitive(transaction_object,"kind");
        transaction->type = cJSON_GetNumberValue(type);

        cJSON* size = cJSON_GetObjectItemCaseSensitive(transaction_object,"size");
        transaction->size = cJSON_GetNumberValue(size);

        cJSON* address = cJSON_GetObjectItemCaseSensitive(transaction_object,"addr");
        transaction->address = cJSON_GetNumberValue(address);

        cJSON* data = cJSON_GetObjectItemCaseSensitive(transaction_object,"data");
        transaction->data = cJSON_GetNumberValue(data);

        cJSON* access = cJSON_GetObjectItemCaseSensitive(transaction_object,"access");
        transaction->access = cJSON_GetNumberValue(access);
        
        ++transaction;

        transaction_object = transaction_object->next;
    }
}

void gba_cpu_apply_state(gba_cpu_t* cpu,gba_cpu_state_t* state){

    for(int i = 0; i < 5; ++i){
        cpu->r8_r12[gba_cpu_none_bank][i] = state->r[i + 8];
        cpu->r8_r12[gba_cpu_fiq_bank][i] = state->r_fiq[i];
    }

    cpu->r13[gba_cpu_none_bank] = state->r[13];
    cpu->r14[gba_cpu_none_bank] = state->r[14];

    cpu->r13[gba_cpu_fiq_bank] = state->r_fiq[5];
    cpu->r14[gba_cpu_fiq_bank] = state->r_fiq[6];

    cpu->r13[gba_cpu_supervisor_bank] = state->r_svc[0];
    cpu->r14[gba_cpu_supervisor_bank] = state->r_svc[1];

    cpu->r13[gba_cpu_abort_bank] = state->r_abt[0];
    cpu->r14[gba_cpu_abort_bank] = state->r_abt[1];

    cpu->r13[gba_cpu_irq_bank] = state->r_irq[0];
    cpu->r14[gba_cpu_irq_bank] = state->r_irq[1];

    cpu->r13[gba_cpu_undefined_bank] = state->r_und[0];
    cpu->r14[gba_cpu_undefined_bank] = state->r_und[1];

    cpu->cpsr = state->cpsr;

    cpu->spsr[gba_cpu_fiq_bank] = state->spsr[0];
    cpu->spsr[gba_cpu_supervisor_bank] = state->spsr[1];
    cpu->spsr[gba_cpu_abort_bank] = state->spsr[2];
    cpu->spsr[gba_cpu_irq_bank] = state->spsr[3];
    cpu->spsr[gba_cpu_undefined_bank] = state->spsr[4];

    cpu->pipeline.decode = state->pipeline[0];
    cpu->pipeline.fetch = state->pipeline[1];


    uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;
    uint8_t mode_bank = gba_cpu_mode_bank[mode];

    for(int i = 0; i < 8; ++i){
        cpu->r[i] = state->r[i];
    }

    if(mode != gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            cpu->r[i + 8] = cpu->r8_r12[gba_cpu_none_bank][i];
        }
    }
    else{
        for(int i = 0; i < 5; ++i){
            cpu->r[i + 8] = cpu->r8_r12[gba_cpu_fiq_bank][i];
        }
    }

    cpu->r[13] = cpu->r13[mode_bank];
    cpu->r[14] = cpu->r14[mode_bank];
    cpu->r[15] = state->r[15];
}

void gba_cpu_compare_state(gba_cpu_t* cpu,gba_cpu_state_t* state){

    uint8_t mode = cpu->cpsr & gba_cpu_mode_flag;

    for(int i = 0; i < 8; ++i){
        if(cpu->r[i] != state->r[i]){
            printf("R%d current: %08X expected: %08X\n",i,cpu->r[i],state->r[i]);
        }
    }

    if(mode != gba_cpu_fiq_mode){
        for(int i = 0; i < 5; ++i){
            if(cpu->r[i + 8] != state->r[i + 8]){
                printf("R%d current: %08X expected: %08X\n",i + 8,cpu->r[i + 8],state->r[i + 8]);
            }
        }
    }
    else{
        for(int i = 0; i < 5; ++i){
            if(cpu->r[i + 8] != state->r_fiq[i]){
                printf("R%d_fiq current: %08X expected: %08X\n",i + 8,cpu->r[i + 8],state->r_fiq[i + 8]);
            }
        }
    }

    uint32_t r13 = 0;
    uint32_t r14 = 0;
    switch(mode){
        case gba_cpu_user_mode:
        case gba_cpu_system_mode:
            r13 = state->r[13];
            r14 = state->r[14];
            break;
        case gba_cpu_fiq_mode:
            r13 = state->r_fiq[5];
            r14 = state->r_fiq[6];
            break;
        case gba_cpu_irq_mode:
            r13 = state->r_irq[0];
            r14 = state->r_irq[1];
            break;
        case gba_cpu_supervisor_mode:
            r13 = state->r_svc[0];
            r14 = state->r_svc[1];
            break;
        case gba_cpu_abort_mode:
            r13 = state->r_abt[0];
            r14 = state->r_abt[1];
            break;
        case gba_cpu_undefined_mode:
            r13 = state->r_und[0];
            r14 = state->r_und[1];
            break;
    }

    if(cpu->r[13] != r13){
        printf("R13 current: %08X expected: %08X\n",cpu->r[13],r13);
    }
    if(cpu->r[14] != r14){
        printf("R14 current: %08X expected: %08X\n",cpu->r[14],r14);
    }

    if(cpu->r[15] != (state->r[15] & ~((cpu->cpsr & gba_cpu_thumb_flag) ? 1 : 3))){
        printf("R15 current: %08X expected: %08X\n",cpu->r[15],state->r[15]);
    }
    /*
    if(cpu->r[15] != state->r[15]){
        printf("R15 current: %08X expected: %08X\n",cpu->r[15],state->r[15]);
    }
    */
    
    if(cpu->cpsr != state->cpsr){
        printf("CPSR current: %08X expected: %08X\n",cpu->cpsr,state->cpsr);
    }

    if(cpu->spsr[gba_cpu_fiq_bank] != state->spsr[0]){
        printf("SPSR_fiq current: %08X expected: %08X\n",cpu->spsr[1],state->spsr[0]);
    }
    if(cpu->spsr[gba_cpu_supervisor_bank] != state->spsr[1]){
        printf("SPSR_svc current: %08X expected: %08X\n",cpu->spsr[2],state->spsr[1]);
    }
    if(cpu->spsr[gba_cpu_abort_bank] != state->spsr[2]){
        printf("SPSR_abt current: %08X expected: %08X\n",cpu->spsr[3],state->spsr[2]);
    }
    if(cpu->spsr[gba_cpu_irq_bank] != state->spsr[3]){
        printf("SPSR_irq current: %08X expected: %08X\n",cpu->spsr[4],state->spsr[3]);
    }
    if(cpu->spsr[gba_cpu_undefined_bank] != state->spsr[4]){
        printf("SPSR_und current: %08X expected: %08X\n",cpu->spsr[5],state->spsr[4]);
    }


    if(cpu->pipeline.decode != state->pipeline[0]){
        printf("Pipeline Decode current: %08X expected: %08X\n",cpu->pipeline.decode,state->pipeline[0]);
    }
    if(cpu->pipeline.fetch != state->pipeline[1]){
        printf("Pipeline Fetch current: %08X expected: %08X\n",cpu->pipeline.fetch,state->pipeline[1]);
    }
}

void gba_cpu_execute_teste(gba_t* gba){
    /*
    arm_b_bl.json
    arm_bx.json (R15 with required misaligned value)
    arm_cdp.json
    arm_data_proc_immediate.json (R15 with required misaligned value)
    arm_data_proc_immediate_shift.json (R15 with required misaligned value)
    arm_data_proc_register_shift.json (R15 with required misaligned value)
    arm_ldm_stm.json (R15 with required misaligned value)
    arm_ldrh_strh.json (R15 with required misaligned value)
    arm_ldrsb_ldrsh.json (R15 with required misaligned value)
    arm_ldr_str_immediate_offset.json (R15 with required misaligned value)
    arm_ldr_str_register_offset.json (R15 with required misaligned value)
    arm_mcr_mrc.json
    arm_mrs.json (Rd with R15 not reload pipeline)
    arm_msr_imm.json (Incorrect results)
    arm_msr_reg.json (Incorrect results)
    arm_mull_mlal.json (R15 with required misaligned value)
    arm_mul_mla.json (R15 with required misaligned value)
    arm_stc_ldc.json (R15 with required misaligned value)
    arm_swi.json
    arm_swp.json (R15 with required misaligned value)
    

    thumb_lsl_lsr_asr.json
    thumb_add_sub.json
    thumb_mov_cmp_add_sub.json
    thumb_data_proc.json (MUL carry flag required)
    thumb_add_cmp_mov_hi.json
    thumb_bx.json (R15 with required misaligned value)
    thumb_ldrb_strb_imm_offset.json
    thumb_ldrh_strh_reg_offset.json
    thumb_ldr_pc_rel.json
    thumb_ldrsb_strb_reg_offset.json
    thumb_ldr_str_sp_rel.json
    thumb_ldr_str_reg_offset.json
    thumb_ldrsh_ldrsb_reg_offset.json
    thumb_ldr_str_imm_offset.json
    thumb_add_sp_or_pc.json
    thumb_add_sub_sp.json
    thumb_push_pop.json (R15 with required misaligned value)
    thumb_ldm_stm.json (R15 with requered misaligned value)
    thumb_bcc.json
    thumb_undefined_bcc.json 
    thumb_swi.json
    thumb_b.json
    thumb_bl_blx_prefix.json
    thumb_bl_suffix.json
    */

    FILE* file = fopen("/home/artur/Downloads/gba_tests/arm_data_proc_immediate.json","r");

    if(!file) return;

    fseek(file,0,SEEK_END);
    
    size_t length = ftell(file);

    fseek(file,0,SEEK_SET);

    char* buffer = malloc(length);

    size_t read = fread(buffer,1,length,file);

    cJSON* json = cJSON_ParseWithLength(buffer,length);

    if(json && cJSON_IsArray(json)){

        cJSON* test = json->child;

        gba_cpu_t* cpu = &gba->cpu;
        gba_cpu_state_t initial_state,final_state;
        gba_cpu_transactions_t transactions = {0};

        //cpu->transactions = &transactions;

        int index = 0;

        while(test != NULL){
            cJSON* initial_object = cJSON_GetObjectItemCaseSensitive(test,"initial");
            
            cJSON* final_object = cJSON_GetObjectItemCaseSensitive(test,"final");

            cJSON* transactions_object = cJSON_GetObjectItemCaseSensitive(test,"transactions");

            gba_cpu_load_state(&initial_state,initial_object);
            gba_cpu_load_state(&final_state,final_object);
            gba_cpu_load_transactions(&transactions,transactions_object);

            gba_cpu_apply_state(cpu,&initial_state);

            //printf("Teste %d\n",index);
            //printf("Opcode %08X\n",cpu->pipeline.execute);

            gba_cpu_execute(cpu);

            gba_cpu_compare_state(cpu,&final_state);

            test = test->next;

            index++;
        }
    }

    cJSON_Delete(json);

    free(buffer);

    fclose(file);
}


void generate_conditional_table(){

    bool table[256] = {0};

    for(int i = 0; i < 16; ++i){
        bool n = i & 0x08;
        bool z = i & 0x04;
        bool c = i & 0x02;
        bool v = i & 0x01;

        bool conditions[16] = {
            z,
            !z,
            c,
            !c,
            n,
            !n,
            v,
            !v,
            c && !z,
            !c || z,
            n == v,
            n != v,
            !z && (n == v),
            z || (n != v),
            true,
            false
        };

        for(int k = 0; k < 16; ++k){
            table[(k << 4) | i] = conditions[k];
        }
    }

    for(int i = 0; i < 16; ++i){
        for(int j = 0; j < 16; ++j){
            printf("%d,",table[(i << 4) | j]);
        }
        printf("\n");
    }
}


int main(int n_args,char** args){

    gba_t* gba = gba_new();

    /*
    Doom (USA, Europe).gba
    Dragon Ball Z - Buu's Fury (USA).gba
    Dragon Ball Z - The Legacy of Goku II (Europe) (En,Fr,De,Es,It).gba
    Legend of Zelda, The - The Minish Cap (USA).gba
    Pokemon - Emerald Version (USA, Europe).gba
    Pokemon - FireRed Version (USA).gba
    V-Rally 3 (USA) (En,Fr,Es).gba
    */

    gba_insert_cartridge(gba,"/home/artur/Downloads/gba_roms/Doom (USA, Europe).gba");

    gba_delete(gba);

    return 0;
}