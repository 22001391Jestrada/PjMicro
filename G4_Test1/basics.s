        AREA |.text|, CODE, READONLY, ALIGN=2
                THUMB
                REQUIRE8
                PRESERVE8

                EXPORT register_display
                EXPORT register_modify
                EXPORT run
                EXPORT call


register_display
        LDR     R1, =regs_buffer
        STMIA   R1, {R0-R12}
        MOV     R2, SP
        STR     R2, [R1, #52]

        MOV     R2, LR
        STR     R2, [R1, #56]

        MOV     R2, PC
        STR     R2, [R1, #60]

        LDR     R0, =regs_buffer
        BX      LR

register_modify
        LDR     R2, =BranchTable_Byte   ;
        TBB     [R2,R0]

case_R0
        MOV     R0, R1                  ; Mueve la data (R1) al registro objetivo
        B       mod_end
case_R1
        MOV     R1, R1
        B       mod_end
case_R2
        MOV     R2, R1
        B       mod_end
case_R3
        MOV     R3, R1
        B       mod_end
case_R4
        MOV     R4, R1
        B       mod_end
case_R5
        MOV     R5, R1
        B       mod_end
case_R6
        MOV     R6, R1
        B       mod_end
case_R7
        MOV     R7, R1
        B       mod_end
case_R8
        MOV     R8, R1
        B       mod_end
case_R9
        MOV     R9, R1
        B       mod_end
case_R10
        MOV     R10, R1
        B       mod_end
case_R11
        MOV     R11, R1
        B       mod_end
case_R12
        MOV     R12, R1
        B       mod_end
case_R13
        MOV     R13, R1                 ; Modificar MSP/PSP (cuidado con el stack pointer)
        B       mod_end
case_R14
        MOV     R14, R1                 ; Link Register (LR)
        B       mod_end


BranchTable_Byte
        DCB     0                       ;
        DCB     ((case_R1-case_R0)/2)   ;
        DCB     ((case_R2-case_R0)/2)   ;
        DCB     ((case_R3-case_R0)/2)   ;
        DCB     ((case_R4-case_R0)/2)   ;
        DCB     ((case_R5-case_R0)/2)   ;
        DCB     ((case_R6-case_R0)/2)   ;
        DCB     ((case_R7-case_R0)/2)   ;
        DCB     ((case_R8-case_R0)/2)   ;
        DCB     ((case_R9-case_R0)/2)   ;
        DCB     ((case_R10-case_R0)/2)  ;
        DCB     ((case_R11-case_R0)/2)  ;
        DCB     ((case_R12-case_R0)/2)  ;
        DCB     ((case_R13-case_R0)/2)  ;
        DCB     ((case_R14-case_R0)/2)  ;

mod_end
        BX      LR                      ; Retorna a C

run
        ORR     R0, R0, #1
        BX      R0                      ; Salta a la dirección en R0 (ejecución sin retorno al monitor)

call
        PUSH    {LR}
        ORR     R0, R0, #1
        BLX     R0                      ; Llama a la subrutina en R0 y guarda el retorno en LR
        POP     {PC}

        AREA |.data|, DATA, READWRITE, ALIGN=2
regs_buffer
        SPACE   64                      ; Espacio para los registros

        END
