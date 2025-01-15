#include "gdt.h"

.section .text

.extern KmIsrDispatchRoutine

KmIsrStub:
    /* Save all registers to restore them after the dispatch. */

    pushq %rbp
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rsi
    pushq %rdi
    pushq %rdx
    pushq %rcx
    pushq %rbx
    pushq %rax

    /* Pass the context to the dispatch routine. */
    mov %rsp, %rdi
    call KmIsrDispatchRoutine
    mov %rax, %rsp

    /* Restore all registers. */

    popq %rax
    popq %rbx
    popq %rcx
    popq %rdx
    popq %rdi
    popq %rsi
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    popq %rbp

    /* Pop the vector number and error code. */
    add $16, %rsp

    iretq

.equ ISR_TABLE_STRIDE, 16

.macro ISR_ENTRY vector
    .align ISR_TABLE_STRIDE
    KmIsr\vector:
        pushq \vector
        jmp KmIsrStub
.endm

.macro ISR_ENTRY2 errcode, vector
    .align ISR_TABLE_STRIDE
    KmIsr\vector:
        pushq \errcode
        pushq \vector
        jmp KmIsrStub
.endm

.macro ISR_ENTRY3 vector
    ISR_ENTRY2 \vector, \vector
.endm

.global KmIsrTable
.align ISR_TABLE_STRIDE
KmIsrTable:
    // base interrupt vectors
    ISR_ENTRY2 $0, $0
    ISR_ENTRY2 $0, $1
    ISR_ENTRY2 $0, $2
    ISR_ENTRY2 $0, $3
    ISR_ENTRY2 $0, $4
    ISR_ENTRY2 $0, $5
    ISR_ENTRY2 $0, $6
    ISR_ENTRY2 $0, $7
    ISR_ENTRY $8
    ISR_ENTRY2 $0, $9
    ISR_ENTRY $10
    ISR_ENTRY $11
    ISR_ENTRY $12
    ISR_ENTRY $13
    ISR_ENTRY $14
    ISR_ENTRY2 $0, $15
    ISR_ENTRY2 $0, $16
    ISR_ENTRY $17
    ISR_ENTRY2 $0, $18
    ISR_ENTRY2 $0, $19
    ISR_ENTRY2 $0, $20
    ISR_ENTRY $21
    ISR_ENTRY2 $0, $22
    ISR_ENTRY2 $0, $23
    ISR_ENTRY2 $0, $24
    ISR_ENTRY2 $0, $25
    ISR_ENTRY2 $0, $26
    ISR_ENTRY2 $0, $27
    ISR_ENTRY2 $0, $28
    ISR_ENTRY2 $0, $29
    ISR_ENTRY2 $0, $30
    ISR_ENTRY2 $0, $31

    // for i in range(32, 255):
    //     print(f'ISR_ENTRY3 ${i}')
    ISR_ENTRY3 $32
    ISR_ENTRY3 $33
    ISR_ENTRY3 $34
    ISR_ENTRY3 $35
    ISR_ENTRY3 $36
    ISR_ENTRY3 $37
    ISR_ENTRY3 $38
    ISR_ENTRY3 $39
    ISR_ENTRY3 $40
    ISR_ENTRY3 $41
    ISR_ENTRY3 $42
    ISR_ENTRY3 $43
    ISR_ENTRY3 $44
    ISR_ENTRY3 $45
    ISR_ENTRY3 $46
    ISR_ENTRY3 $47
    ISR_ENTRY3 $48
    ISR_ENTRY3 $49
    ISR_ENTRY3 $50
    ISR_ENTRY3 $51
    ISR_ENTRY3 $52
    ISR_ENTRY3 $53
    ISR_ENTRY3 $54
    ISR_ENTRY3 $55
    ISR_ENTRY3 $56
    ISR_ENTRY3 $57
    ISR_ENTRY3 $58
    ISR_ENTRY3 $59
    ISR_ENTRY3 $60
    ISR_ENTRY3 $61
    ISR_ENTRY3 $62
    ISR_ENTRY3 $63
    ISR_ENTRY3 $64
    ISR_ENTRY3 $65
    ISR_ENTRY3 $66
    ISR_ENTRY3 $67
    ISR_ENTRY3 $68
    ISR_ENTRY3 $69
    ISR_ENTRY3 $70
    ISR_ENTRY3 $71
    ISR_ENTRY3 $72
    ISR_ENTRY3 $73
    ISR_ENTRY3 $74
    ISR_ENTRY3 $75
    ISR_ENTRY3 $76
    ISR_ENTRY3 $77
    ISR_ENTRY3 $78
    ISR_ENTRY3 $79
    ISR_ENTRY3 $80
    ISR_ENTRY3 $81
    ISR_ENTRY3 $82
    ISR_ENTRY3 $83
    ISR_ENTRY3 $84
    ISR_ENTRY3 $85
    ISR_ENTRY3 $86
    ISR_ENTRY3 $87
    ISR_ENTRY3 $88
    ISR_ENTRY3 $89
    ISR_ENTRY3 $90
    ISR_ENTRY3 $91
    ISR_ENTRY3 $92
    ISR_ENTRY3 $93
    ISR_ENTRY3 $94
    ISR_ENTRY3 $95
    ISR_ENTRY3 $96
    ISR_ENTRY3 $97
    ISR_ENTRY3 $98
    ISR_ENTRY3 $99
    ISR_ENTRY3 $100
    ISR_ENTRY3 $101
    ISR_ENTRY3 $102
    ISR_ENTRY3 $103
    ISR_ENTRY3 $104
    ISR_ENTRY3 $105
    ISR_ENTRY3 $106
    ISR_ENTRY3 $107
    ISR_ENTRY3 $108
    ISR_ENTRY3 $109
    ISR_ENTRY3 $110
    ISR_ENTRY3 $111
    ISR_ENTRY3 $112
    ISR_ENTRY3 $113
    ISR_ENTRY3 $114
    ISR_ENTRY3 $115
    ISR_ENTRY3 $116
    ISR_ENTRY3 $117
    ISR_ENTRY3 $118
    ISR_ENTRY3 $119
    ISR_ENTRY3 $120
    ISR_ENTRY3 $121
    ISR_ENTRY3 $122
    ISR_ENTRY3 $123
    ISR_ENTRY3 $124
    ISR_ENTRY3 $125
    ISR_ENTRY3 $126
    ISR_ENTRY3 $127
    ISR_ENTRY3 $128
    ISR_ENTRY3 $129
    ISR_ENTRY3 $130
    ISR_ENTRY3 $131
    ISR_ENTRY3 $132
    ISR_ENTRY3 $133
    ISR_ENTRY3 $134
    ISR_ENTRY3 $135
    ISR_ENTRY3 $136
    ISR_ENTRY3 $137
    ISR_ENTRY3 $138
    ISR_ENTRY3 $139
    ISR_ENTRY3 $140
    ISR_ENTRY3 $141
    ISR_ENTRY3 $142
    ISR_ENTRY3 $143
    ISR_ENTRY3 $144
    ISR_ENTRY3 $145
    ISR_ENTRY3 $146
    ISR_ENTRY3 $147
    ISR_ENTRY3 $148
    ISR_ENTRY3 $149
    ISR_ENTRY3 $150
    ISR_ENTRY3 $151
    ISR_ENTRY3 $152
    ISR_ENTRY3 $153
    ISR_ENTRY3 $154
    ISR_ENTRY3 $155
    ISR_ENTRY3 $156
    ISR_ENTRY3 $157
    ISR_ENTRY3 $158
    ISR_ENTRY3 $159
    ISR_ENTRY3 $160
    ISR_ENTRY3 $161
    ISR_ENTRY3 $162
    ISR_ENTRY3 $163
    ISR_ENTRY3 $164
    ISR_ENTRY3 $165
    ISR_ENTRY3 $166
    ISR_ENTRY3 $167
    ISR_ENTRY3 $168
    ISR_ENTRY3 $169
    ISR_ENTRY3 $170
    ISR_ENTRY3 $171
    ISR_ENTRY3 $172
    ISR_ENTRY3 $173
    ISR_ENTRY3 $174
    ISR_ENTRY3 $175
    ISR_ENTRY3 $176
    ISR_ENTRY3 $177
    ISR_ENTRY3 $178
    ISR_ENTRY3 $179
    ISR_ENTRY3 $180
    ISR_ENTRY3 $181
    ISR_ENTRY3 $182
    ISR_ENTRY3 $183
    ISR_ENTRY3 $184
    ISR_ENTRY3 $185
    ISR_ENTRY3 $186
    ISR_ENTRY3 $187
    ISR_ENTRY3 $188
    ISR_ENTRY3 $189
    ISR_ENTRY3 $190
    ISR_ENTRY3 $191
    ISR_ENTRY3 $192
    ISR_ENTRY3 $193
    ISR_ENTRY3 $194
    ISR_ENTRY3 $195
    ISR_ENTRY3 $196
    ISR_ENTRY3 $197
    ISR_ENTRY3 $198
    ISR_ENTRY3 $199
    ISR_ENTRY3 $200
    ISR_ENTRY3 $201
    ISR_ENTRY3 $202
    ISR_ENTRY3 $203
    ISR_ENTRY3 $204
    ISR_ENTRY3 $205
    ISR_ENTRY3 $206
    ISR_ENTRY3 $207
    ISR_ENTRY3 $208
    ISR_ENTRY3 $209
    ISR_ENTRY3 $210
    ISR_ENTRY3 $211
    ISR_ENTRY3 $212
    ISR_ENTRY3 $213
    ISR_ENTRY3 $214
    ISR_ENTRY3 $215
    ISR_ENTRY3 $216
    ISR_ENTRY3 $217
    ISR_ENTRY3 $218
    ISR_ENTRY3 $219
    ISR_ENTRY3 $220
    ISR_ENTRY3 $221
    ISR_ENTRY3 $222
    ISR_ENTRY3 $223
    ISR_ENTRY3 $224
    ISR_ENTRY3 $225
    ISR_ENTRY3 $226
    ISR_ENTRY3 $227
    ISR_ENTRY3 $228
    ISR_ENTRY3 $229
    ISR_ENTRY3 $230
    ISR_ENTRY3 $231
    ISR_ENTRY3 $232
    ISR_ENTRY3 $233
    ISR_ENTRY3 $234
    ISR_ENTRY3 $235
    ISR_ENTRY3 $236
    ISR_ENTRY3 $237
    ISR_ENTRY3 $238
    ISR_ENTRY3 $239
    ISR_ENTRY3 $240
    ISR_ENTRY3 $241
    ISR_ENTRY3 $242
    ISR_ENTRY3 $243
    ISR_ENTRY3 $244
    ISR_ENTRY3 $245
    ISR_ENTRY3 $246
    ISR_ENTRY3 $247
    ISR_ENTRY3 $248
    ISR_ENTRY3 $249
    ISR_ENTRY3 $250
    ISR_ENTRY3 $251
    ISR_ENTRY3 $252
    ISR_ENTRY3 $253
    ISR_ENTRY3 $254
