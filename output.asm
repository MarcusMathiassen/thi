extern _puts
section .data
	D0 DQ `Hello`, 0 
	D1 DQ `are`, 0 
	D2 DQ `you?`, 0 
	D3 DD 5.000000
global _main
section .text
_main:
	PUSH RBP
	MOV RBP, RSP
	SUB RSP, 16; 4 alloc, 12 padding
.BEGIN:
	MOV EAX, 0
	PUSH RAX
	MOV EAX, DWORD [RBP-4]; load
	POP RAX
	MOV [RBP-4], EAX; store
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 0
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L2
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 1
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L3
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 2
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L4
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 3
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L5
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 4
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L6
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 5
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L7
	MOV EAX, DWORD [RBP-4]; load
	PUSH RAX
	MOV EAX, 6
	POP RCX
	CMP RCX, RAX
	SETE AL
	CMP AL, 1
	JE .L8
.L0:
.L10:
	JMP .L9
.L9:
	MOV EAX, 7
	JMP .END
	JMP .L1
.L2:
.L3:
	MOV RAX, D0; string_ref
	PUSH RAX
	POP RDI
	CALL _puts
	JMP .L1
.L4:
	MOV RAX, D1; string_ref
	PUSH RAX
	POP RDI
	CALL _puts
	JMP .L1
.L5:
	MOV RAX, D2; string_ref
	PUSH RAX
	POP RDI
	CALL _puts
	JMP .L1
.L6:
.L12:
	JMP .L11
.L11:
	MOV EAX, 4
	JMP .END
	JMP .L1
.L7:
.L14:
	JMP .L13
.L13:
	MOV EAX, 5
	JMP .END
	JMP .L1
.L8:
	MOV RAX, D3; float_ref
	MOVSS XMM0, [RAX]; float_ref
	SUB RSP, 8
	MOVSS [RSP], XMM0
	MOVSS XMM0, DWORD [RBP-8]; load
	MOVSS XMM0, DWORD [RSP]
	ADD RSP, 8
	MOVSS [RBP-8], XMM0; store
.L16:
	JMP .L15
.L15:
	MOV EAX, 6
	JMP .END
	JMP .L1
.L1:
.L18:
	JMP .L17
.L17:
	MOV EAX, 1
	JMP .END
.END:
	ADD RSP, 16; 4 alloc, 12 padding
	LEAVE
	RET
