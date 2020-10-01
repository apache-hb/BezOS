
build/boot/bios/bezos.bin:     file format binary


Disassembly of section .data:

0000000000007c00 <.data>:
    7c00:	fa                   	cli    
    7c01:	fc                   	cld    
    7c02:	ea                   	(bad)  
    7c03:	07                   	(bad)  
    7c04:	7c 00                	jl     0x7c06
    7c06:	00 31                	add    BYTE PTR [rcx],dh
    7c08:	c0 8e d8 8e d0 f7 d0 	ror    BYTE PTR [rsi-0x82f7128],0xd0
    7c0f:	8e e0                	mov    fs,eax
    7c11:	b4 41                	mov    ah,0x41
    7c13:	bb aa 55 cd 13       	mov    ebx,0x13cd55aa
    7c18:	81 fb 55 aa 0f 85    	cmp    ebx,0x850faa55
    7c1e:	ef                   	out    dx,eax
    7c1f:	00 0f                	add    BYTE PTR [rdi],cl
    7c21:	82                   	(bad)  
    7c22:	eb 00                	jmp    0x7c24
    7c24:	b4 42                	mov    ah,0x42
    7c26:	be 9c 7d cd 13       	mov    esi,0x13cd7d9c
    7c2b:	0f 82 fb 00 66 c7    	jb     0xffffffffc7667d2c
    7c31:	06                   	(bad)  
    7c32:	a0 7d 00 00 10 00 c7 	movabs al,ds:0x9e06c7001000007d
    7c39:	06 9e 
    7c3b:	7d 68                	jge    0x7ca5
    7c3d:	00 66 c7             	add    BYTE PTR [rsi-0x39],ah
    7c40:	06                   	(bad)  
    7c41:	a4                   	movs   BYTE PTR es:[rdi],BYTE PTR ds:[rsi]
    7c42:	7d 15                	jge    0x7c59
    7c44:	00 00                	add    BYTE PTR [rax],al
    7c46:	00 cd                	add    ch,cl
    7c48:	13 0f                	adc    ecx,DWORD PTR [rdi]
    7c4a:	82                   	(bad)  
    7c4b:	dd 00                	fld    QWORD PTR [rax]
    7c4d:	e8 ae 00 b8 01       	call   0x1b87d00
    7c52:	24 cd                	and    al,0xcd
    7c54:	15 e8 a6 00 fa       	adc    eax,0xfa00a6e8
    7c59:	e8 94 00 b0 ad       	call   0xffffffffadb07cf2
    7c5e:	e6 64                	out    0x64,al
    7c60:	e8 8d 00 b0 d0       	call   0xffffffffd0b07cf2
    7c65:	e6 64                	out    0x64,al
    7c67:	e8 8d 00 e4 60       	call   0x60e47cf9
    7c6c:	66 50                	push   ax
    7c6e:	e8 7f 00 b0 d1       	call   0xffffffffd1b07cf2
    7c73:	e6 64                	out    0x64,al
    7c75:	e8 78 00 66 58       	call   0x58667cf2
    7c7a:	0c 02                	or     al,0x2
    7c7c:	e6 60                	out    0x60,al
    7c7e:	e8 6f 00 b0 ae       	call   0xffffffffaeb07cf2
    7c83:	e6 64                	out    0x64,al
    7c85:	e8 68 00 fb e8       	call   0xffffffffe8fb7cf2
    7c8a:	72 00                	jb     0x7c8c
    7c8c:	e4 92                	in     al,0x92
    7c8e:	0c 02                	or     al,0x2
    7c90:	e6 92                	out    0x92,al
    7c92:	e8 69 00 e9 b0       	call   0xffffffffb0e97d00
    7c97:	00 31                	add    BYTE PTR [rcx],dh
    7c99:	f6 66 31             	mul    BYTE PTR [rsi+0x31]
    7c9c:	db bf 00 70 8e c7    	fstp   TBYTE PTR [rdi-0x38719000]
    7ca2:	bf fd ff 46 66       	mov    edi,0x6646fffd
    7ca7:	b9 18 00 00 00       	mov    ecx,0x18
    7cac:	66 b8 20 e8          	mov    ax,0xe820
    7cb0:	00 00                	add    BYTE PTR [rax],al
    7cb2:	66 ba 50 41          	mov    dx,0x4150
    7cb6:	4d 53                	rex.WRB push r11
    7cb8:	83 ef 18             	sub    edi,0x18
    7cbb:	cd 15                	int    0x15
    7cbd:	0f 82 aa 00 66 83    	jb     0xffffffff83667d6d
    7cc3:	f9                   	stc    
    7cc4:	14 75                	adc    al,0x75
    7cc6:	09 26                	or     DWORD PTR [rsi],esp
    7cc8:	66 c7 45 14 00 00    	mov    WORD PTR [rbp+0x14],0x0
    7cce:	00 00                	add    BYTE PTR [rax],al
    7cd0:	66 83 fb 00          	cmp    bx,0x0
    7cd4:	75 cf                	jne    0x7ca5
    7cd6:	26 89 36             	mov    DWORD PTR es:[rsi],esi
    7cd9:	fd                   	std    
    7cda:	ff                   	(bad)  
    7cdb:	fa                   	cli    
    7cdc:	0f 01 16             	lgdt   [rsi]
    7cdf:	ac                   	lods   al,BYTE PTR ds:[rsi]
    7ce0:	7d 0f                	jge    0x7cf1
    7ce2:	20 c0                	and    al,al
    7ce4:	66 83 c8 01          	or     ax,0x1
    7ce8:	0f 22 c0             	mov    cr0,rax
    7ceb:	ea                   	(bad)  
    7cec:	00 7e 08             	add    BYTE PTR [rsi+0x8],bh
    7cef:	00 e4                	add    ah,ah
    7cf1:	64 a8 02             	fs test al,0x2
    7cf4:	75 fa                	jne    0x7cf0
    7cf6:	c3                   	ret    
    7cf7:	e4 64                	in     al,0x64
    7cf9:	a8 02                	test   al,0x2
    7cfb:	74 fa                	je     0x7cf7
    7cfd:	c3                   	ret    
    7cfe:	3e c7 06 fe 7d 69 69 	mov    DWORD PTR ds:[rsi],0x69697dfe
    7d05:	64 81 3e 0e 7e 69 69 	cmp    DWORD PTR fs:[rsi],0x69697e0e
    7d0c:	75 8a                	jne    0x7c98
    7d0e:	c3                   	ret    
    7d0f:	be 14 7d eb 79       	mov    esi,0x79eb7d14
    7d14:	6d                   	ins    DWORD PTR es:[rdi],dx
    7d15:	69 73 73 69 6e 67 20 	imul   esi,DWORD PTR [rbx+0x73],0x20676e69
    7d1c:	6c                   	ins    BYTE PTR es:[rdi],dx
    7d1d:	62 61                	(bad)  
    7d1f:	20 65 78             	and    BYTE PTR [rbp+0x78],ah
    7d22:	74 65                	je     0x7d89
    7d24:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    7d25:	73 69                	jae    0x7d90
    7d27:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7d28:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    7d29:	00 be 2f 7d eb 5e    	add    BYTE PTR [rsi+0x5eeb7d2f],bh
    7d2f:	66 61                	data16 (bad) 
    7d31:	69 6c 65 64 20 74 6f 	imul   ebp,DWORD PTR [rbp+riz*2+0x64],0x206f7420
    7d38:	20 
    7d39:	72 65                	jb     0x7da0
    7d3b:	61                   	(bad)  
    7d3c:	64 20 66 72          	and    BYTE PTR fs:[rsi+0x72],ah
    7d40:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7d41:	6d                   	ins    DWORD PTR es:[rdi],dx
    7d42:	20 64 69 73          	and    BYTE PTR [rcx+rbp*2+0x73],ah
    7d46:	6b 00 be             	imul   eax,DWORD PTR [rax],0xffffffbe
    7d49:	4d 7d eb             	rex.WRB jge 0x7d37
    7d4c:	40                   	rex
    7d4d:	66 61                	data16 (bad) 
    7d4f:	69 6c 65 64 20 74 6f 	imul   ebp,DWORD PTR [rbp+riz*2+0x64],0x206f7420
    7d56:	20 
    7d57:	65 6e                	outs   dx,BYTE PTR gs:[rsi]
    7d59:	61                   	(bad)  
    7d5a:	62                   	(bad)  
    7d5b:	6c                   	ins    BYTE PTR es:[rdi],dx
    7d5c:	65 20 74 68 65       	and    BYTE PTR gs:[rax+rbp*2+0x65],dh
    7d61:	20 61 32             	and    BYTE PTR [rcx+0x32],ah
    7d64:	30 20                	xor    BYTE PTR [rax],ah
    7d66:	6c                   	ins    BYTE PTR es:[rdi],dx
    7d67:	69 6e 65 00 be 70 7d 	imul   ebp,DWORD PTR [rsi+0x65],0x7d70be00
    7d6e:	eb 1d                	jmp    0x7d8d
    7d70:	66 61                	data16 (bad) 
    7d72:	69 6c 65 64 20 74 6f 	imul   ebp,DWORD PTR [rbp+riz*2+0x64],0x206f7420
    7d79:	20 
    7d7a:	63 6f 6c             	movsxd ebp,DWORD PTR [rdi+0x6c]
    7d7d:	6c                   	ins    BYTE PTR es:[rdi],dx
    7d7e:	65 63 74 20 6d       	movsxd esi,DWORD PTR gs:[rax+riz*1+0x6d]
    7d83:	65 6d                	gs ins DWORD PTR es:[rdi],dx
    7d85:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7d86:	72 79                	jb     0x7e01
    7d88:	20 6d 61             	and    BYTE PTR [rbp+0x61],ch
    7d8b:	70 00                	jo     0x7d8d
    7d8d:	b4 0e                	mov    ah,0xe
    7d8f:	ac                   	lods   al,BYTE PTR ds:[rsi]
    7d90:	08 c0                	or     al,al
    7d92:	74 04                	je     0x7d98
    7d94:	cd 10                	int    0x10
    7d96:	eb f7                	jmp    0x7d8f
    7d98:	fa                   	cli    
    7d99:	f4                   	hlt    
    7d9a:	eb fc                	jmp    0x7d98
    7d9c:	10 00                	adc    BYTE PTR [rax],al
    7d9e:	15 00 00 7e 00       	adc    eax,0x7e0000
    7da3:	00 01                	add    BYTE PTR [rcx],al
    7da5:	00 00                	add    BYTE PTR [rax],al
    7da7:	00 00                	add    BYTE PTR [rax],al
    7da9:	00 00                	add    BYTE PTR [rax],al
    7dab:	00 17                	add    BYTE PTR [rdi],dl
    7dad:	00 ac 7d 00 00 00 00 	add    BYTE PTR [rbp+rdi*2+0x0],ch
    7db4:	ff                   	(bad)  
    7db5:	ff 00                	inc    DWORD PTR [rax]
    7db7:	00 00                	add    BYTE PTR [rax],al
    7db9:	9a                   	(bad)  
    7dba:	cf                   	iret   
    7dbb:	00 ff                	add    bh,bh
    7dbd:	ff 00                	inc    DWORD PTR [rax]
    7dbf:	00 00                	add    BYTE PTR [rax],al
    7dc1:	92                   	xchg   edx,eax
    7dc2:	cf                   	iret   
	...
    7dfb:	00 00                	add    BYTE PTR [rax],al
    7dfd:	00 55 aa             	add    BYTE PTR [rbp-0x56],dl
    7e00:	66 b8 10 00          	mov    ax,0x10
    7e04:	8e d8                	mov    ds,eax
    7e06:	8e d0                	mov    ss,eax
    7e08:	bc 00 7c 00 00       	mov    esp,0x7c00
    7e0d:	b9 e8 03 00 00       	mov    ecx,0x3e8
    7e12:	c7 04 8d 00 80 0b 00 	mov    DWORD PTR [rcx*4+0xb8000],0x0
    7e19:	00 00 00 00 
    7e1d:	49 75 f2             	rex.WB jne 0x7e12
    7e20:	c7 05 00 80 0b 00 00 	mov    DWORD PTR [rip+0xb8000],0x0        # 0xbfe2a
    7e27:	00 00 00 
    7e2a:	9c                   	pushf  
    7e2b:	58                   	pop    rax
    7e2c:	89 c1                	mov    ecx,eax
    7e2e:	35 00 00 20 00       	xor    eax,0x200000
    7e33:	50                   	push   rax
    7e34:	9d                   	popf   
    7e35:	9c                   	pushf  
    7e36:	58                   	pop    rax
    7e37:	51                   	push   rcx
    7e38:	9d                   	popf   
    7e39:	31 c8                	xor    eax,ecx
    7e3b:	0f 84 e4 00 00 00    	je     0x7f25
    7e41:	b8 00 00 00 80       	mov    eax,0x80000000
    7e46:	0f a2                	cpuid  
    7e48:	3d 01 00 00 80       	cmp    eax,0x80000001
    7e4d:	0f 82 f3 00 00 00    	jb     0x7f46
    7e53:	b8 01 00 00 80       	mov    eax,0x80000001
    7e58:	0f a2                	cpuid  
    7e5a:	f7 c2 00 00 00 20    	test   edx,0x20000000
    7e60:	0f 84 fe 00 00 00    	je     0x7f64
    7e66:	b8 00 b0 00 00       	mov    eax,0xb000
    7e6b:	8d 90 00 40 00 00    	lea    edx,[rax+0x4000]
    7e71:	c7 00 00 00 00 00    	mov    DWORD PTR [rax],0x0
    7e77:	83 c0 04             	add    eax,0x4
    7e7a:	39 d0                	cmp    eax,edx
    7e7c:	75 f3                	jne    0x7e71
    7e7e:	a1 88 7f 00 00 81 05 	movabs eax,ds:0x7f88058100007f88
    7e85:	88 7f 
    7e87:	00 00                	add    BYTE PTR [rax],al
    7e89:	00 40 00             	add    BYTE PTR [rax+0x0],al
    7e8c:	00 8d 90 00 10 00    	add    BYTE PTR [rbp+0x100090],cl
    7e92:	00 8d b0 00 20 00    	add    BYTE PTR [rbp+0x2000b0],cl
    7e98:	00 8d 88 00 30 00    	add    BYTE PTR [rbp+0x300088],cl
    7e9e:	00 83 ca 03 c7 40    	add    BYTE PTR [rbx+0x40c703ca],al
    7ea4:	04 00                	add    al,0x0
    7ea6:	00 00                	add    BYTE PTR [rax],al
    7ea8:	00 89 10 89 ca 83    	add    BYTE PTR [rcx-0x7c3576f0],cl
    7eae:	ce                   	(bad)  
    7eaf:	03 89 b0 00 10 00    	add    ecx,DWORD PTR [rcx+0x1000b0]
    7eb5:	00 c7                	add    bh,al
    7eb7:	80 04 10 00          	add    BYTE PTR [rax+rdx*1],0x0
    7ebb:	00 00                	add    BYTE PTR [rax],al
    7ebd:	00 00                	add    BYTE PTR [rax],al
    7ebf:	00 c7                	add    bh,al
    7ec1:	80 00 20             	add    BYTE PTR [rax],0x20
    7ec4:	00 00                	add    BYTE PTR [rax],al
    7ec6:	00 00                	add    BYTE PTR [rax],al
    7ec8:	00 00                	add    BYTE PTR [rax],al
    7eca:	83 ca 03             	or     edx,0x3
    7ecd:	89 90 00 20 00 00    	mov    DWORD PTR [rax+0x2000],edx
    7ed3:	ba 03 00 00 00       	mov    edx,0x3
    7ed8:	89 11                	mov    DWORD PTR [rcx],edx
    7eda:	81 c2 00 10 00 00    	add    edx,0x1000
    7ee0:	c7 41 04 00 00 00 00 	mov    DWORD PTR [rcx+0x4],0x0
    7ee7:	83 c1 08             	add    ecx,0x8
    7eea:	81 fa 03 00 10 00    	cmp    edx,0x100003
    7ef0:	75 e6                	jne    0x7ed8
    7ef2:	0f 22 d8             	mov    cr3,rax
    7ef5:	0f 20 e0             	mov    rax,cr4
    7ef8:	83 c8 20             	or     eax,0x20
    7efb:	0f 22 e0             	mov    cr4,rax
    7efe:	b9 80 00 00 c0       	mov    ecx,0xc0000080
    7f03:	0f 32                	rdmsr  
    7f05:	0d 00 01 00 00       	or     eax,0x100
    7f0a:	0f 30                	wrmsr  
    7f0c:	0f 20 c0             	mov    rax,cr0
    7f0f:	0d 00 00 00 80       	or     eax,0x80000000
    7f14:	0f 22 c0             	mov    cr0,rax
    7f17:	0f 01 15 d0 7f 00 00 	lgdt   [rip+0x7fd0]        # 0xfeee
    7f1e:	ea                   	(bad)  
    7f1f:	da 7f 00             	fidivr DWORD PTR [rdi+0x0]
    7f22:	00 08                	add    BYTE PTR [rax],cl
    7f24:	00 bf 2c 7f 00 00    	add    BYTE PTR [rdi+0x7f2c],bh
    7f2a:	eb 64                	jmp    0x7f90
    7f2c:	63 70 75             	movsxd esi,DWORD PTR [rax+0x75]
    7f2f:	69 64 20 69 6e 73 74 	imul   esp,DWORD PTR [rax+riz*1+0x69],0x7274736e
    7f36:	72 
    7f37:	75 63                	jne    0x7f9c
    7f39:	74 69                	je     0x7fa4
    7f3b:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7f3c:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    7f3d:	20 6d 69             	and    BYTE PTR [rbp+0x69],ch
    7f40:	73 73                	jae    0x7fb5
    7f42:	69 6e 67 00 bf 4d 7f 	imul   ebp,DWORD PTR [rsi+0x67],0x7f4dbf00
    7f49:	00 00                	add    BYTE PTR [rax],al
    7f4b:	eb 43                	jmp    0x7f90
    7f4d:	65 78 74             	gs js  0x7fc4
    7f50:	65 6e                	outs   dx,BYTE PTR gs:[rsi]
    7f52:	64 65 64 20 63 70    	fs gs and BYTE PTR fs:[rbx+0x70],ah
    7f58:	75 69                	jne    0x7fc3
    7f5a:	64 20 6d 69          	and    BYTE PTR fs:[rbp+0x69],ch
    7f5e:	73 73                	jae    0x7fd3
    7f60:	69 6e 67 00 bf 6b 7f 	imul   ebp,DWORD PTR [rsi+0x67],0x7f6bbf00
    7f67:	00 00                	add    BYTE PTR [rax],al
    7f69:	eb 25                	jmp    0x7f90
    7f6b:	6c                   	ins    BYTE PTR es:[rdi],dx
    7f6c:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7f6d:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    7f6e:	67 20 6d 6f          	and    BYTE PTR [ebp+0x6f],ch
    7f72:	64 65 20 6e 6f       	fs and BYTE PTR gs:[rsi+0x6f],ch
    7f77:	74 20                	je     0x7f99
    7f79:	73 75                	jae    0x7ff0
    7f7b:	70 70                	jo     0x7fed
    7f7d:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    7f7e:	72 74                	jb     0x7ff4
    7f80:	65 64 00 90 90 90 90 	gs add BYTE PTR fs:[rax-0x6f6f6f70],dl
    7f87:	90 
    7f88:	00 b0 00 00 00 00    	add    BYTE PTR [rax+0x0],dh
    7f8e:	00 00                	add    BYTE PTR [rax],al
    7f90:	bb 00 80 0b 00       	mov    ebx,0xb8000
    7f95:	b4 07                	mov    ah,0x7
    7f97:	8a 07                	mov    al,BYTE PTR [rdi]
    7f99:	08 c0                	or     al,al
    7f9b:	74 09                	je     0x7fa6
    7f9d:	66 89 03             	mov    WORD PTR [rbx],ax
    7fa0:	83 c3 02             	add    ebx,0x2
    7fa3:	47 eb f1             	rex.RXB jmp 0x7f97
    7fa6:	fa                   	cli    
    7fa7:	f4                   	hlt    
    7fa8:	eb fc                	jmp    0x7fa6
    7faa:	ff                   	(bad)  
    7fab:	ff 00                	inc    DWORD PTR [rax]
	...
    7fb5:	00 00                	add    BYTE PTR [rax],al
    7fb7:	9a                   	(bad)  
    7fb8:	20 00                	and    BYTE PTR [rax],al
    7fba:	00 00                	add    BYTE PTR [rax],al
    7fbc:	00 00                	add    BYTE PTR [rax],al
    7fbe:	00 92 00 00 90 90    	add    BYTE PTR [rdx-0x6f700000],dl
    7fc4:	90                   	nop
    7fc5:	90                   	nop
    7fc6:	90                   	nop
    7fc7:	90                   	nop
    7fc8:	90                   	nop
    7fc9:	90                   	nop
    7fca:	90                   	nop
    7fcb:	90                   	nop
    7fcc:	90                   	nop
    7fcd:	90                   	nop
    7fce:	90                   	nop
    7fcf:	90                   	nop
    7fd0:	17                   	(bad)  
    7fd1:	00 aa 7f 00 00 00    	add    BYTE PTR [rdx+0x7f],ch
    7fd7:	00 00                	add    BYTE PTR [rax],al
    7fd9:	00 bc 00 7c 00 00 48 	add    BYTE PTR [rax+rax*1+0x4800007c],bh
    7fe0:	31 c0                	xor    eax,eax
    7fe2:	48 31 db             	xor    rbx,rbx
    7fe5:	48 31 c9             	xor    rcx,rcx
    7fe8:	48 31 d2             	xor    rdx,rdx
    7feb:	48 31 ed             	xor    rbp,rbp
    7fee:	e8 9d 16 00 00       	call   0x9690
    7ff3:	66 2e 0f 1f 84 00 00 	nop    WORD PTR cs:[rax+rax*1+0x0]
    7ffa:	00 00 00 
    7ffd:	0f 1f 00             	nop    DWORD PTR [rax]
    8000:	55                   	push   rbp
    8001:	48 89 e5             	mov    rbp,rsp
    8004:	48 81 ec 90 00 00 00 	sub    rsp,0x90
    800b:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
    800f:	48 8b 05 72 ff ff ff 	mov    rax,QWORD PTR [rip+0xffffffffffffff72]        # 0x7f88
    8016:	48 89 45 f0          	mov    QWORD PTR [rbp-0x10],rax
    801a:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
    801e:	48 8b 0d 63 ff ff ff 	mov    rcx,QWORD PTR [rip+0xffffffffffffff63]        # 0x7f88
    8025:	48 8d 04 c1          	lea    rax,[rcx+rax*8]
    8029:	48 89 05 58 ff ff ff 	mov    QWORD PTR [rip+0xffffffffffffff58],rax        # 0x7f88
    8030:	48 8d 45 e3          	lea    rax,[rbp-0x1d]
    8034:	48 b9 61 74 65 64 20 	movabs rcx,0x78302064657461
    803b:	30 78 00 
    803e:	48 89 48 05          	mov    QWORD PTR [rax+0x5],rcx
    8042:	48 b9 61 6c 6c 6f 63 	movabs rcx,0x657461636f6c6c61
    8049:	61 74 65 
    804c:	48 89 08             	mov    QWORD PTR [rax],rcx
    804f:	c7 45 dc 00 00 00 00 	mov    DWORD PTR [rbp-0x24],0x0
    8056:	48 63 45 dc          	movsxd rax,DWORD PTR [rbp-0x24]
    805a:	80 7c 05 e3 00       	cmp    BYTE PTR [rbp+rax*1-0x1d],0x0
    805f:	0f 84 19 00 00 00    	je     0x807e
    8065:	8b 45 dc             	mov    eax,DWORD PTR [rbp-0x24]
    8068:	89 c1                	mov    ecx,eax
    806a:	83 c1 01             	add    ecx,0x1
    806d:	89 4d dc             	mov    DWORD PTR [rbp-0x24],ecx
    8070:	48 63 d0             	movsxd rdx,eax
    8073:	8a 44 15 e3          	mov    al,BYTE PTR [rbp+rdx*1-0x1d]
    8077:	e6 e9                	out    0xe9,al
    8079:	e9 d8 ff ff ff       	jmp    0x8056
    807e:	31 c0                	xor    eax,eax
    8080:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
    8084:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    808b:	00 
    808c:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8093:	00 
    8094:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    809b:	00 
    809c:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    80a3:	00 
    80a4:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    80ab:	00 
    80ac:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    80b3:	00 
    80b4:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    80bb:	00 
    80bc:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    80c3:	c7 45 8c 00 00 00 00 	mov    DWORD PTR [rbp-0x74],0x0
    80ca:	48 8b 4d f0          	mov    rcx,QWORD PTR [rbp-0x10]
    80ce:	48 89 4d 80          	mov    QWORD PTR [rbp-0x80],rcx
    80d2:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    80d6:	48 89 85 78 ff ff ff 	mov    QWORD PTR [rbp-0x88],rax
    80dd:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    80e1:	48 c1 e8 04          	shr    rax,0x4
    80e5:	48 89 45 80          	mov    QWORD PTR [rbp-0x80],rax
    80e9:	48 8b 85 78 ff ff ff 	mov    rax,QWORD PTR [rbp-0x88]
    80f0:	48 8b 4d 80          	mov    rcx,QWORD PTR [rbp-0x80]
    80f4:	48 c1 e1 04          	shl    rcx,0x4
    80f8:	48 29 c8             	sub    rax,rcx
    80fb:	48 89 c1             	mov    rcx,rax
    80fe:	48 83 c1 23          	add    rcx,0x23
    8102:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8109:	8a 10                	mov    dl,BYTE PTR [rax]
    810b:	8b 75 8c             	mov    esi,DWORD PTR [rbp-0x74]
    810e:	89 f7                	mov    edi,esi
    8110:	83 c7 01             	add    edi,0x1
    8113:	89 7d 8c             	mov    DWORD PTR [rbp-0x74],edi
    8116:	48 63 c6             	movsxd rax,esi
    8119:	88 54 05 90          	mov    BYTE PTR [rbp+rax*1-0x70],dl
    811d:	48 83 7d 80 00       	cmp    QWORD PTR [rbp-0x80],0x0
    8122:	0f 85 aa ff ff ff    	jne    0x80d2
    8128:	8b 45 8c             	mov    eax,DWORD PTR [rbp-0x74]
    812b:	89 85 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],eax
    8131:	8b 85 74 ff ff ff    	mov    eax,DWORD PTR [rbp-0x8c]
    8137:	89 c1                	mov    ecx,eax
    8139:	83 c1 ff             	add    ecx,0xffffffff
    813c:	89 8d 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],ecx
    8142:	83 f8 00             	cmp    eax,0x0
    8145:	0f 84 12 00 00 00    	je     0x815d
    814b:	48 63 85 74 ff ff ff 	movsxd rax,DWORD PTR [rbp-0x8c]
    8152:	8a 44 05 90          	mov    al,BYTE PTR [rbp+rax*1-0x70]
    8156:	e6 e9                	out    0xe9,al
    8158:	e9 d4 ff ff ff       	jmp    0x8131
    815d:	b0 0a                	mov    al,0xa
    815f:	e6 e9                	out    0xe9,al
    8161:	48 8b 45 f0          	mov    rax,QWORD PTR [rbp-0x10]
    8165:	48 81 c4 90 00 00 00 	add    rsp,0x90
    816c:	5d                   	pop    rbp
    816d:	c3                   	ret    
    816e:	66 90                	xchg   ax,ax
    8170:	55                   	push   rbp
    8171:	48 89 e5             	mov    rbp,rsp
    8174:	48 83 ec 30          	sub    rsp,0x30
    8178:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
    817c:	48 89 75 f0          	mov    QWORD PTR [rbp-0x10],rsi
    8180:	48 89 55 e8          	mov    QWORD PTR [rbp-0x18],rdx
    8184:	48 c7 45 e0 00 00 00 	mov    QWORD PTR [rbp-0x20],0x0
    818b:	00 
    818c:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
    8190:	48 8b 4d f0          	mov    rcx,QWORD PTR [rbp-0x10]
    8194:	48 8b 04 c8          	mov    rax,QWORD PTR [rax+rcx*8]
    8198:	48 25 01 00 00 00    	and    rax,0x1
    819e:	48 83 f8 00          	cmp    rax,0x0
    81a2:	0f 85 59 00 00 00    	jne    0x8201
    81a8:	bf 00 10 00 00       	mov    edi,0x1000
    81ad:	e8 4e fe ff ff       	call   0x8000
    81b2:	48 89 45 e0          	mov    QWORD PTR [rbp-0x20],rax
    81b6:	c7 45 dc 00 00 00 00 	mov    DWORD PTR [rbp-0x24],0x0
    81bd:	81 7d dc 00 02 00 00 	cmp    DWORD PTR [rbp-0x24],0x200
    81c4:	0f 8d 1e 00 00 00    	jge    0x81e8
    81ca:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
    81ce:	48 63 4d dc          	movsxd rcx,DWORD PTR [rbp-0x24]
    81d2:	48 c7 04 c8 00 00 00 	mov    QWORD PTR [rax+rcx*8],0x0
    81d9:	00 
    81da:	8b 45 dc             	mov    eax,DWORD PTR [rbp-0x24]
    81dd:	83 c0 01             	add    eax,0x1
    81e0:	89 45 dc             	mov    DWORD PTR [rbp-0x24],eax
    81e3:	e9 d5 ff ff ff       	jmp    0x81bd
    81e8:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
    81ec:	48 0b 45 e8          	or     rax,QWORD PTR [rbp-0x18]
    81f0:	48 8b 4d f8          	mov    rcx,QWORD PTR [rbp-0x8]
    81f4:	48 8b 55 f0          	mov    rdx,QWORD PTR [rbp-0x10]
    81f8:	48 89 04 d1          	mov    QWORD PTR [rcx+rdx*8],rax
    81fc:	e9 16 00 00 00       	jmp    0x8217
    8201:	48 8b 45 f8          	mov    rax,QWORD PTR [rbp-0x8]
    8205:	48 8b 4d f0          	mov    rcx,QWORD PTR [rbp-0x10]
    8209:	48 8b 04 c8          	mov    rax,QWORD PTR [rax+rcx*8]
    820d:	48 25 fc ff ff ff    	and    rax,0xfffffffffffffffc
    8213:	48 89 45 e0          	mov    QWORD PTR [rbp-0x20],rax
    8217:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
    821b:	48 83 c4 30          	add    rsp,0x30
    821f:	5d                   	pop    rbp
    8220:	c3                   	ret    
    8221:	66 2e 0f 1f 84 00 00 	nop    WORD PTR cs:[rax+rax*1+0x0]
    8228:	00 00 00 
    822b:	0f 1f 44 00 00       	nop    DWORD PTR [rax+rax*1+0x0]
    8230:	55                   	push   rbp
    8231:	48 89 e5             	mov    rbp,rsp
    8234:	48 81 ec 70 06 00 00 	sub    rsp,0x670
    823b:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
    823f:	48 89 75 f0          	mov    QWORD PTR [rbp-0x10],rsi
    8243:	48 89 55 e8          	mov    QWORD PTR [rbp-0x18],rdx
    8247:	48 8d 45 dd          	lea    rax,[rbp-0x23]
    824b:	48 b9 6d 61 70 70 69 	movabs rcx,0x20676e697070616d
    8252:	6e 67 20 
    8255:	48 89 08             	mov    QWORD PTR [rax],rcx
    8258:	c7 40 07 20 30 78 00 	mov    DWORD PTR [rax+0x7],0x783020
    825f:	c7 45 d8 00 00 00 00 	mov    DWORD PTR [rbp-0x28],0x0
    8266:	48 63 45 d8          	movsxd rax,DWORD PTR [rbp-0x28]
    826a:	80 7c 05 dd 00       	cmp    BYTE PTR [rbp+rax*1-0x23],0x0
    826f:	0f 84 19 00 00 00    	je     0x828e
    8275:	8b 45 d8             	mov    eax,DWORD PTR [rbp-0x28]
    8278:	89 c1                	mov    ecx,eax
    827a:	83 c1 01             	add    ecx,0x1
    827d:	89 4d d8             	mov    DWORD PTR [rbp-0x28],ecx
    8280:	48 63 d0             	movsxd rdx,eax
    8283:	8a 44 15 dd          	mov    al,BYTE PTR [rbp+rdx*1-0x23]
    8287:	e6 e9                	out    0xe9,al
    8289:	e9 d8 ff ff ff       	jmp    0x8266
    828e:	31 c0                	xor    eax,eax
    8290:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
    8294:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    829b:	00 
    829c:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    82a3:	00 
    82a4:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    82ab:	00 
    82ac:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    82b3:	00 
    82b4:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    82bb:	00 
    82bc:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    82c3:	00 
    82c4:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    82cb:	00 
    82cc:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    82d3:	c7 45 8c 00 00 00 00 	mov    DWORD PTR [rbp-0x74],0x0
    82da:	48 8b 4d f0          	mov    rcx,QWORD PTR [rbp-0x10]
    82de:	48 89 4d 80          	mov    QWORD PTR [rbp-0x80],rcx
    82e2:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    82e6:	48 89 85 78 ff ff ff 	mov    QWORD PTR [rbp-0x88],rax
    82ed:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    82f1:	48 c1 e8 04          	shr    rax,0x4
    82f5:	48 89 45 80          	mov    QWORD PTR [rbp-0x80],rax
    82f9:	48 8b 85 78 ff ff ff 	mov    rax,QWORD PTR [rbp-0x88]
    8300:	48 8b 4d 80          	mov    rcx,QWORD PTR [rbp-0x80]
    8304:	48 c1 e1 04          	shl    rcx,0x4
    8308:	48 29 c8             	sub    rax,rcx
    830b:	48 89 c1             	mov    rcx,rax
    830e:	48 83 c1 23          	add    rcx,0x23
    8312:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8319:	8a 10                	mov    dl,BYTE PTR [rax]
    831b:	8b 75 8c             	mov    esi,DWORD PTR [rbp-0x74]
    831e:	89 f7                	mov    edi,esi
    8320:	83 c7 01             	add    edi,0x1
    8323:	89 7d 8c             	mov    DWORD PTR [rbp-0x74],edi
    8326:	48 63 c6             	movsxd rax,esi
    8329:	88 54 05 90          	mov    BYTE PTR [rbp+rax*1-0x70],dl
    832d:	48 83 7d 80 00       	cmp    QWORD PTR [rbp-0x80],0x0
    8332:	0f 85 aa ff ff ff    	jne    0x82e2
    8338:	8b 45 8c             	mov    eax,DWORD PTR [rbp-0x74]
    833b:	89 85 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],eax
    8341:	8b 85 74 ff ff ff    	mov    eax,DWORD PTR [rbp-0x8c]
    8347:	89 c1                	mov    ecx,eax
    8349:	83 c1 ff             	add    ecx,0xffffffff
    834c:	89 8d 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],ecx
    8352:	83 f8 00             	cmp    eax,0x0
    8355:	0f 84 12 00 00 00    	je     0x836d
    835b:	48 63 85 74 ff ff ff 	movsxd rax,DWORD PTR [rbp-0x8c]
    8362:	8a 44 05 90          	mov    al,BYTE PTR [rbp+rax*1-0x70]
    8366:	e6 e9                	out    0xe9,al
    8368:	e9 d4 ff ff ff       	jmp    0x8341
    836d:	48 8d 85 6d ff ff ff 	lea    rax,[rbp-0x93]
    8374:	c7 40 03 20 30 78 00 	mov    DWORD PTR [rax+0x3],0x783020
    837b:	c7 00 20 74 6f 20    	mov    DWORD PTR [rax],0x206f7420
    8381:	c7 85 68 ff ff ff 00 	mov    DWORD PTR [rbp-0x98],0x0
    8388:	00 00 00 
    838b:	48 63 85 68 ff ff ff 	movsxd rax,DWORD PTR [rbp-0x98]
    8392:	80 bc 05 6d ff ff ff 	cmp    BYTE PTR [rbp+rax*1-0x93],0x0
    8399:	00 
    839a:	0f 84 22 00 00 00    	je     0x83c2
    83a0:	8b 85 68 ff ff ff    	mov    eax,DWORD PTR [rbp-0x98]
    83a6:	89 c1                	mov    ecx,eax
    83a8:	83 c1 01             	add    ecx,0x1
    83ab:	89 8d 68 ff ff ff    	mov    DWORD PTR [rbp-0x98],ecx
    83b1:	48 63 d0             	movsxd rdx,eax
    83b4:	8a 84 15 6d ff ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x93]
    83bb:	e6 e9                	out    0xe9,al
    83bd:	e9 c9 ff ff ff       	jmp    0x838b
    83c2:	31 c0                	xor    eax,eax
    83c4:	48 8d 8d 20 ff ff ff 	lea    rcx,[rbp-0xe0]
    83cb:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    83d2:	00 
    83d3:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    83da:	00 
    83db:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    83e2:	00 
    83e3:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    83ea:	00 
    83eb:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    83f2:	00 
    83f3:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    83fa:	00 
    83fb:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8402:	00 
    8403:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    840a:	c7 85 1c ff ff ff 00 	mov    DWORD PTR [rbp-0xe4],0x0
    8411:	00 00 00 
    8414:	48 8b 4d e8          	mov    rcx,QWORD PTR [rbp-0x18]
    8418:	48 89 8d 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],rcx
    841f:	48 8b 85 10 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf0]
    8426:	48 89 85 08 ff ff ff 	mov    QWORD PTR [rbp-0xf8],rax
    842d:	48 8b 85 10 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf0]
    8434:	48 c1 e8 04          	shr    rax,0x4
    8438:	48 89 85 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],rax
    843f:	48 8b 85 08 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf8]
    8446:	48 8b 8d 10 ff ff ff 	mov    rcx,QWORD PTR [rbp-0xf0]
    844d:	48 c1 e1 04          	shl    rcx,0x4
    8451:	48 29 c8             	sub    rax,rcx
    8454:	48 89 c1             	mov    rcx,rax
    8457:	48 83 c1 23          	add    rcx,0x23
    845b:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8462:	8a 10                	mov    dl,BYTE PTR [rax]
    8464:	8b b5 1c ff ff ff    	mov    esi,DWORD PTR [rbp-0xe4]
    846a:	89 f7                	mov    edi,esi
    846c:	83 c7 01             	add    edi,0x1
    846f:	89 bd 1c ff ff ff    	mov    DWORD PTR [rbp-0xe4],edi
    8475:	48 63 c6             	movsxd rax,esi
    8478:	88 94 05 20 ff ff ff 	mov    BYTE PTR [rbp+rax*1-0xe0],dl
    847f:	48 83 bd 10 ff ff ff 	cmp    QWORD PTR [rbp-0xf0],0x0
    8486:	00 
    8487:	0f 85 92 ff ff ff    	jne    0x841f
    848d:	8b 85 1c ff ff ff    	mov    eax,DWORD PTR [rbp-0xe4]
    8493:	89 85 04 ff ff ff    	mov    DWORD PTR [rbp-0xfc],eax
    8499:	8b 85 04 ff ff ff    	mov    eax,DWORD PTR [rbp-0xfc]
    849f:	89 c1                	mov    ecx,eax
    84a1:	83 c1 ff             	add    ecx,0xffffffff
    84a4:	89 8d 04 ff ff ff    	mov    DWORD PTR [rbp-0xfc],ecx
    84aa:	83 f8 00             	cmp    eax,0x0
    84ad:	0f 84 15 00 00 00    	je     0x84c8
    84b3:	48 63 85 04 ff ff ff 	movsxd rax,DWORD PTR [rbp-0xfc]
    84ba:	8a 84 05 20 ff ff ff 	mov    al,BYTE PTR [rbp+rax*1-0xe0]
    84c1:	e6 e9                	out    0xe9,al
    84c3:	e9 d1 ff ff ff       	jmp    0x8499
    84c8:	b0 0a                	mov    al,0xa
    84ca:	e6 e9                	out    0xe9,al
    84cc:	48 b9 00 00 00 00 80 	movabs rcx,0xff8000000000
    84d3:	ff 00 00 
    84d6:	48 23 4d e8          	and    rcx,QWORD PTR [rbp-0x18]
    84da:	48 c1 e9 27          	shr    rcx,0x27
    84de:	48 89 8d f8 fe ff ff 	mov    QWORD PTR [rbp-0x108],rcx
    84e5:	48 b9 00 00 00 c0 7f 	movabs rcx,0x7fc0000000
    84ec:	00 00 00 
    84ef:	48 23 4d e8          	and    rcx,QWORD PTR [rbp-0x18]
    84f3:	48 c1 e9 1e          	shr    rcx,0x1e
    84f7:	48 89 8d f0 fe ff ff 	mov    QWORD PTR [rbp-0x110],rcx
    84fe:	48 8b 4d e8          	mov    rcx,QWORD PTR [rbp-0x18]
    8502:	48 81 e1 00 00 e0 3f 	and    rcx,0x3fe00000
    8509:	48 c1 e9 15          	shr    rcx,0x15
    850d:	48 89 8d e8 fe ff ff 	mov    QWORD PTR [rbp-0x118],rcx
    8514:	48 8b 4d e8          	mov    rcx,QWORD PTR [rbp-0x18]
    8518:	48 81 e1 00 f0 1f 00 	and    rcx,0x1ff000
    851f:	48 c1 e9 0c          	shr    rcx,0xc
    8523:	48 89 8d e0 fe ff ff 	mov    QWORD PTR [rbp-0x120],rcx
    852a:	48 8b 7d f8          	mov    rdi,QWORD PTR [rbp-0x8]
    852e:	48 8b b5 f8 fe ff ff 	mov    rsi,QWORD PTR [rbp-0x108]
    8535:	ba 03 00 00 00       	mov    edx,0x3
    853a:	e8 31 fc ff ff       	call   0x8170
    853f:	48 89 85 d8 fe ff ff 	mov    QWORD PTR [rbp-0x128],rax
    8546:	48 8b bd d8 fe ff ff 	mov    rdi,QWORD PTR [rbp-0x128]
    854d:	48 8b b5 f0 fe ff ff 	mov    rsi,QWORD PTR [rbp-0x110]
    8554:	ba 03 00 00 00       	mov    edx,0x3
    8559:	e8 12 fc ff ff       	call   0x8170
    855e:	48 89 85 d0 fe ff ff 	mov    QWORD PTR [rbp-0x130],rax
    8565:	48 8b bd d0 fe ff ff 	mov    rdi,QWORD PTR [rbp-0x130]
    856c:	48 8b b5 e8 fe ff ff 	mov    rsi,QWORD PTR [rbp-0x118]
    8573:	ba 03 00 00 00       	mov    edx,0x3
    8578:	e8 f3 fb ff ff       	call   0x8170
    857d:	48 89 85 c8 fe ff ff 	mov    QWORD PTR [rbp-0x138],rax
    8584:	48 8b 45 f0          	mov    rax,QWORD PTR [rbp-0x10]
    8588:	48 0d 03 00 00 00    	or     rax,0x3
    858e:	48 8b 8d c8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x138]
    8595:	48 8b 95 e0 fe ff ff 	mov    rdx,QWORD PTR [rbp-0x120]
    859c:	48 89 04 d1          	mov    QWORD PTR [rcx+rdx*8],rax
    85a0:	48 8d 85 c5 fe ff ff 	lea    rax,[rbp-0x13b]
    85a7:	c6 40 02 00          	mov    BYTE PTR [rax+0x2],0x0
    85ab:	66 c7 00 30 78       	mov    WORD PTR [rax],0x7830
    85b0:	c7 85 c0 fe ff ff 00 	mov    DWORD PTR [rbp-0x140],0x0
    85b7:	00 00 00 
    85ba:	48 63 85 c0 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x140]
    85c1:	80 bc 05 c5 fe ff ff 	cmp    BYTE PTR [rbp+rax*1-0x13b],0x0
    85c8:	00 
    85c9:	0f 84 22 00 00 00    	je     0x85f1
    85cf:	8b 85 c0 fe ff ff    	mov    eax,DWORD PTR [rbp-0x140]
    85d5:	89 c1                	mov    ecx,eax
    85d7:	83 c1 01             	add    ecx,0x1
    85da:	89 8d c0 fe ff ff    	mov    DWORD PTR [rbp-0x140],ecx
    85e0:	48 63 d0             	movsxd rdx,eax
    85e3:	8a 84 15 c5 fe ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x13b]
    85ea:	e6 e9                	out    0xe9,al
    85ec:	e9 c9 ff ff ff       	jmp    0x85ba
    85f1:	31 c0                	xor    eax,eax
    85f3:	48 8d 8d 80 fe ff ff 	lea    rcx,[rbp-0x180]
    85fa:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8601:	00 
    8602:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8609:	00 
    860a:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8611:	00 
    8612:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8619:	00 
    861a:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8621:	00 
    8622:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8629:	00 
    862a:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8631:	00 
    8632:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8639:	c7 85 7c fe ff ff 00 	mov    DWORD PTR [rbp-0x184],0x0
    8640:	00 00 00 
    8643:	48 8b 4d f8          	mov    rcx,QWORD PTR [rbp-0x8]
    8647:	48 89 8d 70 fe ff ff 	mov    QWORD PTR [rbp-0x190],rcx
    864e:	48 8b 85 70 fe ff ff 	mov    rax,QWORD PTR [rbp-0x190]
    8655:	48 89 85 68 fe ff ff 	mov    QWORD PTR [rbp-0x198],rax
    865c:	48 8b 85 70 fe ff ff 	mov    rax,QWORD PTR [rbp-0x190]
    8663:	48 c1 e8 04          	shr    rax,0x4
    8667:	48 89 85 70 fe ff ff 	mov    QWORD PTR [rbp-0x190],rax
    866e:	48 8b 85 68 fe ff ff 	mov    rax,QWORD PTR [rbp-0x198]
    8675:	48 8b 8d 70 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x190]
    867c:	48 c1 e1 04          	shl    rcx,0x4
    8680:	48 29 c8             	sub    rax,rcx
    8683:	48 89 c1             	mov    rcx,rax
    8686:	48 83 c1 23          	add    rcx,0x23
    868a:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8691:	8a 10                	mov    dl,BYTE PTR [rax]
    8693:	8b b5 7c fe ff ff    	mov    esi,DWORD PTR [rbp-0x184]
    8699:	89 f7                	mov    edi,esi
    869b:	83 c7 01             	add    edi,0x1
    869e:	89 bd 7c fe ff ff    	mov    DWORD PTR [rbp-0x184],edi
    86a4:	48 63 c6             	movsxd rax,esi
    86a7:	88 94 05 80 fe ff ff 	mov    BYTE PTR [rbp+rax*1-0x180],dl
    86ae:	48 83 bd 70 fe ff ff 	cmp    QWORD PTR [rbp-0x190],0x0
    86b5:	00 
    86b6:	0f 85 92 ff ff ff    	jne    0x864e
    86bc:	8b 85 7c fe ff ff    	mov    eax,DWORD PTR [rbp-0x184]
    86c2:	89 85 64 fe ff ff    	mov    DWORD PTR [rbp-0x19c],eax
    86c8:	8b 85 64 fe ff ff    	mov    eax,DWORD PTR [rbp-0x19c]
    86ce:	89 c1                	mov    ecx,eax
    86d0:	83 c1 ff             	add    ecx,0xffffffff
    86d3:	89 8d 64 fe ff ff    	mov    DWORD PTR [rbp-0x19c],ecx
    86d9:	83 f8 00             	cmp    eax,0x0
    86dc:	0f 84 15 00 00 00    	je     0x86f7
    86e2:	48 63 85 64 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x19c]
    86e9:	8a 84 05 80 fe ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x180]
    86f0:	e6 e9                	out    0xe9,al
    86f2:	e9 d1 ff ff ff       	jmp    0x86c8
    86f7:	48 8d 85 5d fe ff ff 	lea    rax,[rbp-0x1a3]
    86fe:	c7 40 03 6c 34 5b 00 	mov    DWORD PTR [rax+0x3],0x5b346c
    8705:	c7 00 20 70 6d 6c    	mov    DWORD PTR [rax],0x6c6d7020
    870b:	c7 85 58 fe ff ff 00 	mov    DWORD PTR [rbp-0x1a8],0x0
    8712:	00 00 00 
    8715:	48 63 85 58 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x1a8]
    871c:	80 bc 05 5d fe ff ff 	cmp    BYTE PTR [rbp+rax*1-0x1a3],0x0
    8723:	00 
    8724:	0f 84 22 00 00 00    	je     0x874c
    872a:	8b 85 58 fe ff ff    	mov    eax,DWORD PTR [rbp-0x1a8]
    8730:	89 c1                	mov    ecx,eax
    8732:	83 c1 01             	add    ecx,0x1
    8735:	89 8d 58 fe ff ff    	mov    DWORD PTR [rbp-0x1a8],ecx
    873b:	48 63 d0             	movsxd rdx,eax
    873e:	8a 84 15 5d fe ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x1a3]
    8745:	e6 e9                	out    0xe9,al
    8747:	e9 c9 ff ff ff       	jmp    0x8715
    874c:	31 c0                	xor    eax,eax
    874e:	48 8d 8d 10 fe ff ff 	lea    rcx,[rbp-0x1f0]
    8755:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    875c:	00 
    875d:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8764:	00 
    8765:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    876c:	00 
    876d:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8774:	00 
    8775:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    877c:	00 
    877d:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8784:	00 
    8785:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    878c:	00 
    878d:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8794:	c7 85 0c fe ff ff 00 	mov    DWORD PTR [rbp-0x1f4],0x0
    879b:	00 00 00 
    879e:	48 8b 8d f8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x108]
    87a5:	48 89 8d 00 fe ff ff 	mov    QWORD PTR [rbp-0x200],rcx
    87ac:	48 8b 85 00 fe ff ff 	mov    rax,QWORD PTR [rbp-0x200]
    87b3:	48 89 85 f8 fd ff ff 	mov    QWORD PTR [rbp-0x208],rax
    87ba:	48 8b 85 00 fe ff ff 	mov    rax,QWORD PTR [rbp-0x200]
    87c1:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    87c8:	cc cc cc 
    87cb:	48 f7 e1             	mul    rcx
    87ce:	48 c1 ea 03          	shr    rdx,0x3
    87d2:	48 89 95 00 fe ff ff 	mov    QWORD PTR [rbp-0x200],rdx
    87d9:	48 8b 85 f8 fd ff ff 	mov    rax,QWORD PTR [rbp-0x208]
    87e0:	48 8b 8d 00 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x200]
    87e7:	48 01 c9             	add    rcx,rcx
    87ea:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    87ee:	48 29 c8             	sub    rax,rcx
    87f1:	48 89 c1             	mov    rcx,rax
    87f4:	48 83 c1 23          	add    rcx,0x23
    87f8:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    87ff:	40 8a 30             	mov    sil,BYTE PTR [rax]
    8802:	8b bd 0c fe ff ff    	mov    edi,DWORD PTR [rbp-0x1f4]
    8808:	41 89 f8             	mov    r8d,edi
    880b:	41 83 c0 01          	add    r8d,0x1
    880f:	44 89 85 0c fe ff ff 	mov    DWORD PTR [rbp-0x1f4],r8d
    8816:	48 63 c7             	movsxd rax,edi
    8819:	40 88 b4 05 10 fe ff 	mov    BYTE PTR [rbp+rax*1-0x1f0],sil
    8820:	ff 
    8821:	48 83 bd 00 fe ff ff 	cmp    QWORD PTR [rbp-0x200],0x0
    8828:	00 
    8829:	0f 85 7d ff ff ff    	jne    0x87ac
    882f:	8b 85 0c fe ff ff    	mov    eax,DWORD PTR [rbp-0x1f4]
    8835:	89 85 f4 fd ff ff    	mov    DWORD PTR [rbp-0x20c],eax
    883b:	8b 85 f4 fd ff ff    	mov    eax,DWORD PTR [rbp-0x20c]
    8841:	89 c1                	mov    ecx,eax
    8843:	83 c1 ff             	add    ecx,0xffffffff
    8846:	89 8d f4 fd ff ff    	mov    DWORD PTR [rbp-0x20c],ecx
    884c:	83 f8 00             	cmp    eax,0x0
    884f:	0f 84 15 00 00 00    	je     0x886a
    8855:	48 63 85 f4 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x20c]
    885c:	8a 84 05 10 fe ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x1f0]
    8863:	e6 e9                	out    0xe9,al
    8865:	e9 d1 ff ff ff       	jmp    0x883b
    886a:	48 8d 85 ed fd ff ff 	lea    rax,[rbp-0x213]
    8871:	c7 40 03 20 30 78 00 	mov    DWORD PTR [rax+0x3],0x783020
    8878:	c7 00 5d 20 3d 20    	mov    DWORD PTR [rax],0x203d205d
    887e:	c7 85 e8 fd ff ff 00 	mov    DWORD PTR [rbp-0x218],0x0
    8885:	00 00 00 
    8888:	48 63 85 e8 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x218]
    888f:	80 bc 05 ed fd ff ff 	cmp    BYTE PTR [rbp+rax*1-0x213],0x0
    8896:	00 
    8897:	0f 84 22 00 00 00    	je     0x88bf
    889d:	8b 85 e8 fd ff ff    	mov    eax,DWORD PTR [rbp-0x218]
    88a3:	89 c1                	mov    ecx,eax
    88a5:	83 c1 01             	add    ecx,0x1
    88a8:	89 8d e8 fd ff ff    	mov    DWORD PTR [rbp-0x218],ecx
    88ae:	48 63 d0             	movsxd rdx,eax
    88b1:	8a 84 15 ed fd ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x213]
    88b8:	e6 e9                	out    0xe9,al
    88ba:	e9 c9 ff ff ff       	jmp    0x8888
    88bf:	31 c0                	xor    eax,eax
    88c1:	48 8d 8d a0 fd ff ff 	lea    rcx,[rbp-0x260]
    88c8:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    88cf:	00 
    88d0:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    88d7:	00 
    88d8:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    88df:	00 
    88e0:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    88e7:	00 
    88e8:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    88ef:	00 
    88f0:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    88f7:	00 
    88f8:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    88ff:	00 
    8900:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8907:	c7 85 9c fd ff ff 00 	mov    DWORD PTR [rbp-0x264],0x0
    890e:	00 00 00 
    8911:	48 8b 4d f8          	mov    rcx,QWORD PTR [rbp-0x8]
    8915:	48 8b 95 f8 fe ff ff 	mov    rdx,QWORD PTR [rbp-0x108]
    891c:	48 8b 0c d1          	mov    rcx,QWORD PTR [rcx+rdx*8]
    8920:	48 89 8d 90 fd ff ff 	mov    QWORD PTR [rbp-0x270],rcx
    8927:	48 8b 85 90 fd ff ff 	mov    rax,QWORD PTR [rbp-0x270]
    892e:	48 89 85 88 fd ff ff 	mov    QWORD PTR [rbp-0x278],rax
    8935:	48 8b 85 90 fd ff ff 	mov    rax,QWORD PTR [rbp-0x270]
    893c:	48 c1 e8 04          	shr    rax,0x4
    8940:	48 89 85 90 fd ff ff 	mov    QWORD PTR [rbp-0x270],rax
    8947:	48 8b 85 88 fd ff ff 	mov    rax,QWORD PTR [rbp-0x278]
    894e:	48 8b 8d 90 fd ff ff 	mov    rcx,QWORD PTR [rbp-0x270]
    8955:	48 c1 e1 04          	shl    rcx,0x4
    8959:	48 29 c8             	sub    rax,rcx
    895c:	48 89 c1             	mov    rcx,rax
    895f:	48 83 c1 23          	add    rcx,0x23
    8963:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    896a:	8a 10                	mov    dl,BYTE PTR [rax]
    896c:	8b b5 9c fd ff ff    	mov    esi,DWORD PTR [rbp-0x264]
    8972:	89 f7                	mov    edi,esi
    8974:	83 c7 01             	add    edi,0x1
    8977:	89 bd 9c fd ff ff    	mov    DWORD PTR [rbp-0x264],edi
    897d:	48 63 c6             	movsxd rax,esi
    8980:	88 94 05 a0 fd ff ff 	mov    BYTE PTR [rbp+rax*1-0x260],dl
    8987:	48 83 bd 90 fd ff ff 	cmp    QWORD PTR [rbp-0x270],0x0
    898e:	00 
    898f:	0f 85 92 ff ff ff    	jne    0x8927
    8995:	8b 85 9c fd ff ff    	mov    eax,DWORD PTR [rbp-0x264]
    899b:	89 85 84 fd ff ff    	mov    DWORD PTR [rbp-0x27c],eax
    89a1:	8b 85 84 fd ff ff    	mov    eax,DWORD PTR [rbp-0x27c]
    89a7:	89 c1                	mov    ecx,eax
    89a9:	83 c1 ff             	add    ecx,0xffffffff
    89ac:	89 8d 84 fd ff ff    	mov    DWORD PTR [rbp-0x27c],ecx
    89b2:	83 f8 00             	cmp    eax,0x0
    89b5:	0f 84 15 00 00 00    	je     0x89d0
    89bb:	48 63 85 84 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x27c]
    89c2:	8a 84 05 a0 fd ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x260]
    89c9:	e6 e9                	out    0xe9,al
    89cb:	e9 d1 ff ff ff       	jmp    0x89a1
    89d0:	b0 0a                	mov    al,0xa
    89d2:	e6 e9                	out    0xe9,al
    89d4:	48 8d 8d 81 fd ff ff 	lea    rcx,[rbp-0x27f]
    89db:	c6 41 02 00          	mov    BYTE PTR [rcx+0x2],0x0
    89df:	66 c7 01 30 78       	mov    WORD PTR [rcx],0x7830
    89e4:	c7 85 7c fd ff ff 00 	mov    DWORD PTR [rbp-0x284],0x0
    89eb:	00 00 00 
    89ee:	48 63 85 7c fd ff ff 	movsxd rax,DWORD PTR [rbp-0x284]
    89f5:	80 bc 05 81 fd ff ff 	cmp    BYTE PTR [rbp+rax*1-0x27f],0x0
    89fc:	00 
    89fd:	0f 84 22 00 00 00    	je     0x8a25
    8a03:	8b 85 7c fd ff ff    	mov    eax,DWORD PTR [rbp-0x284]
    8a09:	89 c1                	mov    ecx,eax
    8a0b:	83 c1 01             	add    ecx,0x1
    8a0e:	89 8d 7c fd ff ff    	mov    DWORD PTR [rbp-0x284],ecx
    8a14:	48 63 d0             	movsxd rdx,eax
    8a17:	8a 84 15 81 fd ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x27f]
    8a1e:	e6 e9                	out    0xe9,al
    8a20:	e9 c9 ff ff ff       	jmp    0x89ee
    8a25:	31 c0                	xor    eax,eax
    8a27:	48 8d 8d 30 fd ff ff 	lea    rcx,[rbp-0x2d0]
    8a2e:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8a35:	00 
    8a36:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8a3d:	00 
    8a3e:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8a45:	00 
    8a46:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8a4d:	00 
    8a4e:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8a55:	00 
    8a56:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8a5d:	00 
    8a5e:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8a65:	00 
    8a66:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8a6d:	c7 85 2c fd ff ff 00 	mov    DWORD PTR [rbp-0x2d4],0x0
    8a74:	00 00 00 
    8a77:	48 8b 8d d8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x128]
    8a7e:	48 89 8d 20 fd ff ff 	mov    QWORD PTR [rbp-0x2e0],rcx
    8a85:	48 8b 85 20 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2e0]
    8a8c:	48 89 85 18 fd ff ff 	mov    QWORD PTR [rbp-0x2e8],rax
    8a93:	48 8b 85 20 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2e0]
    8a9a:	48 c1 e8 04          	shr    rax,0x4
    8a9e:	48 89 85 20 fd ff ff 	mov    QWORD PTR [rbp-0x2e0],rax
    8aa5:	48 8b 85 18 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2e8]
    8aac:	48 8b 8d 20 fd ff ff 	mov    rcx,QWORD PTR [rbp-0x2e0]
    8ab3:	48 c1 e1 04          	shl    rcx,0x4
    8ab7:	48 29 c8             	sub    rax,rcx
    8aba:	48 89 c1             	mov    rcx,rax
    8abd:	48 83 c1 23          	add    rcx,0x23
    8ac1:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8ac8:	8a 10                	mov    dl,BYTE PTR [rax]
    8aca:	8b b5 2c fd ff ff    	mov    esi,DWORD PTR [rbp-0x2d4]
    8ad0:	89 f7                	mov    edi,esi
    8ad2:	83 c7 01             	add    edi,0x1
    8ad5:	89 bd 2c fd ff ff    	mov    DWORD PTR [rbp-0x2d4],edi
    8adb:	48 63 c6             	movsxd rax,esi
    8ade:	88 94 05 30 fd ff ff 	mov    BYTE PTR [rbp+rax*1-0x2d0],dl
    8ae5:	48 83 bd 20 fd ff ff 	cmp    QWORD PTR [rbp-0x2e0],0x0
    8aec:	00 
    8aed:	0f 85 92 ff ff ff    	jne    0x8a85
    8af3:	8b 85 2c fd ff ff    	mov    eax,DWORD PTR [rbp-0x2d4]
    8af9:	89 85 14 fd ff ff    	mov    DWORD PTR [rbp-0x2ec],eax
    8aff:	8b 85 14 fd ff ff    	mov    eax,DWORD PTR [rbp-0x2ec]
    8b05:	89 c1                	mov    ecx,eax
    8b07:	83 c1 ff             	add    ecx,0xffffffff
    8b0a:	89 8d 14 fd ff ff    	mov    DWORD PTR [rbp-0x2ec],ecx
    8b10:	83 f8 00             	cmp    eax,0x0
    8b13:	0f 84 15 00 00 00    	je     0x8b2e
    8b19:	48 63 85 14 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x2ec]
    8b20:	8a 84 05 30 fd ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x2d0]
    8b27:	e6 e9                	out    0xe9,al
    8b29:	e9 d1 ff ff ff       	jmp    0x8aff
    8b2e:	48 8d 85 0d fd ff ff 	lea    rax,[rbp-0x2f3]
    8b35:	c7 40 03 70 74 5b 00 	mov    DWORD PTR [rax+0x3],0x5b7470
    8b3c:	c7 00 20 70 64 70    	mov    DWORD PTR [rax],0x70647020
    8b42:	c7 85 08 fd ff ff 00 	mov    DWORD PTR [rbp-0x2f8],0x0
    8b49:	00 00 00 
    8b4c:	48 63 85 08 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x2f8]
    8b53:	80 bc 05 0d fd ff ff 	cmp    BYTE PTR [rbp+rax*1-0x2f3],0x0
    8b5a:	00 
    8b5b:	0f 84 22 00 00 00    	je     0x8b83
    8b61:	8b 85 08 fd ff ff    	mov    eax,DWORD PTR [rbp-0x2f8]
    8b67:	89 c1                	mov    ecx,eax
    8b69:	83 c1 01             	add    ecx,0x1
    8b6c:	89 8d 08 fd ff ff    	mov    DWORD PTR [rbp-0x2f8],ecx
    8b72:	48 63 d0             	movsxd rdx,eax
    8b75:	8a 84 15 0d fd ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x2f3]
    8b7c:	e6 e9                	out    0xe9,al
    8b7e:	e9 c9 ff ff ff       	jmp    0x8b4c
    8b83:	31 c0                	xor    eax,eax
    8b85:	48 8d 8d c0 fc ff ff 	lea    rcx,[rbp-0x340]
    8b8c:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8b93:	00 
    8b94:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8b9b:	00 
    8b9c:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8ba3:	00 
    8ba4:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8bab:	00 
    8bac:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8bb3:	00 
    8bb4:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8bbb:	00 
    8bbc:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8bc3:	00 
    8bc4:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8bcb:	c7 85 bc fc ff ff 00 	mov    DWORD PTR [rbp-0x344],0x0
    8bd2:	00 00 00 
    8bd5:	48 8b 8d f0 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x110]
    8bdc:	48 89 8d b0 fc ff ff 	mov    QWORD PTR [rbp-0x350],rcx
    8be3:	48 8b 85 b0 fc ff ff 	mov    rax,QWORD PTR [rbp-0x350]
    8bea:	48 89 85 a8 fc ff ff 	mov    QWORD PTR [rbp-0x358],rax
    8bf1:	48 8b 85 b0 fc ff ff 	mov    rax,QWORD PTR [rbp-0x350]
    8bf8:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    8bff:	cc cc cc 
    8c02:	48 f7 e1             	mul    rcx
    8c05:	48 c1 ea 03          	shr    rdx,0x3
    8c09:	48 89 95 b0 fc ff ff 	mov    QWORD PTR [rbp-0x350],rdx
    8c10:	48 8b 85 a8 fc ff ff 	mov    rax,QWORD PTR [rbp-0x358]
    8c17:	48 8b 8d b0 fc ff ff 	mov    rcx,QWORD PTR [rbp-0x350]
    8c1e:	48 01 c9             	add    rcx,rcx
    8c21:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    8c25:	48 29 c8             	sub    rax,rcx
    8c28:	48 89 c1             	mov    rcx,rax
    8c2b:	48 83 c1 23          	add    rcx,0x23
    8c2f:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8c36:	40 8a 30             	mov    sil,BYTE PTR [rax]
    8c39:	8b bd bc fc ff ff    	mov    edi,DWORD PTR [rbp-0x344]
    8c3f:	41 89 f8             	mov    r8d,edi
    8c42:	41 83 c0 01          	add    r8d,0x1
    8c46:	44 89 85 bc fc ff ff 	mov    DWORD PTR [rbp-0x344],r8d
    8c4d:	48 63 c7             	movsxd rax,edi
    8c50:	40 88 b4 05 c0 fc ff 	mov    BYTE PTR [rbp+rax*1-0x340],sil
    8c57:	ff 
    8c58:	48 83 bd b0 fc ff ff 	cmp    QWORD PTR [rbp-0x350],0x0
    8c5f:	00 
    8c60:	0f 85 7d ff ff ff    	jne    0x8be3
    8c66:	8b 85 bc fc ff ff    	mov    eax,DWORD PTR [rbp-0x344]
    8c6c:	89 85 a4 fc ff ff    	mov    DWORD PTR [rbp-0x35c],eax
    8c72:	8b 85 a4 fc ff ff    	mov    eax,DWORD PTR [rbp-0x35c]
    8c78:	89 c1                	mov    ecx,eax
    8c7a:	83 c1 ff             	add    ecx,0xffffffff
    8c7d:	89 8d a4 fc ff ff    	mov    DWORD PTR [rbp-0x35c],ecx
    8c83:	83 f8 00             	cmp    eax,0x0
    8c86:	0f 84 15 00 00 00    	je     0x8ca1
    8c8c:	48 63 85 a4 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x35c]
    8c93:	8a 84 05 c0 fc ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x340]
    8c9a:	e6 e9                	out    0xe9,al
    8c9c:	e9 d1 ff ff ff       	jmp    0x8c72
    8ca1:	48 8d 85 9d fc ff ff 	lea    rax,[rbp-0x363]
    8ca8:	c7 40 03 20 30 78 00 	mov    DWORD PTR [rax+0x3],0x783020
    8caf:	c7 00 5d 20 3d 20    	mov    DWORD PTR [rax],0x203d205d
    8cb5:	c7 85 98 fc ff ff 00 	mov    DWORD PTR [rbp-0x368],0x0
    8cbc:	00 00 00 
    8cbf:	48 63 85 98 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x368]
    8cc6:	80 bc 05 9d fc ff ff 	cmp    BYTE PTR [rbp+rax*1-0x363],0x0
    8ccd:	00 
    8cce:	0f 84 22 00 00 00    	je     0x8cf6
    8cd4:	8b 85 98 fc ff ff    	mov    eax,DWORD PTR [rbp-0x368]
    8cda:	89 c1                	mov    ecx,eax
    8cdc:	83 c1 01             	add    ecx,0x1
    8cdf:	89 8d 98 fc ff ff    	mov    DWORD PTR [rbp-0x368],ecx
    8ce5:	48 63 d0             	movsxd rdx,eax
    8ce8:	8a 84 15 9d fc ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x363]
    8cef:	e6 e9                	out    0xe9,al
    8cf1:	e9 c9 ff ff ff       	jmp    0x8cbf
    8cf6:	31 c0                	xor    eax,eax
    8cf8:	48 8d 8d 50 fc ff ff 	lea    rcx,[rbp-0x3b0]
    8cff:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8d06:	00 
    8d07:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8d0e:	00 
    8d0f:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8d16:	00 
    8d17:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8d1e:	00 
    8d1f:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8d26:	00 
    8d27:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8d2e:	00 
    8d2f:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8d36:	00 
    8d37:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8d3e:	c7 85 4c fc ff ff 00 	mov    DWORD PTR [rbp-0x3b4],0x0
    8d45:	00 00 00 
    8d48:	48 8b 8d d8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x128]
    8d4f:	48 8b 95 f0 fe ff ff 	mov    rdx,QWORD PTR [rbp-0x110]
    8d56:	48 8b 0c d1          	mov    rcx,QWORD PTR [rcx+rdx*8]
    8d5a:	48 89 8d 40 fc ff ff 	mov    QWORD PTR [rbp-0x3c0],rcx
    8d61:	48 8b 85 40 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3c0]
    8d68:	48 89 85 38 fc ff ff 	mov    QWORD PTR [rbp-0x3c8],rax
    8d6f:	48 8b 85 40 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3c0]
    8d76:	48 c1 e8 04          	shr    rax,0x4
    8d7a:	48 89 85 40 fc ff ff 	mov    QWORD PTR [rbp-0x3c0],rax
    8d81:	48 8b 85 38 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3c8]
    8d88:	48 8b 8d 40 fc ff ff 	mov    rcx,QWORD PTR [rbp-0x3c0]
    8d8f:	48 c1 e1 04          	shl    rcx,0x4
    8d93:	48 29 c8             	sub    rax,rcx
    8d96:	48 89 c1             	mov    rcx,rax
    8d99:	48 83 c1 23          	add    rcx,0x23
    8d9d:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8da4:	8a 10                	mov    dl,BYTE PTR [rax]
    8da6:	8b b5 4c fc ff ff    	mov    esi,DWORD PTR [rbp-0x3b4]
    8dac:	89 f7                	mov    edi,esi
    8dae:	83 c7 01             	add    edi,0x1
    8db1:	89 bd 4c fc ff ff    	mov    DWORD PTR [rbp-0x3b4],edi
    8db7:	48 63 c6             	movsxd rax,esi
    8dba:	88 94 05 50 fc ff ff 	mov    BYTE PTR [rbp+rax*1-0x3b0],dl
    8dc1:	48 83 bd 40 fc ff ff 	cmp    QWORD PTR [rbp-0x3c0],0x0
    8dc8:	00 
    8dc9:	0f 85 92 ff ff ff    	jne    0x8d61
    8dcf:	8b 85 4c fc ff ff    	mov    eax,DWORD PTR [rbp-0x3b4]
    8dd5:	89 85 34 fc ff ff    	mov    DWORD PTR [rbp-0x3cc],eax
    8ddb:	8b 85 34 fc ff ff    	mov    eax,DWORD PTR [rbp-0x3cc]
    8de1:	89 c1                	mov    ecx,eax
    8de3:	83 c1 ff             	add    ecx,0xffffffff
    8de6:	89 8d 34 fc ff ff    	mov    DWORD PTR [rbp-0x3cc],ecx
    8dec:	83 f8 00             	cmp    eax,0x0
    8def:	0f 84 15 00 00 00    	je     0x8e0a
    8df5:	48 63 85 34 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x3cc]
    8dfc:	8a 84 05 50 fc ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x3b0]
    8e03:	e6 e9                	out    0xe9,al
    8e05:	e9 d1 ff ff ff       	jmp    0x8ddb
    8e0a:	b0 0a                	mov    al,0xa
    8e0c:	e6 e9                	out    0xe9,al
    8e0e:	48 8d 8d 31 fc ff ff 	lea    rcx,[rbp-0x3cf]
    8e15:	c6 41 02 00          	mov    BYTE PTR [rcx+0x2],0x0
    8e19:	66 c7 01 30 78       	mov    WORD PTR [rcx],0x7830
    8e1e:	c7 85 2c fc ff ff 00 	mov    DWORD PTR [rbp-0x3d4],0x0
    8e25:	00 00 00 
    8e28:	48 63 85 2c fc ff ff 	movsxd rax,DWORD PTR [rbp-0x3d4]
    8e2f:	80 bc 05 31 fc ff ff 	cmp    BYTE PTR [rbp+rax*1-0x3cf],0x0
    8e36:	00 
    8e37:	0f 84 22 00 00 00    	je     0x8e5f
    8e3d:	8b 85 2c fc ff ff    	mov    eax,DWORD PTR [rbp-0x3d4]
    8e43:	89 c1                	mov    ecx,eax
    8e45:	83 c1 01             	add    ecx,0x1
    8e48:	89 8d 2c fc ff ff    	mov    DWORD PTR [rbp-0x3d4],ecx
    8e4e:	48 63 d0             	movsxd rdx,eax
    8e51:	8a 84 15 31 fc ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x3cf]
    8e58:	e6 e9                	out    0xe9,al
    8e5a:	e9 c9 ff ff ff       	jmp    0x8e28
    8e5f:	31 c0                	xor    eax,eax
    8e61:	48 8d 8d e0 fb ff ff 	lea    rcx,[rbp-0x420]
    8e68:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8e6f:	00 
    8e70:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8e77:	00 
    8e78:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8e7f:	00 
    8e80:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8e87:	00 
    8e88:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8e8f:	00 
    8e90:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8e97:	00 
    8e98:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8e9f:	00 
    8ea0:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    8ea7:	c7 85 dc fb ff ff 00 	mov    DWORD PTR [rbp-0x424],0x0
    8eae:	00 00 00 
    8eb1:	48 8b 8d d0 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x130]
    8eb8:	48 89 8d d0 fb ff ff 	mov    QWORD PTR [rbp-0x430],rcx
    8ebf:	48 8b 85 d0 fb ff ff 	mov    rax,QWORD PTR [rbp-0x430]
    8ec6:	48 89 85 c8 fb ff ff 	mov    QWORD PTR [rbp-0x438],rax
    8ecd:	48 8b 85 d0 fb ff ff 	mov    rax,QWORD PTR [rbp-0x430]
    8ed4:	48 c1 e8 04          	shr    rax,0x4
    8ed8:	48 89 85 d0 fb ff ff 	mov    QWORD PTR [rbp-0x430],rax
    8edf:	48 8b 85 c8 fb ff ff 	mov    rax,QWORD PTR [rbp-0x438]
    8ee6:	48 8b 8d d0 fb ff ff 	mov    rcx,QWORD PTR [rbp-0x430]
    8eed:	48 c1 e1 04          	shl    rcx,0x4
    8ef1:	48 29 c8             	sub    rax,rcx
    8ef4:	48 89 c1             	mov    rcx,rax
    8ef7:	48 83 c1 23          	add    rcx,0x23
    8efb:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    8f02:	8a 10                	mov    dl,BYTE PTR [rax]
    8f04:	8b b5 dc fb ff ff    	mov    esi,DWORD PTR [rbp-0x424]
    8f0a:	89 f7                	mov    edi,esi
    8f0c:	83 c7 01             	add    edi,0x1
    8f0f:	89 bd dc fb ff ff    	mov    DWORD PTR [rbp-0x424],edi
    8f15:	48 63 c6             	movsxd rax,esi
    8f18:	88 94 05 e0 fb ff ff 	mov    BYTE PTR [rbp+rax*1-0x420],dl
    8f1f:	48 83 bd d0 fb ff ff 	cmp    QWORD PTR [rbp-0x430],0x0
    8f26:	00 
    8f27:	0f 85 92 ff ff ff    	jne    0x8ebf
    8f2d:	8b 85 dc fb ff ff    	mov    eax,DWORD PTR [rbp-0x424]
    8f33:	89 85 c4 fb ff ff    	mov    DWORD PTR [rbp-0x43c],eax
    8f39:	8b 85 c4 fb ff ff    	mov    eax,DWORD PTR [rbp-0x43c]
    8f3f:	89 c1                	mov    ecx,eax
    8f41:	83 c1 ff             	add    ecx,0xffffffff
    8f44:	89 8d c4 fb ff ff    	mov    DWORD PTR [rbp-0x43c],ecx
    8f4a:	83 f8 00             	cmp    eax,0x0
    8f4d:	0f 84 15 00 00 00    	je     0x8f68
    8f53:	48 63 85 c4 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x43c]
    8f5a:	8a 84 05 e0 fb ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x420]
    8f61:	e6 e9                	out    0xe9,al
    8f63:	e9 d1 ff ff ff       	jmp    0x8f39
    8f68:	48 8d 85 bf fb ff ff 	lea    rax,[rbp-0x441]
    8f6f:	c6 40 04 00          	mov    BYTE PTR [rax+0x4],0x0
    8f73:	c7 00 20 70 64 5b    	mov    DWORD PTR [rax],0x5b647020
    8f79:	c7 85 b8 fb ff ff 00 	mov    DWORD PTR [rbp-0x448],0x0
    8f80:	00 00 00 
    8f83:	48 63 85 b8 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x448]
    8f8a:	80 bc 05 bf fb ff ff 	cmp    BYTE PTR [rbp+rax*1-0x441],0x0
    8f91:	00 
    8f92:	0f 84 22 00 00 00    	je     0x8fba
    8f98:	8b 85 b8 fb ff ff    	mov    eax,DWORD PTR [rbp-0x448]
    8f9e:	89 c1                	mov    ecx,eax
    8fa0:	83 c1 01             	add    ecx,0x1
    8fa3:	89 8d b8 fb ff ff    	mov    DWORD PTR [rbp-0x448],ecx
    8fa9:	48 63 d0             	movsxd rdx,eax
    8fac:	8a 84 15 bf fb ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x441]
    8fb3:	e6 e9                	out    0xe9,al
    8fb5:	e9 c9 ff ff ff       	jmp    0x8f83
    8fba:	31 c0                	xor    eax,eax
    8fbc:	48 8d 8d 70 fb ff ff 	lea    rcx,[rbp-0x490]
    8fc3:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    8fca:	00 
    8fcb:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    8fd2:	00 
    8fd3:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    8fda:	00 
    8fdb:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    8fe2:	00 
    8fe3:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    8fea:	00 
    8feb:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    8ff2:	00 
    8ff3:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    8ffa:	00 
    8ffb:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    9002:	c7 85 6c fb ff ff 00 	mov    DWORD PTR [rbp-0x494],0x0
    9009:	00 00 00 
    900c:	48 8b 8d e8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x118]
    9013:	48 89 8d 60 fb ff ff 	mov    QWORD PTR [rbp-0x4a0],rcx
    901a:	48 8b 85 60 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4a0]
    9021:	48 89 85 58 fb ff ff 	mov    QWORD PTR [rbp-0x4a8],rax
    9028:	48 8b 85 60 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4a0]
    902f:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    9036:	cc cc cc 
    9039:	48 f7 e1             	mul    rcx
    903c:	48 c1 ea 03          	shr    rdx,0x3
    9040:	48 89 95 60 fb ff ff 	mov    QWORD PTR [rbp-0x4a0],rdx
    9047:	48 8b 85 58 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4a8]
    904e:	48 8b 8d 60 fb ff ff 	mov    rcx,QWORD PTR [rbp-0x4a0]
    9055:	48 01 c9             	add    rcx,rcx
    9058:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    905c:	48 29 c8             	sub    rax,rcx
    905f:	48 89 c1             	mov    rcx,rax
    9062:	48 83 c1 23          	add    rcx,0x23
    9066:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    906d:	40 8a 30             	mov    sil,BYTE PTR [rax]
    9070:	8b bd 6c fb ff ff    	mov    edi,DWORD PTR [rbp-0x494]
    9076:	41 89 f8             	mov    r8d,edi
    9079:	41 83 c0 01          	add    r8d,0x1
    907d:	44 89 85 6c fb ff ff 	mov    DWORD PTR [rbp-0x494],r8d
    9084:	48 63 c7             	movsxd rax,edi
    9087:	40 88 b4 05 70 fb ff 	mov    BYTE PTR [rbp+rax*1-0x490],sil
    908e:	ff 
    908f:	48 83 bd 60 fb ff ff 	cmp    QWORD PTR [rbp-0x4a0],0x0
    9096:	00 
    9097:	0f 85 7d ff ff ff    	jne    0x901a
    909d:	8b 85 6c fb ff ff    	mov    eax,DWORD PTR [rbp-0x494]
    90a3:	89 85 54 fb ff ff    	mov    DWORD PTR [rbp-0x4ac],eax
    90a9:	8b 85 54 fb ff ff    	mov    eax,DWORD PTR [rbp-0x4ac]
    90af:	89 c1                	mov    ecx,eax
    90b1:	83 c1 ff             	add    ecx,0xffffffff
    90b4:	89 8d 54 fb ff ff    	mov    DWORD PTR [rbp-0x4ac],ecx
    90ba:	83 f8 00             	cmp    eax,0x0
    90bd:	0f 84 15 00 00 00    	je     0x90d8
    90c3:	48 63 85 54 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x4ac]
    90ca:	8a 84 05 70 fb ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x490]
    90d1:	e6 e9                	out    0xe9,al
    90d3:	e9 d1 ff ff ff       	jmp    0x90a9
    90d8:	48 8d 85 4d fb ff ff 	lea    rax,[rbp-0x4b3]
    90df:	c7 40 03 20 30 78 00 	mov    DWORD PTR [rax+0x3],0x783020
    90e6:	c7 00 5d 20 3d 20    	mov    DWORD PTR [rax],0x203d205d
    90ec:	c7 85 48 fb ff ff 00 	mov    DWORD PTR [rbp-0x4b8],0x0
    90f3:	00 00 00 
    90f6:	48 63 85 48 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x4b8]
    90fd:	80 bc 05 4d fb ff ff 	cmp    BYTE PTR [rbp+rax*1-0x4b3],0x0
    9104:	00 
    9105:	0f 84 22 00 00 00    	je     0x912d
    910b:	8b 85 48 fb ff ff    	mov    eax,DWORD PTR [rbp-0x4b8]
    9111:	89 c1                	mov    ecx,eax
    9113:	83 c1 01             	add    ecx,0x1
    9116:	89 8d 48 fb ff ff    	mov    DWORD PTR [rbp-0x4b8],ecx
    911c:	48 63 d0             	movsxd rdx,eax
    911f:	8a 84 15 4d fb ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x4b3]
    9126:	e6 e9                	out    0xe9,al
    9128:	e9 c9 ff ff ff       	jmp    0x90f6
    912d:	31 c0                	xor    eax,eax
    912f:	48 8d 8d 00 fb ff ff 	lea    rcx,[rbp-0x500]
    9136:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    913d:	00 
    913e:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    9145:	00 
    9146:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    914d:	00 
    914e:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    9155:	00 
    9156:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    915d:	00 
    915e:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    9165:	00 
    9166:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    916d:	00 
    916e:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    9175:	c7 85 fc fa ff ff 00 	mov    DWORD PTR [rbp-0x504],0x0
    917c:	00 00 00 
    917f:	48 8b 8d d0 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x130]
    9186:	48 8b 95 e8 fe ff ff 	mov    rdx,QWORD PTR [rbp-0x118]
    918d:	48 8b 0c d1          	mov    rcx,QWORD PTR [rcx+rdx*8]
    9191:	48 89 8d f0 fa ff ff 	mov    QWORD PTR [rbp-0x510],rcx
    9198:	48 8b 85 f0 fa ff ff 	mov    rax,QWORD PTR [rbp-0x510]
    919f:	48 89 85 e8 fa ff ff 	mov    QWORD PTR [rbp-0x518],rax
    91a6:	48 8b 85 f0 fa ff ff 	mov    rax,QWORD PTR [rbp-0x510]
    91ad:	48 c1 e8 04          	shr    rax,0x4
    91b1:	48 89 85 f0 fa ff ff 	mov    QWORD PTR [rbp-0x510],rax
    91b8:	48 8b 85 e8 fa ff ff 	mov    rax,QWORD PTR [rbp-0x518]
    91bf:	48 8b 8d f0 fa ff ff 	mov    rcx,QWORD PTR [rbp-0x510]
    91c6:	48 c1 e1 04          	shl    rcx,0x4
    91ca:	48 29 c8             	sub    rax,rcx
    91cd:	48 89 c1             	mov    rcx,rax
    91d0:	48 83 c1 23          	add    rcx,0x23
    91d4:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    91db:	8a 10                	mov    dl,BYTE PTR [rax]
    91dd:	8b b5 fc fa ff ff    	mov    esi,DWORD PTR [rbp-0x504]
    91e3:	89 f7                	mov    edi,esi
    91e5:	83 c7 01             	add    edi,0x1
    91e8:	89 bd fc fa ff ff    	mov    DWORD PTR [rbp-0x504],edi
    91ee:	48 63 c6             	movsxd rax,esi
    91f1:	88 94 05 00 fb ff ff 	mov    BYTE PTR [rbp+rax*1-0x500],dl
    91f8:	48 83 bd f0 fa ff ff 	cmp    QWORD PTR [rbp-0x510],0x0
    91ff:	00 
    9200:	0f 85 92 ff ff ff    	jne    0x9198
    9206:	8b 85 fc fa ff ff    	mov    eax,DWORD PTR [rbp-0x504]
    920c:	89 85 e4 fa ff ff    	mov    DWORD PTR [rbp-0x51c],eax
    9212:	8b 85 e4 fa ff ff    	mov    eax,DWORD PTR [rbp-0x51c]
    9218:	89 c1                	mov    ecx,eax
    921a:	83 c1 ff             	add    ecx,0xffffffff
    921d:	89 8d e4 fa ff ff    	mov    DWORD PTR [rbp-0x51c],ecx
    9223:	83 f8 00             	cmp    eax,0x0
    9226:	0f 84 15 00 00 00    	je     0x9241
    922c:	48 63 85 e4 fa ff ff 	movsxd rax,DWORD PTR [rbp-0x51c]
    9233:	8a 84 05 00 fb ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x500]
    923a:	e6 e9                	out    0xe9,al
    923c:	e9 d1 ff ff ff       	jmp    0x9212
    9241:	b0 0a                	mov    al,0xa
    9243:	e6 e9                	out    0xe9,al
    9245:	48 8d 8d e1 fa ff ff 	lea    rcx,[rbp-0x51f]
    924c:	c6 41 02 00          	mov    BYTE PTR [rcx+0x2],0x0
    9250:	66 c7 01 30 78       	mov    WORD PTR [rcx],0x7830
    9255:	c7 85 dc fa ff ff 00 	mov    DWORD PTR [rbp-0x524],0x0
    925c:	00 00 00 
    925f:	48 63 85 dc fa ff ff 	movsxd rax,DWORD PTR [rbp-0x524]
    9266:	80 bc 05 e1 fa ff ff 	cmp    BYTE PTR [rbp+rax*1-0x51f],0x0
    926d:	00 
    926e:	0f 84 22 00 00 00    	je     0x9296
    9274:	8b 85 dc fa ff ff    	mov    eax,DWORD PTR [rbp-0x524]
    927a:	89 c1                	mov    ecx,eax
    927c:	83 c1 01             	add    ecx,0x1
    927f:	89 8d dc fa ff ff    	mov    DWORD PTR [rbp-0x524],ecx
    9285:	48 63 d0             	movsxd rdx,eax
    9288:	8a 84 15 e1 fa ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x51f]
    928f:	e6 e9                	out    0xe9,al
    9291:	e9 c9 ff ff ff       	jmp    0x925f
    9296:	31 c0                	xor    eax,eax
    9298:	48 8d 8d 90 fa ff ff 	lea    rcx,[rbp-0x570]
    929f:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    92a6:	00 
    92a7:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    92ae:	00 
    92af:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    92b6:	00 
    92b7:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    92be:	00 
    92bf:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    92c6:	00 
    92c7:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    92ce:	00 
    92cf:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    92d6:	00 
    92d7:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    92de:	c7 85 8c fa ff ff 00 	mov    DWORD PTR [rbp-0x574],0x0
    92e5:	00 00 00 
    92e8:	48 8b 8d c8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x138]
    92ef:	48 89 8d 80 fa ff ff 	mov    QWORD PTR [rbp-0x580],rcx
    92f6:	48 8b 85 80 fa ff ff 	mov    rax,QWORD PTR [rbp-0x580]
    92fd:	48 89 85 78 fa ff ff 	mov    QWORD PTR [rbp-0x588],rax
    9304:	48 8b 85 80 fa ff ff 	mov    rax,QWORD PTR [rbp-0x580]
    930b:	48 c1 e8 04          	shr    rax,0x4
    930f:	48 89 85 80 fa ff ff 	mov    QWORD PTR [rbp-0x580],rax
    9316:	48 8b 85 78 fa ff ff 	mov    rax,QWORD PTR [rbp-0x588]
    931d:	48 8b 8d 80 fa ff ff 	mov    rcx,QWORD PTR [rbp-0x580]
    9324:	48 c1 e1 04          	shl    rcx,0x4
    9328:	48 29 c8             	sub    rax,rcx
    932b:	48 89 c1             	mov    rcx,rax
    932e:	48 83 c1 23          	add    rcx,0x23
    9332:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9339:	8a 10                	mov    dl,BYTE PTR [rax]
    933b:	8b b5 8c fa ff ff    	mov    esi,DWORD PTR [rbp-0x574]
    9341:	89 f7                	mov    edi,esi
    9343:	83 c7 01             	add    edi,0x1
    9346:	89 bd 8c fa ff ff    	mov    DWORD PTR [rbp-0x574],edi
    934c:	48 63 c6             	movsxd rax,esi
    934f:	88 94 05 90 fa ff ff 	mov    BYTE PTR [rbp+rax*1-0x570],dl
    9356:	48 83 bd 80 fa ff ff 	cmp    QWORD PTR [rbp-0x580],0x0
    935d:	00 
    935e:	0f 85 92 ff ff ff    	jne    0x92f6
    9364:	8b 85 8c fa ff ff    	mov    eax,DWORD PTR [rbp-0x574]
    936a:	89 85 74 fa ff ff    	mov    DWORD PTR [rbp-0x58c],eax
    9370:	8b 85 74 fa ff ff    	mov    eax,DWORD PTR [rbp-0x58c]
    9376:	89 c1                	mov    ecx,eax
    9378:	83 c1 ff             	add    ecx,0xffffffff
    937b:	89 8d 74 fa ff ff    	mov    DWORD PTR [rbp-0x58c],ecx
    9381:	83 f8 00             	cmp    eax,0x0
    9384:	0f 84 15 00 00 00    	je     0x939f
    938a:	48 63 85 74 fa ff ff 	movsxd rax,DWORD PTR [rbp-0x58c]
    9391:	8a 84 05 90 fa ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x570]
    9398:	e6 e9                	out    0xe9,al
    939a:	e9 d1 ff ff ff       	jmp    0x9370
    939f:	48 8d 85 6f fa ff ff 	lea    rax,[rbp-0x591]
    93a6:	c6 40 04 00          	mov    BYTE PTR [rax+0x4],0x0
    93aa:	c7 00 20 70 74 5b    	mov    DWORD PTR [rax],0x5b747020
    93b0:	c7 85 68 fa ff ff 00 	mov    DWORD PTR [rbp-0x598],0x0
    93b7:	00 00 00 
    93ba:	48 63 85 68 fa ff ff 	movsxd rax,DWORD PTR [rbp-0x598]
    93c1:	80 bc 05 6f fa ff ff 	cmp    BYTE PTR [rbp+rax*1-0x591],0x0
    93c8:	00 
    93c9:	0f 84 22 00 00 00    	je     0x93f1
    93cf:	8b 85 68 fa ff ff    	mov    eax,DWORD PTR [rbp-0x598]
    93d5:	89 c1                	mov    ecx,eax
    93d7:	83 c1 01             	add    ecx,0x1
    93da:	89 8d 68 fa ff ff    	mov    DWORD PTR [rbp-0x598],ecx
    93e0:	48 63 d0             	movsxd rdx,eax
    93e3:	8a 84 15 6f fa ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x591]
    93ea:	e6 e9                	out    0xe9,al
    93ec:	e9 c9 ff ff ff       	jmp    0x93ba
    93f1:	31 c0                	xor    eax,eax
    93f3:	48 8d 8d 20 fa ff ff 	lea    rcx,[rbp-0x5e0]
    93fa:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    9401:	00 
    9402:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    9409:	00 
    940a:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    9411:	00 
    9412:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    9419:	00 
    941a:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    9421:	00 
    9422:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    9429:	00 
    942a:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    9431:	00 
    9432:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    9439:	c7 85 1c fa ff ff 00 	mov    DWORD PTR [rbp-0x5e4],0x0
    9440:	00 00 00 
    9443:	48 8b 8d e0 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x120]
    944a:	48 89 8d 10 fa ff ff 	mov    QWORD PTR [rbp-0x5f0],rcx
    9451:	48 8b 85 10 fa ff ff 	mov    rax,QWORD PTR [rbp-0x5f0]
    9458:	48 89 85 08 fa ff ff 	mov    QWORD PTR [rbp-0x5f8],rax
    945f:	48 8b 85 10 fa ff ff 	mov    rax,QWORD PTR [rbp-0x5f0]
    9466:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    946d:	cc cc cc 
    9470:	48 f7 e1             	mul    rcx
    9473:	48 c1 ea 03          	shr    rdx,0x3
    9477:	48 89 95 10 fa ff ff 	mov    QWORD PTR [rbp-0x5f0],rdx
    947e:	48 8b 85 08 fa ff ff 	mov    rax,QWORD PTR [rbp-0x5f8]
    9485:	48 8b 8d 10 fa ff ff 	mov    rcx,QWORD PTR [rbp-0x5f0]
    948c:	48 01 c9             	add    rcx,rcx
    948f:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    9493:	48 29 c8             	sub    rax,rcx
    9496:	48 89 c1             	mov    rcx,rax
    9499:	48 83 c1 23          	add    rcx,0x23
    949d:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    94a4:	40 8a 30             	mov    sil,BYTE PTR [rax]
    94a7:	8b bd 1c fa ff ff    	mov    edi,DWORD PTR [rbp-0x5e4]
    94ad:	41 89 f8             	mov    r8d,edi
    94b0:	41 83 c0 01          	add    r8d,0x1
    94b4:	44 89 85 1c fa ff ff 	mov    DWORD PTR [rbp-0x5e4],r8d
    94bb:	48 63 c7             	movsxd rax,edi
    94be:	40 88 b4 05 20 fa ff 	mov    BYTE PTR [rbp+rax*1-0x5e0],sil
    94c5:	ff 
    94c6:	48 83 bd 10 fa ff ff 	cmp    QWORD PTR [rbp-0x5f0],0x0
    94cd:	00 
    94ce:	0f 85 7d ff ff ff    	jne    0x9451
    94d4:	8b 85 1c fa ff ff    	mov    eax,DWORD PTR [rbp-0x5e4]
    94da:	89 85 04 fa ff ff    	mov    DWORD PTR [rbp-0x5fc],eax
    94e0:	8b 85 04 fa ff ff    	mov    eax,DWORD PTR [rbp-0x5fc]
    94e6:	89 c1                	mov    ecx,eax
    94e8:	83 c1 ff             	add    ecx,0xffffffff
    94eb:	89 8d 04 fa ff ff    	mov    DWORD PTR [rbp-0x5fc],ecx
    94f1:	83 f8 00             	cmp    eax,0x0
    94f4:	0f 84 15 00 00 00    	je     0x950f
    94fa:	48 63 85 04 fa ff ff 	movsxd rax,DWORD PTR [rbp-0x5fc]
    9501:	8a 84 05 20 fa ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x5e0]
    9508:	e6 e9                	out    0xe9,al
    950a:	e9 d1 ff ff ff       	jmp    0x94e0
    950f:	48 8d 85 fd f9 ff ff 	lea    rax,[rbp-0x603]
    9516:	c7 40 03 20 30 78 00 	mov    DWORD PTR [rax+0x3],0x783020
    951d:	c7 00 5d 20 3d 20    	mov    DWORD PTR [rax],0x203d205d
    9523:	c7 85 f8 f9 ff ff 00 	mov    DWORD PTR [rbp-0x608],0x0
    952a:	00 00 00 
    952d:	48 63 85 f8 f9 ff ff 	movsxd rax,DWORD PTR [rbp-0x608]
    9534:	80 bc 05 fd f9 ff ff 	cmp    BYTE PTR [rbp+rax*1-0x603],0x0
    953b:	00 
    953c:	0f 84 22 00 00 00    	je     0x9564
    9542:	8b 85 f8 f9 ff ff    	mov    eax,DWORD PTR [rbp-0x608]
    9548:	89 c1                	mov    ecx,eax
    954a:	83 c1 01             	add    ecx,0x1
    954d:	89 8d f8 f9 ff ff    	mov    DWORD PTR [rbp-0x608],ecx
    9553:	48 63 d0             	movsxd rdx,eax
    9556:	8a 84 15 fd f9 ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x603]
    955d:	e6 e9                	out    0xe9,al
    955f:	e9 c9 ff ff ff       	jmp    0x952d
    9564:	31 c0                	xor    eax,eax
    9566:	48 8d 8d b0 f9 ff ff 	lea    rcx,[rbp-0x650]
    956d:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    9574:	00 
    9575:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    957c:	00 
    957d:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    9584:	00 
    9585:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    958c:	00 
    958d:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    9594:	00 
    9595:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    959c:	00 
    959d:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    95a4:	00 
    95a5:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    95ac:	c7 85 ac f9 ff ff 00 	mov    DWORD PTR [rbp-0x654],0x0
    95b3:	00 00 00 
    95b6:	48 8b 8d c8 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x138]
    95bd:	48 8b 95 e0 fe ff ff 	mov    rdx,QWORD PTR [rbp-0x120]
    95c4:	48 8b 0c d1          	mov    rcx,QWORD PTR [rcx+rdx*8]
    95c8:	48 89 8d a0 f9 ff ff 	mov    QWORD PTR [rbp-0x660],rcx
    95cf:	48 8b 85 a0 f9 ff ff 	mov    rax,QWORD PTR [rbp-0x660]
    95d6:	48 89 85 98 f9 ff ff 	mov    QWORD PTR [rbp-0x668],rax
    95dd:	48 8b 85 a0 f9 ff ff 	mov    rax,QWORD PTR [rbp-0x660]
    95e4:	48 c1 e8 04          	shr    rax,0x4
    95e8:	48 89 85 a0 f9 ff ff 	mov    QWORD PTR [rbp-0x660],rax
    95ef:	48 8b 85 98 f9 ff ff 	mov    rax,QWORD PTR [rbp-0x668]
    95f6:	48 8b 8d a0 f9 ff ff 	mov    rcx,QWORD PTR [rbp-0x660]
    95fd:	48 c1 e1 04          	shl    rcx,0x4
    9601:	48 29 c8             	sub    rax,rcx
    9604:	48 89 c1             	mov    rcx,rax
    9607:	48 83 c1 23          	add    rcx,0x23
    960b:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9612:	8a 10                	mov    dl,BYTE PTR [rax]
    9614:	8b b5 ac f9 ff ff    	mov    esi,DWORD PTR [rbp-0x654]
    961a:	89 f7                	mov    edi,esi
    961c:	83 c7 01             	add    edi,0x1
    961f:	89 bd ac f9 ff ff    	mov    DWORD PTR [rbp-0x654],edi
    9625:	48 63 c6             	movsxd rax,esi
    9628:	88 94 05 b0 f9 ff ff 	mov    BYTE PTR [rbp+rax*1-0x650],dl
    962f:	48 83 bd a0 f9 ff ff 	cmp    QWORD PTR [rbp-0x660],0x0
    9636:	00 
    9637:	0f 85 92 ff ff ff    	jne    0x95cf
    963d:	8b 85 ac f9 ff ff    	mov    eax,DWORD PTR [rbp-0x654]
    9643:	89 85 94 f9 ff ff    	mov    DWORD PTR [rbp-0x66c],eax
    9649:	8b 85 94 f9 ff ff    	mov    eax,DWORD PTR [rbp-0x66c]
    964f:	89 c1                	mov    ecx,eax
    9651:	83 c1 ff             	add    ecx,0xffffffff
    9654:	89 8d 94 f9 ff ff    	mov    DWORD PTR [rbp-0x66c],ecx
    965a:	83 f8 00             	cmp    eax,0x0
    965d:	0f 84 15 00 00 00    	je     0x9678
    9663:	48 63 85 94 f9 ff ff 	movsxd rax,DWORD PTR [rbp-0x66c]
    966a:	8a 84 05 b0 f9 ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x650]
    9671:	e6 e9                	out    0xe9,al
    9673:	e9 d1 ff ff ff       	jmp    0x9649
    9678:	b0 0a                	mov    al,0xa
    967a:	e6 e9                	out    0xe9,al
    967c:	48 81 c4 70 06 00 00 	add    rsp,0x670
    9683:	5d                   	pop    rbp
    9684:	c3                   	ret    
    9685:	66 2e 0f 1f 84 00 00 	nop    WORD PTR cs:[rax+rax*1+0x0]
    968c:	00 00 00 
    968f:	90                   	nop
    9690:	55                   	push   rbp
    9691:	48 89 e5             	mov    rbp,rsp
    9694:	48 81 ec 10 05 00 00 	sub    rsp,0x510
    969b:	b8 fd ff 07 00       	mov    eax,0x7fffd
    96a0:	66 8b 08             	mov    cx,WORD PTR [rax]
    96a3:	66 89 4d fe          	mov    WORD PTR [rbp-0x2],cx
    96a7:	0f b7 55 fe          	movzx  edx,WORD PTR [rbp-0x2]
    96ab:	89 d0                	mov    eax,edx
    96ad:	48 69 c0 18 00 00 00 	imul   rax,rax,0x18
    96b4:	48 05 02 00 00 00    	add    rax,0x2
    96ba:	be ff ff 07 00       	mov    esi,0x7ffff
    96bf:	48 29 c6             	sub    rsi,rax
    96c2:	48 89 75 f0          	mov    QWORD PTR [rbp-0x10],rsi
    96c6:	0f b7 55 fe          	movzx  edx,WORD PTR [rbp-0x2]
    96ca:	89 d0                	mov    eax,edx
    96cc:	48 89 45 e0          	mov    QWORD PTR [rbp-0x20],rax
    96d0:	48 8b 45 f0          	mov    rax,QWORD PTR [rbp-0x10]
    96d4:	48 89 45 e8          	mov    QWORD PTR [rbp-0x18],rax
    96d8:	48 8d 45 d6          	lea    rax,[rbp-0x2a]
    96dc:	48 be 62 6f 6f 74 20 	movabs rsi,0x30203d20746f6f62
    96e3:	3d 20 30 
    96e6:	48 89 30             	mov    QWORD PTR [rax],rsi
    96e9:	66 c7 40 08 78 00    	mov    WORD PTR [rax+0x8],0x78
    96ef:	c7 45 d0 00 00 00 00 	mov    DWORD PTR [rbp-0x30],0x0
    96f6:	48 63 45 d0          	movsxd rax,DWORD PTR [rbp-0x30]
    96fa:	80 7c 05 d6 00       	cmp    BYTE PTR [rbp+rax*1-0x2a],0x0
    96ff:	0f 84 19 00 00 00    	je     0x971e
    9705:	8b 45 d0             	mov    eax,DWORD PTR [rbp-0x30]
    9708:	89 c1                	mov    ecx,eax
    970a:	83 c1 01             	add    ecx,0x1
    970d:	89 4d d0             	mov    DWORD PTR [rbp-0x30],ecx
    9710:	48 63 d0             	movsxd rdx,eax
    9713:	8a 44 15 d6          	mov    al,BYTE PTR [rbp+rdx*1-0x2a]
    9717:	e6 e9                	out    0xe9,al
    9719:	e9 d8 ff ff ff       	jmp    0x96f6
    971e:	48 c7 45 c8 00 00 00 	mov    QWORD PTR [rbp-0x38],0x0
    9725:	00 
    9726:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
    972d:	00 
    972e:	48 c7 45 b8 00 00 00 	mov    QWORD PTR [rbp-0x48],0x0
    9735:	00 
    9736:	48 c7 45 b0 00 00 00 	mov    QWORD PTR [rbp-0x50],0x0
    973d:	00 
    973e:	48 c7 45 a8 00 00 00 	mov    QWORD PTR [rbp-0x58],0x0
    9745:	00 
    9746:	48 c7 45 a0 00 00 00 	mov    QWORD PTR [rbp-0x60],0x0
    974d:	00 
    974e:	48 c7 45 98 00 00 00 	mov    QWORD PTR [rbp-0x68],0x0
    9755:	00 
    9756:	48 c7 45 90 00 00 00 	mov    QWORD PTR [rbp-0x70],0x0
    975d:	00 
    975e:	c7 45 8c 00 00 00 00 	mov    DWORD PTR [rbp-0x74],0x0
    9765:	48 c7 45 80 90 96 00 	mov    QWORD PTR [rbp-0x80],0x9690
    976c:	00 
    976d:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    9771:	48 89 85 78 ff ff ff 	mov    QWORD PTR [rbp-0x88],rax
    9778:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
    977c:	48 c1 e8 04          	shr    rax,0x4
    9780:	48 89 45 80          	mov    QWORD PTR [rbp-0x80],rax
    9784:	48 8b 85 78 ff ff ff 	mov    rax,QWORD PTR [rbp-0x88]
    978b:	48 8b 4d 80          	mov    rcx,QWORD PTR [rbp-0x80]
    978f:	48 c1 e1 04          	shl    rcx,0x4
    9793:	48 29 c8             	sub    rax,rcx
    9796:	48 89 c1             	mov    rcx,rax
    9799:	48 83 c1 23          	add    rcx,0x23
    979d:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    97a4:	8a 10                	mov    dl,BYTE PTR [rax]
    97a6:	8b 75 8c             	mov    esi,DWORD PTR [rbp-0x74]
    97a9:	89 f7                	mov    edi,esi
    97ab:	83 c7 01             	add    edi,0x1
    97ae:	89 7d 8c             	mov    DWORD PTR [rbp-0x74],edi
    97b1:	48 63 c6             	movsxd rax,esi
    97b4:	88 54 05 90          	mov    BYTE PTR [rbp+rax*1-0x70],dl
    97b8:	48 83 7d 80 00       	cmp    QWORD PTR [rbp-0x80],0x0
    97bd:	0f 85 aa ff ff ff    	jne    0x976d
    97c3:	8b 45 8c             	mov    eax,DWORD PTR [rbp-0x74]
    97c6:	89 85 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],eax
    97cc:	8b 85 74 ff ff ff    	mov    eax,DWORD PTR [rbp-0x8c]
    97d2:	89 c1                	mov    ecx,eax
    97d4:	83 c1 ff             	add    ecx,0xffffffff
    97d7:	89 8d 74 ff ff ff    	mov    DWORD PTR [rbp-0x8c],ecx
    97dd:	83 f8 00             	cmp    eax,0x0
    97e0:	0f 84 12 00 00 00    	je     0x97f8
    97e6:	48 63 85 74 ff ff ff 	movsxd rax,DWORD PTR [rbp-0x8c]
    97ed:	8a 44 05 90          	mov    al,BYTE PTR [rbp+rax*1-0x70]
    97f1:	e6 e9                	out    0xe9,al
    97f3:	e9 d4 ff ff ff       	jmp    0x97cc
    97f8:	b0 0a                	mov    al,0xa
    97fa:	e6 e9                	out    0xe9,al
    97fc:	48 8d 8d 6a ff ff ff 	lea    rcx,[rbp-0x96]
    9803:	48 ba 70 6d 6c 34 20 	movabs rdx,0x30203d20346c6d70
    980a:	3d 20 30 
    980d:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9810:	66 c7 41 08 78 00    	mov    WORD PTR [rcx+0x8],0x78
    9816:	c7 85 64 ff ff ff 00 	mov    DWORD PTR [rbp-0x9c],0x0
    981d:	00 00 00 
    9820:	48 63 85 64 ff ff ff 	movsxd rax,DWORD PTR [rbp-0x9c]
    9827:	80 bc 05 6a ff ff ff 	cmp    BYTE PTR [rbp+rax*1-0x96],0x0
    982e:	00 
    982f:	0f 84 22 00 00 00    	je     0x9857
    9835:	8b 85 64 ff ff ff    	mov    eax,DWORD PTR [rbp-0x9c]
    983b:	89 c1                	mov    ecx,eax
    983d:	83 c1 01             	add    ecx,0x1
    9840:	89 8d 64 ff ff ff    	mov    DWORD PTR [rbp-0x9c],ecx
    9846:	48 63 d0             	movsxd rdx,eax
    9849:	8a 84 15 6a ff ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x96]
    9850:	e6 e9                	out    0xe9,al
    9852:	e9 c9 ff ff ff       	jmp    0x9820
    9857:	48 c7 85 58 ff ff ff 	mov    QWORD PTR [rbp-0xa8],0x0
    985e:	00 00 00 00 
    9862:	48 c7 85 50 ff ff ff 	mov    QWORD PTR [rbp-0xb0],0x0
    9869:	00 00 00 00 
    986d:	48 c7 85 48 ff ff ff 	mov    QWORD PTR [rbp-0xb8],0x0
    9874:	00 00 00 00 
    9878:	48 c7 85 40 ff ff ff 	mov    QWORD PTR [rbp-0xc0],0x0
    987f:	00 00 00 00 
    9883:	48 c7 85 38 ff ff ff 	mov    QWORD PTR [rbp-0xc8],0x0
    988a:	00 00 00 00 
    988e:	48 c7 85 30 ff ff ff 	mov    QWORD PTR [rbp-0xd0],0x0
    9895:	00 00 00 00 
    9899:	48 c7 85 28 ff ff ff 	mov    QWORD PTR [rbp-0xd8],0x0
    98a0:	00 00 00 00 
    98a4:	48 c7 85 20 ff ff ff 	mov    QWORD PTR [rbp-0xe0],0x0
    98ab:	00 00 00 00 
    98af:	c7 85 1c ff ff ff 00 	mov    DWORD PTR [rbp-0xe4],0x0
    98b6:	00 00 00 
    98b9:	48 c7 85 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],0xb000
    98c0:	00 b0 00 00 
    98c4:	48 8b 85 10 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf0]
    98cb:	48 89 85 08 ff ff ff 	mov    QWORD PTR [rbp-0xf8],rax
    98d2:	48 8b 85 10 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf0]
    98d9:	48 c1 e8 04          	shr    rax,0x4
    98dd:	48 89 85 10 ff ff ff 	mov    QWORD PTR [rbp-0xf0],rax
    98e4:	48 8b 85 08 ff ff ff 	mov    rax,QWORD PTR [rbp-0xf8]
    98eb:	48 8b 8d 10 ff ff ff 	mov    rcx,QWORD PTR [rbp-0xf0]
    98f2:	48 c1 e1 04          	shl    rcx,0x4
    98f6:	48 29 c8             	sub    rax,rcx
    98f9:	48 89 c1             	mov    rcx,rax
    98fc:	48 83 c1 23          	add    rcx,0x23
    9900:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9907:	8a 10                	mov    dl,BYTE PTR [rax]
    9909:	8b b5 1c ff ff ff    	mov    esi,DWORD PTR [rbp-0xe4]
    990f:	89 f7                	mov    edi,esi
    9911:	83 c7 01             	add    edi,0x1
    9914:	89 bd 1c ff ff ff    	mov    DWORD PTR [rbp-0xe4],edi
    991a:	48 63 c6             	movsxd rax,esi
    991d:	88 94 05 20 ff ff ff 	mov    BYTE PTR [rbp+rax*1-0xe0],dl
    9924:	48 83 bd 10 ff ff ff 	cmp    QWORD PTR [rbp-0xf0],0x0
    992b:	00 
    992c:	0f 85 92 ff ff ff    	jne    0x98c4
    9932:	8b 85 1c ff ff ff    	mov    eax,DWORD PTR [rbp-0xe4]
    9938:	89 85 04 ff ff ff    	mov    DWORD PTR [rbp-0xfc],eax
    993e:	8b 85 04 ff ff ff    	mov    eax,DWORD PTR [rbp-0xfc]
    9944:	89 c1                	mov    ecx,eax
    9946:	83 c1 ff             	add    ecx,0xffffffff
    9949:	89 8d 04 ff ff ff    	mov    DWORD PTR [rbp-0xfc],ecx
    994f:	83 f8 00             	cmp    eax,0x0
    9952:	0f 84 15 00 00 00    	je     0x996d
    9958:	48 63 85 04 ff ff ff 	movsxd rax,DWORD PTR [rbp-0xfc]
    995f:	8a 84 05 20 ff ff ff 	mov    al,BYTE PTR [rbp+rax*1-0xe0]
    9966:	e6 e9                	out    0xe9,al
    9968:	e9 d1 ff ff ff       	jmp    0x993e
    996d:	b0 0a                	mov    al,0xa
    996f:	e6 e9                	out    0xe9,al
    9971:	48 8d 8d f0 fe ff ff 	lea    rcx,[rbp-0x110]
    9978:	48 ba 41 44 44 52 20 	movabs rdx,0x30203d2052444441
    997f:	3d 20 30 
    9982:	48 89 51 08          	mov    QWORD PTR [rcx+0x8],rdx
    9986:	48 ba 4b 45 52 4e 45 	movabs rdx,0x565f4c454e52454b
    998d:	4c 5f 56 
    9990:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9993:	66 c7 41 10 78 00    	mov    WORD PTR [rcx+0x10],0x78
    9999:	c7 85 ec fe ff ff 00 	mov    DWORD PTR [rbp-0x114],0x0
    99a0:	00 00 00 
    99a3:	48 63 85 ec fe ff ff 	movsxd rax,DWORD PTR [rbp-0x114]
    99aa:	80 bc 05 f0 fe ff ff 	cmp    BYTE PTR [rbp+rax*1-0x110],0x0
    99b1:	00 
    99b2:	0f 84 22 00 00 00    	je     0x99da
    99b8:	8b 85 ec fe ff ff    	mov    eax,DWORD PTR [rbp-0x114]
    99be:	89 c1                	mov    ecx,eax
    99c0:	83 c1 01             	add    ecx,0x1
    99c3:	89 8d ec fe ff ff    	mov    DWORD PTR [rbp-0x114],ecx
    99c9:	48 63 d0             	movsxd rdx,eax
    99cc:	8a 84 15 f0 fe ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x110]
    99d3:	e6 e9                	out    0xe9,al
    99d5:	e9 c9 ff ff ff       	jmp    0x99a3
    99da:	48 c7 85 d8 fe ff ff 	mov    QWORD PTR [rbp-0x128],0x0
    99e1:	00 00 00 00 
    99e5:	48 c7 85 d0 fe ff ff 	mov    QWORD PTR [rbp-0x130],0x0
    99ec:	00 00 00 00 
    99f0:	48 c7 85 c8 fe ff ff 	mov    QWORD PTR [rbp-0x138],0x0
    99f7:	00 00 00 00 
    99fb:	48 c7 85 c0 fe ff ff 	mov    QWORD PTR [rbp-0x140],0x0
    9a02:	00 00 00 00 
    9a06:	48 c7 85 b8 fe ff ff 	mov    QWORD PTR [rbp-0x148],0x0
    9a0d:	00 00 00 00 
    9a11:	48 c7 85 b0 fe ff ff 	mov    QWORD PTR [rbp-0x150],0x0
    9a18:	00 00 00 00 
    9a1c:	48 c7 85 a8 fe ff ff 	mov    QWORD PTR [rbp-0x158],0x0
    9a23:	00 00 00 00 
    9a27:	48 c7 85 a0 fe ff ff 	mov    QWORD PTR [rbp-0x160],0x0
    9a2e:	00 00 00 00 
    9a32:	c7 85 9c fe ff ff 00 	mov    DWORD PTR [rbp-0x164],0x0
    9a39:	00 00 00 
    9a3c:	48 c7 85 90 fe ff ff 	mov    QWORD PTR [rbp-0x170],0xffffffff8000b000
    9a43:	00 b0 00 80 
    9a47:	48 8b 85 90 fe ff ff 	mov    rax,QWORD PTR [rbp-0x170]
    9a4e:	48 89 85 88 fe ff ff 	mov    QWORD PTR [rbp-0x178],rax
    9a55:	48 8b 85 90 fe ff ff 	mov    rax,QWORD PTR [rbp-0x170]
    9a5c:	48 c1 e8 04          	shr    rax,0x4
    9a60:	48 89 85 90 fe ff ff 	mov    QWORD PTR [rbp-0x170],rax
    9a67:	48 8b 85 88 fe ff ff 	mov    rax,QWORD PTR [rbp-0x178]
    9a6e:	48 8b 8d 90 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x170]
    9a75:	48 c1 e1 04          	shl    rcx,0x4
    9a79:	48 29 c8             	sub    rax,rcx
    9a7c:	48 89 c1             	mov    rcx,rax
    9a7f:	48 83 c1 23          	add    rcx,0x23
    9a83:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9a8a:	8a 10                	mov    dl,BYTE PTR [rax]
    9a8c:	8b b5 9c fe ff ff    	mov    esi,DWORD PTR [rbp-0x164]
    9a92:	89 f7                	mov    edi,esi
    9a94:	83 c7 01             	add    edi,0x1
    9a97:	89 bd 9c fe ff ff    	mov    DWORD PTR [rbp-0x164],edi
    9a9d:	48 63 c6             	movsxd rax,esi
    9aa0:	88 94 05 a0 fe ff ff 	mov    BYTE PTR [rbp+rax*1-0x160],dl
    9aa7:	48 83 bd 90 fe ff ff 	cmp    QWORD PTR [rbp-0x170],0x0
    9aae:	00 
    9aaf:	0f 85 92 ff ff ff    	jne    0x9a47
    9ab5:	8b 85 9c fe ff ff    	mov    eax,DWORD PTR [rbp-0x164]
    9abb:	89 85 84 fe ff ff    	mov    DWORD PTR [rbp-0x17c],eax
    9ac1:	8b 85 84 fe ff ff    	mov    eax,DWORD PTR [rbp-0x17c]
    9ac7:	89 c1                	mov    ecx,eax
    9ac9:	83 c1 ff             	add    ecx,0xffffffff
    9acc:	89 8d 84 fe ff ff    	mov    DWORD PTR [rbp-0x17c],ecx
    9ad2:	83 f8 00             	cmp    eax,0x0
    9ad5:	0f 84 15 00 00 00    	je     0x9af0
    9adb:	48 63 85 84 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x17c]
    9ae2:	8a 84 05 a0 fe ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x160]
    9ae9:	e6 e9                	out    0xe9,al
    9aeb:	e9 d1 ff ff ff       	jmp    0x9ac1
    9af0:	b0 0a                	mov    al,0xa
    9af2:	e6 e9                	out    0xe9,al
    9af4:	48 8d 8d 79 fe ff ff 	lea    rcx,[rbp-0x187]
    9afb:	48 ba 2e 74 65 78 74 	movabs rdx,0x203d20747865742e
    9b02:	20 3d 20 
    9b05:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9b08:	c7 41 07 20 30 78 00 	mov    DWORD PTR [rcx+0x7],0x783020
    9b0f:	c7 85 74 fe ff ff 00 	mov    DWORD PTR [rbp-0x18c],0x0
    9b16:	00 00 00 
    9b19:	48 63 85 74 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x18c]
    9b20:	80 bc 05 79 fe ff ff 	cmp    BYTE PTR [rbp+rax*1-0x187],0x0
    9b27:	00 
    9b28:	0f 84 22 00 00 00    	je     0x9b50
    9b2e:	8b 85 74 fe ff ff    	mov    eax,DWORD PTR [rbp-0x18c]
    9b34:	89 c1                	mov    ecx,eax
    9b36:	83 c1 01             	add    ecx,0x1
    9b39:	89 8d 74 fe ff ff    	mov    DWORD PTR [rbp-0x18c],ecx
    9b3f:	48 63 d0             	movsxd rdx,eax
    9b42:	8a 84 15 79 fe ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x187]
    9b49:	e6 e9                	out    0xe9,al
    9b4b:	e9 c9 ff ff ff       	jmp    0x9b19
    9b50:	48 c7 85 68 fe ff ff 	mov    QWORD PTR [rbp-0x198],0x0
    9b57:	00 00 00 00 
    9b5b:	48 c7 85 60 fe ff ff 	mov    QWORD PTR [rbp-0x1a0],0x0
    9b62:	00 00 00 00 
    9b66:	48 c7 85 58 fe ff ff 	mov    QWORD PTR [rbp-0x1a8],0x0
    9b6d:	00 00 00 00 
    9b71:	48 c7 85 50 fe ff ff 	mov    QWORD PTR [rbp-0x1b0],0x0
    9b78:	00 00 00 00 
    9b7c:	48 c7 85 48 fe ff ff 	mov    QWORD PTR [rbp-0x1b8],0x0
    9b83:	00 00 00 00 
    9b87:	48 c7 85 40 fe ff ff 	mov    QWORD PTR [rbp-0x1c0],0x0
    9b8e:	00 00 00 00 
    9b92:	48 c7 85 38 fe ff ff 	mov    QWORD PTR [rbp-0x1c8],0x0
    9b99:	00 00 00 00 
    9b9d:	48 c7 85 30 fe ff ff 	mov    QWORD PTR [rbp-0x1d0],0x0
    9ba4:	00 00 00 00 
    9ba8:	c7 85 2c fe ff ff 00 	mov    DWORD PTR [rbp-0x1d4],0x0
    9baf:	00 00 00 
    9bb2:	48 c7 85 20 fe ff ff 	mov    QWORD PTR [rbp-0x1e0],0xffffffff8000b000
    9bb9:	00 b0 00 80 
    9bbd:	48 8b 85 20 fe ff ff 	mov    rax,QWORD PTR [rbp-0x1e0]
    9bc4:	48 89 85 18 fe ff ff 	mov    QWORD PTR [rbp-0x1e8],rax
    9bcb:	48 8b 85 20 fe ff ff 	mov    rax,QWORD PTR [rbp-0x1e0]
    9bd2:	48 c1 e8 04          	shr    rax,0x4
    9bd6:	48 89 85 20 fe ff ff 	mov    QWORD PTR [rbp-0x1e0],rax
    9bdd:	48 8b 85 18 fe ff ff 	mov    rax,QWORD PTR [rbp-0x1e8]
    9be4:	48 8b 8d 20 fe ff ff 	mov    rcx,QWORD PTR [rbp-0x1e0]
    9beb:	48 c1 e1 04          	shl    rcx,0x4
    9bef:	48 29 c8             	sub    rax,rcx
    9bf2:	48 89 c1             	mov    rcx,rax
    9bf5:	48 83 c1 23          	add    rcx,0x23
    9bf9:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9c00:	8a 10                	mov    dl,BYTE PTR [rax]
    9c02:	8b b5 2c fe ff ff    	mov    esi,DWORD PTR [rbp-0x1d4]
    9c08:	89 f7                	mov    edi,esi
    9c0a:	83 c7 01             	add    edi,0x1
    9c0d:	89 bd 2c fe ff ff    	mov    DWORD PTR [rbp-0x1d4],edi
    9c13:	48 63 c6             	movsxd rax,esi
    9c16:	88 94 05 30 fe ff ff 	mov    BYTE PTR [rbp+rax*1-0x1d0],dl
    9c1d:	48 83 bd 20 fe ff ff 	cmp    QWORD PTR [rbp-0x1e0],0x0
    9c24:	00 
    9c25:	0f 85 92 ff ff ff    	jne    0x9bbd
    9c2b:	8b 85 2c fe ff ff    	mov    eax,DWORD PTR [rbp-0x1d4]
    9c31:	89 85 14 fe ff ff    	mov    DWORD PTR [rbp-0x1ec],eax
    9c37:	8b 85 14 fe ff ff    	mov    eax,DWORD PTR [rbp-0x1ec]
    9c3d:	89 c1                	mov    ecx,eax
    9c3f:	83 c1 ff             	add    ecx,0xffffffff
    9c42:	89 8d 14 fe ff ff    	mov    DWORD PTR [rbp-0x1ec],ecx
    9c48:	83 f8 00             	cmp    eax,0x0
    9c4b:	0f 84 15 00 00 00    	je     0x9c66
    9c51:	48 63 85 14 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x1ec]
    9c58:	8a 84 05 30 fe ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x1d0]
    9c5f:	e6 e9                	out    0xe9,al
    9c61:	e9 d1 ff ff ff       	jmp    0x9c37
    9c66:	b0 0a                	mov    al,0xa
    9c68:	e6 e9                	out    0xe9,al
    9c6a:	48 8d 8d 09 fe ff ff 	lea    rcx,[rbp-0x1f7]
    9c71:	48 ba 2e 64 61 74 61 	movabs rdx,0x203d20617461642e
    9c78:	20 3d 20 
    9c7b:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9c7e:	c7 41 07 20 30 78 00 	mov    DWORD PTR [rcx+0x7],0x783020
    9c85:	c7 85 04 fe ff ff 00 	mov    DWORD PTR [rbp-0x1fc],0x0
    9c8c:	00 00 00 
    9c8f:	48 63 85 04 fe ff ff 	movsxd rax,DWORD PTR [rbp-0x1fc]
    9c96:	80 bc 05 09 fe ff ff 	cmp    BYTE PTR [rbp+rax*1-0x1f7],0x0
    9c9d:	00 
    9c9e:	0f 84 22 00 00 00    	je     0x9cc6
    9ca4:	8b 85 04 fe ff ff    	mov    eax,DWORD PTR [rbp-0x1fc]
    9caa:	89 c1                	mov    ecx,eax
    9cac:	83 c1 01             	add    ecx,0x1
    9caf:	89 8d 04 fe ff ff    	mov    DWORD PTR [rbp-0x1fc],ecx
    9cb5:	48 63 d0             	movsxd rdx,eax
    9cb8:	8a 84 15 09 fe ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x1f7]
    9cbf:	e6 e9                	out    0xe9,al
    9cc1:	e9 c9 ff ff ff       	jmp    0x9c8f
    9cc6:	48 c7 85 f8 fd ff ff 	mov    QWORD PTR [rbp-0x208],0x0
    9ccd:	00 00 00 00 
    9cd1:	48 c7 85 f0 fd ff ff 	mov    QWORD PTR [rbp-0x210],0x0
    9cd8:	00 00 00 00 
    9cdc:	48 c7 85 e8 fd ff ff 	mov    QWORD PTR [rbp-0x218],0x0
    9ce3:	00 00 00 00 
    9ce7:	48 c7 85 e0 fd ff ff 	mov    QWORD PTR [rbp-0x220],0x0
    9cee:	00 00 00 00 
    9cf2:	48 c7 85 d8 fd ff ff 	mov    QWORD PTR [rbp-0x228],0x0
    9cf9:	00 00 00 00 
    9cfd:	48 c7 85 d0 fd ff ff 	mov    QWORD PTR [rbp-0x230],0x0
    9d04:	00 00 00 00 
    9d08:	48 c7 85 c8 fd ff ff 	mov    QWORD PTR [rbp-0x238],0x0
    9d0f:	00 00 00 00 
    9d13:	48 c7 85 c0 fd ff ff 	mov    QWORD PTR [rbp-0x240],0x0
    9d1a:	00 00 00 00 
    9d1e:	c7 85 bc fd ff ff 00 	mov    DWORD PTR [rbp-0x244],0x0
    9d25:	00 00 00 
    9d28:	48 c7 85 b0 fd ff ff 	mov    QWORD PTR [rbp-0x250],0xffffffff8000c000
    9d2f:	00 c0 00 80 
    9d33:	48 8b 85 b0 fd ff ff 	mov    rax,QWORD PTR [rbp-0x250]
    9d3a:	48 89 85 a8 fd ff ff 	mov    QWORD PTR [rbp-0x258],rax
    9d41:	48 8b 85 b0 fd ff ff 	mov    rax,QWORD PTR [rbp-0x250]
    9d48:	48 c1 e8 04          	shr    rax,0x4
    9d4c:	48 89 85 b0 fd ff ff 	mov    QWORD PTR [rbp-0x250],rax
    9d53:	48 8b 85 a8 fd ff ff 	mov    rax,QWORD PTR [rbp-0x258]
    9d5a:	48 8b 8d b0 fd ff ff 	mov    rcx,QWORD PTR [rbp-0x250]
    9d61:	48 c1 e1 04          	shl    rcx,0x4
    9d65:	48 29 c8             	sub    rax,rcx
    9d68:	48 89 c1             	mov    rcx,rax
    9d6b:	48 83 c1 23          	add    rcx,0x23
    9d6f:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9d76:	8a 10                	mov    dl,BYTE PTR [rax]
    9d78:	8b b5 bc fd ff ff    	mov    esi,DWORD PTR [rbp-0x244]
    9d7e:	89 f7                	mov    edi,esi
    9d80:	83 c7 01             	add    edi,0x1
    9d83:	89 bd bc fd ff ff    	mov    DWORD PTR [rbp-0x244],edi
    9d89:	48 63 c6             	movsxd rax,esi
    9d8c:	88 94 05 c0 fd ff ff 	mov    BYTE PTR [rbp+rax*1-0x240],dl
    9d93:	48 83 bd b0 fd ff ff 	cmp    QWORD PTR [rbp-0x250],0x0
    9d9a:	00 
    9d9b:	0f 85 92 ff ff ff    	jne    0x9d33
    9da1:	8b 85 bc fd ff ff    	mov    eax,DWORD PTR [rbp-0x244]
    9da7:	89 85 a4 fd ff ff    	mov    DWORD PTR [rbp-0x25c],eax
    9dad:	8b 85 a4 fd ff ff    	mov    eax,DWORD PTR [rbp-0x25c]
    9db3:	89 c1                	mov    ecx,eax
    9db5:	83 c1 ff             	add    ecx,0xffffffff
    9db8:	89 8d a4 fd ff ff    	mov    DWORD PTR [rbp-0x25c],ecx
    9dbe:	83 f8 00             	cmp    eax,0x0
    9dc1:	0f 84 15 00 00 00    	je     0x9ddc
    9dc7:	48 63 85 a4 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x25c]
    9dce:	8a 84 05 c0 fd ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x240]
    9dd5:	e6 e9                	out    0xe9,al
    9dd7:	e9 d1 ff ff ff       	jmp    0x9dad
    9ddc:	b0 0a                	mov    al,0xa
    9dde:	e6 e9                	out    0xe9,al
    9de0:	48 8d 8d 97 fd ff ff 	lea    rcx,[rbp-0x269]
    9de7:	48 ba 74 61 20 3d 20 	movabs rdx,0x7830203d206174
    9dee:	30 78 00 
    9df1:	48 89 51 05          	mov    QWORD PTR [rcx+0x5],rdx
    9df5:	48 ba 2e 72 6f 64 61 	movabs rdx,0x20617461646f722e
    9dfc:	74 61 20 
    9dff:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9e02:	c7 85 90 fd ff ff 00 	mov    DWORD PTR [rbp-0x270],0x0
    9e09:	00 00 00 
    9e0c:	48 63 85 90 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x270]
    9e13:	80 bc 05 97 fd ff ff 	cmp    BYTE PTR [rbp+rax*1-0x269],0x0
    9e1a:	00 
    9e1b:	0f 84 22 00 00 00    	je     0x9e43
    9e21:	8b 85 90 fd ff ff    	mov    eax,DWORD PTR [rbp-0x270]
    9e27:	89 c1                	mov    ecx,eax
    9e29:	83 c1 01             	add    ecx,0x1
    9e2c:	89 8d 90 fd ff ff    	mov    DWORD PTR [rbp-0x270],ecx
    9e32:	48 63 d0             	movsxd rdx,eax
    9e35:	8a 84 15 97 fd ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x269]
    9e3c:	e6 e9                	out    0xe9,al
    9e3e:	e9 c9 ff ff ff       	jmp    0x9e0c
    9e43:	48 c7 85 88 fd ff ff 	mov    QWORD PTR [rbp-0x278],0x0
    9e4a:	00 00 00 00 
    9e4e:	48 c7 85 80 fd ff ff 	mov    QWORD PTR [rbp-0x280],0x0
    9e55:	00 00 00 00 
    9e59:	48 c7 85 78 fd ff ff 	mov    QWORD PTR [rbp-0x288],0x0
    9e60:	00 00 00 00 
    9e64:	48 c7 85 70 fd ff ff 	mov    QWORD PTR [rbp-0x290],0x0
    9e6b:	00 00 00 00 
    9e6f:	48 c7 85 68 fd ff ff 	mov    QWORD PTR [rbp-0x298],0x0
    9e76:	00 00 00 00 
    9e7a:	48 c7 85 60 fd ff ff 	mov    QWORD PTR [rbp-0x2a0],0x0
    9e81:	00 00 00 00 
    9e85:	48 c7 85 58 fd ff ff 	mov    QWORD PTR [rbp-0x2a8],0x0
    9e8c:	00 00 00 00 
    9e90:	48 c7 85 50 fd ff ff 	mov    QWORD PTR [rbp-0x2b0],0x0
    9e97:	00 00 00 00 
    9e9b:	c7 85 4c fd ff ff 00 	mov    DWORD PTR [rbp-0x2b4],0x0
    9ea2:	00 00 00 
    9ea5:	48 c7 85 40 fd ff ff 	mov    QWORD PTR [rbp-0x2c0],0xffffffff8000d000
    9eac:	00 d0 00 80 
    9eb0:	48 8b 85 40 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2c0]
    9eb7:	48 89 85 38 fd ff ff 	mov    QWORD PTR [rbp-0x2c8],rax
    9ebe:	48 8b 85 40 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2c0]
    9ec5:	48 c1 e8 04          	shr    rax,0x4
    9ec9:	48 89 85 40 fd ff ff 	mov    QWORD PTR [rbp-0x2c0],rax
    9ed0:	48 8b 85 38 fd ff ff 	mov    rax,QWORD PTR [rbp-0x2c8]
    9ed7:	48 8b 8d 40 fd ff ff 	mov    rcx,QWORD PTR [rbp-0x2c0]
    9ede:	48 c1 e1 04          	shl    rcx,0x4
    9ee2:	48 29 c8             	sub    rax,rcx
    9ee5:	48 89 c1             	mov    rcx,rax
    9ee8:	48 83 c1 23          	add    rcx,0x23
    9eec:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    9ef3:	8a 10                	mov    dl,BYTE PTR [rax]
    9ef5:	8b b5 4c fd ff ff    	mov    esi,DWORD PTR [rbp-0x2b4]
    9efb:	89 f7                	mov    edi,esi
    9efd:	83 c7 01             	add    edi,0x1
    9f00:	89 bd 4c fd ff ff    	mov    DWORD PTR [rbp-0x2b4],edi
    9f06:	48 63 c6             	movsxd rax,esi
    9f09:	88 94 05 50 fd ff ff 	mov    BYTE PTR [rbp+rax*1-0x2b0],dl
    9f10:	48 83 bd 40 fd ff ff 	cmp    QWORD PTR [rbp-0x2c0],0x0
    9f17:	00 
    9f18:	0f 85 92 ff ff ff    	jne    0x9eb0
    9f1e:	8b 85 4c fd ff ff    	mov    eax,DWORD PTR [rbp-0x2b4]
    9f24:	89 85 34 fd ff ff    	mov    DWORD PTR [rbp-0x2cc],eax
    9f2a:	8b 85 34 fd ff ff    	mov    eax,DWORD PTR [rbp-0x2cc]
    9f30:	89 c1                	mov    ecx,eax
    9f32:	83 c1 ff             	add    ecx,0xffffffff
    9f35:	89 8d 34 fd ff ff    	mov    DWORD PTR [rbp-0x2cc],ecx
    9f3b:	83 f8 00             	cmp    eax,0x0
    9f3e:	0f 84 15 00 00 00    	je     0x9f59
    9f44:	48 63 85 34 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x2cc]
    9f4b:	8a 84 05 50 fd ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x2b0]
    9f52:	e6 e9                	out    0xe9,al
    9f54:	e9 d1 ff ff ff       	jmp    0x9f2a
    9f59:	b0 0a                	mov    al,0xa
    9f5b:	e6 e9                	out    0xe9,al
    9f5d:	48 8d 8d 2a fd ff ff 	lea    rcx,[rbp-0x2d6]
    9f64:	48 ba 2e 62 73 73 20 	movabs rdx,0x30203d207373622e
    9f6b:	3d 20 30 
    9f6e:	48 89 11             	mov    QWORD PTR [rcx],rdx
    9f71:	66 c7 41 08 78 00    	mov    WORD PTR [rcx+0x8],0x78
    9f77:	c7 85 24 fd ff ff 00 	mov    DWORD PTR [rbp-0x2dc],0x0
    9f7e:	00 00 00 
    9f81:	48 63 85 24 fd ff ff 	movsxd rax,DWORD PTR [rbp-0x2dc]
    9f88:	80 bc 05 2a fd ff ff 	cmp    BYTE PTR [rbp+rax*1-0x2d6],0x0
    9f8f:	00 
    9f90:	0f 84 22 00 00 00    	je     0x9fb8
    9f96:	8b 85 24 fd ff ff    	mov    eax,DWORD PTR [rbp-0x2dc]
    9f9c:	89 c1                	mov    ecx,eax
    9f9e:	83 c1 01             	add    ecx,0x1
    9fa1:	89 8d 24 fd ff ff    	mov    DWORD PTR [rbp-0x2dc],ecx
    9fa7:	48 63 d0             	movsxd rdx,eax
    9faa:	8a 84 15 2a fd ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x2d6]
    9fb1:	e6 e9                	out    0xe9,al
    9fb3:	e9 c9 ff ff ff       	jmp    0x9f81
    9fb8:	48 c7 85 18 fd ff ff 	mov    QWORD PTR [rbp-0x2e8],0x0
    9fbf:	00 00 00 00 
    9fc3:	48 c7 85 10 fd ff ff 	mov    QWORD PTR [rbp-0x2f0],0x0
    9fca:	00 00 00 00 
    9fce:	48 c7 85 08 fd ff ff 	mov    QWORD PTR [rbp-0x2f8],0x0
    9fd5:	00 00 00 00 
    9fd9:	48 c7 85 00 fd ff ff 	mov    QWORD PTR [rbp-0x300],0x0
    9fe0:	00 00 00 00 
    9fe4:	48 c7 85 f8 fc ff ff 	mov    QWORD PTR [rbp-0x308],0x0
    9feb:	00 00 00 00 
    9fef:	48 c7 85 f0 fc ff ff 	mov    QWORD PTR [rbp-0x310],0x0
    9ff6:	00 00 00 00 
    9ffa:	48 c7 85 e8 fc ff ff 	mov    QWORD PTR [rbp-0x318],0x0
    a001:	00 00 00 00 
    a005:	48 c7 85 e0 fc ff ff 	mov    QWORD PTR [rbp-0x320],0x0
    a00c:	00 00 00 00 
    a010:	c7 85 dc fc ff ff 00 	mov    DWORD PTR [rbp-0x324],0x0
    a017:	00 00 00 
    a01a:	48 c7 85 d0 fc ff ff 	mov    QWORD PTR [rbp-0x330],0xffffffff8000e000
    a021:	00 e0 00 80 
    a025:	48 8b 85 d0 fc ff ff 	mov    rax,QWORD PTR [rbp-0x330]
    a02c:	48 89 85 c8 fc ff ff 	mov    QWORD PTR [rbp-0x338],rax
    a033:	48 8b 85 d0 fc ff ff 	mov    rax,QWORD PTR [rbp-0x330]
    a03a:	48 c1 e8 04          	shr    rax,0x4
    a03e:	48 89 85 d0 fc ff ff 	mov    QWORD PTR [rbp-0x330],rax
    a045:	48 8b 85 c8 fc ff ff 	mov    rax,QWORD PTR [rbp-0x338]
    a04c:	48 8b 8d d0 fc ff ff 	mov    rcx,QWORD PTR [rbp-0x330]
    a053:	48 c1 e1 04          	shl    rcx,0x4
    a057:	48 29 c8             	sub    rax,rcx
    a05a:	48 89 c1             	mov    rcx,rax
    a05d:	48 83 c1 23          	add    rcx,0x23
    a061:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    a068:	8a 10                	mov    dl,BYTE PTR [rax]
    a06a:	8b b5 dc fc ff ff    	mov    esi,DWORD PTR [rbp-0x324]
    a070:	89 f7                	mov    edi,esi
    a072:	83 c7 01             	add    edi,0x1
    a075:	89 bd dc fc ff ff    	mov    DWORD PTR [rbp-0x324],edi
    a07b:	48 63 c6             	movsxd rax,esi
    a07e:	88 94 05 e0 fc ff ff 	mov    BYTE PTR [rbp+rax*1-0x320],dl
    a085:	48 83 bd d0 fc ff ff 	cmp    QWORD PTR [rbp-0x330],0x0
    a08c:	00 
    a08d:	0f 85 92 ff ff ff    	jne    0xa025
    a093:	8b 85 dc fc ff ff    	mov    eax,DWORD PTR [rbp-0x324]
    a099:	89 85 c4 fc ff ff    	mov    DWORD PTR [rbp-0x33c],eax
    a09f:	8b 85 c4 fc ff ff    	mov    eax,DWORD PTR [rbp-0x33c]
    a0a5:	89 c1                	mov    ecx,eax
    a0a7:	83 c1 ff             	add    ecx,0xffffffff
    a0aa:	89 8d c4 fc ff ff    	mov    DWORD PTR [rbp-0x33c],ecx
    a0b0:	83 f8 00             	cmp    eax,0x0
    a0b3:	0f 84 15 00 00 00    	je     0xa0ce
    a0b9:	48 63 85 c4 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x33c]
    a0c0:	8a 84 05 e0 fc ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x320]
    a0c7:	e6 e9                	out    0xe9,al
    a0c9:	e9 d1 ff ff ff       	jmp    0xa09f
    a0ce:	b0 0a                	mov    al,0xa
    a0d0:	e6 e9                	out    0xe9,al
    a0d2:	48 8d 8d bb fc ff ff 	lea    rcx,[rbp-0x345]
    a0d9:	48 ba 70 61 67 65 73 	movabs rdx,0x203d207365676170
    a0e0:	20 3d 20 
    a0e3:	48 89 11             	mov    QWORD PTR [rcx],rdx
    a0e6:	c6 41 08 00          	mov    BYTE PTR [rcx+0x8],0x0
    a0ea:	c7 85 b4 fc ff ff 00 	mov    DWORD PTR [rbp-0x34c],0x0
    a0f1:	00 00 00 
    a0f4:	48 63 85 b4 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x34c]
    a0fb:	80 bc 05 bb fc ff ff 	cmp    BYTE PTR [rbp+rax*1-0x345],0x0
    a102:	00 
    a103:	0f 84 22 00 00 00    	je     0xa12b
    a109:	8b 85 b4 fc ff ff    	mov    eax,DWORD PTR [rbp-0x34c]
    a10f:	89 c1                	mov    ecx,eax
    a111:	83 c1 01             	add    ecx,0x1
    a114:	89 8d b4 fc ff ff    	mov    DWORD PTR [rbp-0x34c],ecx
    a11a:	48 63 d0             	movsxd rdx,eax
    a11d:	8a 84 15 bb fc ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x345]
    a124:	e6 e9                	out    0xe9,al
    a126:	e9 c9 ff ff ff       	jmp    0xa0f4
    a12b:	48 c7 85 a8 fc ff ff 	mov    QWORD PTR [rbp-0x358],0x0
    a132:	00 00 00 00 
    a136:	48 c7 85 a0 fc ff ff 	mov    QWORD PTR [rbp-0x360],0x0
    a13d:	00 00 00 00 
    a141:	48 c7 85 98 fc ff ff 	mov    QWORD PTR [rbp-0x368],0x0
    a148:	00 00 00 00 
    a14c:	48 c7 85 90 fc ff ff 	mov    QWORD PTR [rbp-0x370],0x0
    a153:	00 00 00 00 
    a157:	48 c7 85 88 fc ff ff 	mov    QWORD PTR [rbp-0x378],0x0
    a15e:	00 00 00 00 
    a162:	48 c7 85 80 fc ff ff 	mov    QWORD PTR [rbp-0x380],0x0
    a169:	00 00 00 00 
    a16d:	48 c7 85 78 fc ff ff 	mov    QWORD PTR [rbp-0x388],0x0
    a174:	00 00 00 00 
    a178:	48 c7 85 70 fc ff ff 	mov    QWORD PTR [rbp-0x390],0x0
    a17f:	00 00 00 00 
    a183:	c7 85 6c fc ff ff 00 	mov    DWORD PTR [rbp-0x394],0x0
    a18a:	00 00 00 
    a18d:	48 c7 85 60 fc ff ff 	mov    QWORD PTR [rbp-0x3a0],0x5
    a194:	05 00 00 00 
    a198:	48 8b 85 60 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3a0]
    a19f:	48 89 85 58 fc ff ff 	mov    QWORD PTR [rbp-0x3a8],rax
    a1a6:	48 8b 85 60 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3a0]
    a1ad:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    a1b4:	cc cc cc 
    a1b7:	48 f7 e1             	mul    rcx
    a1ba:	48 c1 ea 03          	shr    rdx,0x3
    a1be:	48 89 95 60 fc ff ff 	mov    QWORD PTR [rbp-0x3a0],rdx
    a1c5:	48 8b 85 58 fc ff ff 	mov    rax,QWORD PTR [rbp-0x3a8]
    a1cc:	48 8b 8d 60 fc ff ff 	mov    rcx,QWORD PTR [rbp-0x3a0]
    a1d3:	48 01 c9             	add    rcx,rcx
    a1d6:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    a1da:	48 29 c8             	sub    rax,rcx
    a1dd:	48 89 c1             	mov    rcx,rax
    a1e0:	48 83 c1 23          	add    rcx,0x23
    a1e4:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    a1eb:	40 8a 30             	mov    sil,BYTE PTR [rax]
    a1ee:	8b bd 6c fc ff ff    	mov    edi,DWORD PTR [rbp-0x394]
    a1f4:	41 89 f8             	mov    r8d,edi
    a1f7:	41 83 c0 01          	add    r8d,0x1
    a1fb:	44 89 85 6c fc ff ff 	mov    DWORD PTR [rbp-0x394],r8d
    a202:	48 63 c7             	movsxd rax,edi
    a205:	40 88 b4 05 70 fc ff 	mov    BYTE PTR [rbp+rax*1-0x390],sil
    a20c:	ff 
    a20d:	48 83 bd 60 fc ff ff 	cmp    QWORD PTR [rbp-0x3a0],0x0
    a214:	00 
    a215:	0f 85 7d ff ff ff    	jne    0xa198
    a21b:	8b 85 6c fc ff ff    	mov    eax,DWORD PTR [rbp-0x394]
    a221:	89 85 54 fc ff ff    	mov    DWORD PTR [rbp-0x3ac],eax
    a227:	8b 85 54 fc ff ff    	mov    eax,DWORD PTR [rbp-0x3ac]
    a22d:	89 c1                	mov    ecx,eax
    a22f:	83 c1 ff             	add    ecx,0xffffffff
    a232:	89 8d 54 fc ff ff    	mov    DWORD PTR [rbp-0x3ac],ecx
    a238:	83 f8 00             	cmp    eax,0x0
    a23b:	0f 84 15 00 00 00    	je     0xa256
    a241:	48 63 85 54 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x3ac]
    a248:	8a 84 05 70 fc ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x390]
    a24f:	e6 e9                	out    0xe9,al
    a251:	e9 d1 ff ff ff       	jmp    0xa227
    a256:	b0 0a                	mov    al,0xa
    a258:	e6 e9                	out    0xe9,al
    a25a:	48 8d 8d 49 fc ff ff 	lea    rcx,[rbp-0x3b7]
    a261:	48 ba 6b 6d 61 69 6e 	movabs rdx,0x203d206e69616d6b
    a268:	20 3d 20 
    a26b:	48 89 11             	mov    QWORD PTR [rcx],rdx
    a26e:	c7 41 07 20 30 78 00 	mov    DWORD PTR [rcx+0x7],0x783020
    a275:	c7 85 44 fc ff ff 00 	mov    DWORD PTR [rbp-0x3bc],0x0
    a27c:	00 00 00 
    a27f:	48 63 85 44 fc ff ff 	movsxd rax,DWORD PTR [rbp-0x3bc]
    a286:	80 bc 05 49 fc ff ff 	cmp    BYTE PTR [rbp+rax*1-0x3b7],0x0
    a28d:	00 
    a28e:	0f 84 22 00 00 00    	je     0xa2b6
    a294:	8b 85 44 fc ff ff    	mov    eax,DWORD PTR [rbp-0x3bc]
    a29a:	89 c1                	mov    ecx,eax
    a29c:	83 c1 01             	add    ecx,0x1
    a29f:	89 8d 44 fc ff ff    	mov    DWORD PTR [rbp-0x3bc],ecx
    a2a5:	48 63 d0             	movsxd rdx,eax
    a2a8:	8a 84 15 49 fc ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x3b7]
    a2af:	e6 e9                	out    0xe9,al
    a2b1:	e9 c9 ff ff ff       	jmp    0xa27f
    a2b6:	48 c7 85 38 fc ff ff 	mov    QWORD PTR [rbp-0x3c8],0x0
    a2bd:	00 00 00 00 
    a2c1:	48 c7 85 30 fc ff ff 	mov    QWORD PTR [rbp-0x3d0],0x0
    a2c8:	00 00 00 00 
    a2cc:	48 c7 85 28 fc ff ff 	mov    QWORD PTR [rbp-0x3d8],0x0
    a2d3:	00 00 00 00 
    a2d7:	48 c7 85 20 fc ff ff 	mov    QWORD PTR [rbp-0x3e0],0x0
    a2de:	00 00 00 00 
    a2e2:	48 c7 85 18 fc ff ff 	mov    QWORD PTR [rbp-0x3e8],0x0
    a2e9:	00 00 00 00 
    a2ed:	48 c7 85 10 fc ff ff 	mov    QWORD PTR [rbp-0x3f0],0x0
    a2f4:	00 00 00 00 
    a2f8:	48 c7 85 08 fc ff ff 	mov    QWORD PTR [rbp-0x3f8],0x0
    a2ff:	00 00 00 00 
    a303:	48 c7 85 00 fc ff ff 	mov    QWORD PTR [rbp-0x400],0x0
    a30a:	00 00 00 00 
    a30e:	c7 85 fc fb ff ff 00 	mov    DWORD PTR [rbp-0x404],0x0
    a315:	00 00 00 
    a318:	48 c7 85 f0 fb ff ff 	mov    QWORD PTR [rbp-0x410],0xffffffff8000b000
    a31f:	00 b0 00 80 
    a323:	48 8b 85 f0 fb ff ff 	mov    rax,QWORD PTR [rbp-0x410]
    a32a:	48 89 85 e8 fb ff ff 	mov    QWORD PTR [rbp-0x418],rax
    a331:	48 8b 85 f0 fb ff ff 	mov    rax,QWORD PTR [rbp-0x410]
    a338:	48 c1 e8 04          	shr    rax,0x4
    a33c:	48 89 85 f0 fb ff ff 	mov    QWORD PTR [rbp-0x410],rax
    a343:	48 8b 85 e8 fb ff ff 	mov    rax,QWORD PTR [rbp-0x418]
    a34a:	48 8b 8d f0 fb ff ff 	mov    rcx,QWORD PTR [rbp-0x410]
    a351:	48 c1 e1 04          	shl    rcx,0x4
    a355:	48 29 c8             	sub    rax,rcx
    a358:	48 89 c1             	mov    rcx,rax
    a35b:	48 83 c1 23          	add    rcx,0x23
    a35f:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    a366:	8a 10                	mov    dl,BYTE PTR [rax]
    a368:	8b b5 fc fb ff ff    	mov    esi,DWORD PTR [rbp-0x404]
    a36e:	89 f7                	mov    edi,esi
    a370:	83 c7 01             	add    edi,0x1
    a373:	89 bd fc fb ff ff    	mov    DWORD PTR [rbp-0x404],edi
    a379:	48 63 c6             	movsxd rax,esi
    a37c:	88 94 05 00 fc ff ff 	mov    BYTE PTR [rbp+rax*1-0x400],dl
    a383:	48 83 bd f0 fb ff ff 	cmp    QWORD PTR [rbp-0x410],0x0
    a38a:	00 
    a38b:	0f 85 92 ff ff ff    	jne    0xa323
    a391:	8b 85 fc fb ff ff    	mov    eax,DWORD PTR [rbp-0x404]
    a397:	89 85 e4 fb ff ff    	mov    DWORD PTR [rbp-0x41c],eax
    a39d:	8b 85 e4 fb ff ff    	mov    eax,DWORD PTR [rbp-0x41c]
    a3a3:	89 c1                	mov    ecx,eax
    a3a5:	83 c1 ff             	add    ecx,0xffffffff
    a3a8:	89 8d e4 fb ff ff    	mov    DWORD PTR [rbp-0x41c],ecx
    a3ae:	83 f8 00             	cmp    eax,0x0
    a3b1:	0f 84 15 00 00 00    	je     0xa3cc
    a3b7:	48 63 85 e4 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x41c]
    a3be:	8a 84 05 00 fc ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x400]
    a3c5:	e6 e9                	out    0xe9,al
    a3c7:	e9 d1 ff ff ff       	jmp    0xa39d
    a3cc:	b0 0a                	mov    al,0xa
    a3ce:	e6 e9                	out    0xe9,al
    a3d0:	48 8d 8d d9 fb ff ff 	lea    rcx,[rbp-0x427]
    a3d7:	48 ba 6d 65 6d 6f 72 	movabs rdx,0x3d2079726f6d656d
    a3de:	79 20 3d 
    a3e1:	48 89 11             	mov    QWORD PTR [rcx],rdx
    a3e4:	c7 41 07 3d 20 28 00 	mov    DWORD PTR [rcx+0x7],0x28203d
    a3eb:	c7 85 d4 fb ff ff 00 	mov    DWORD PTR [rbp-0x42c],0x0
    a3f2:	00 00 00 
    a3f5:	48 63 85 d4 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x42c]
    a3fc:	80 bc 05 d9 fb ff ff 	cmp    BYTE PTR [rbp+rax*1-0x427],0x0
    a403:	00 
    a404:	0f 84 22 00 00 00    	je     0xa42c
    a40a:	8b 85 d4 fb ff ff    	mov    eax,DWORD PTR [rbp-0x42c]
    a410:	89 c1                	mov    ecx,eax
    a412:	83 c1 01             	add    ecx,0x1
    a415:	89 8d d4 fb ff ff    	mov    DWORD PTR [rbp-0x42c],ecx
    a41b:	48 63 d0             	movsxd rdx,eax
    a41e:	8a 84 15 d9 fb ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x427]
    a425:	e6 e9                	out    0xe9,al
    a427:	e9 c9 ff ff ff       	jmp    0xa3f5
    a42c:	31 c0                	xor    eax,eax
    a42e:	48 8d 8d 90 fb ff ff 	lea    rcx,[rbp-0x470]
    a435:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    a43c:	00 
    a43d:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    a444:	00 
    a445:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    a44c:	00 
    a44d:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    a454:	00 
    a455:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    a45c:	00 
    a45d:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    a464:	00 
    a465:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    a46c:	00 
    a46d:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    a474:	c7 85 8c fb ff ff 00 	mov    DWORD PTR [rbp-0x474],0x0
    a47b:	00 00 00 
    a47e:	48 8b 4d e0          	mov    rcx,QWORD PTR [rbp-0x20]
    a482:	48 89 8d 80 fb ff ff 	mov    QWORD PTR [rbp-0x480],rcx
    a489:	48 8b 85 80 fb ff ff 	mov    rax,QWORD PTR [rbp-0x480]
    a490:	48 89 85 78 fb ff ff 	mov    QWORD PTR [rbp-0x488],rax
    a497:	48 8b 85 80 fb ff ff 	mov    rax,QWORD PTR [rbp-0x480]
    a49e:	48 b9 cd cc cc cc cc 	movabs rcx,0xcccccccccccccccd
    a4a5:	cc cc cc 
    a4a8:	48 f7 e1             	mul    rcx
    a4ab:	48 c1 ea 03          	shr    rdx,0x3
    a4af:	48 89 95 80 fb ff ff 	mov    QWORD PTR [rbp-0x480],rdx
    a4b6:	48 8b 85 78 fb ff ff 	mov    rax,QWORD PTR [rbp-0x488]
    a4bd:	48 8b 8d 80 fb ff ff 	mov    rcx,QWORD PTR [rbp-0x480]
    a4c4:	48 01 c9             	add    rcx,rcx
    a4c7:	48 8d 0c 89          	lea    rcx,[rcx+rcx*4]
    a4cb:	48 29 c8             	sub    rax,rcx
    a4ce:	48 89 c1             	mov    rcx,rax
    a4d1:	48 83 c1 23          	add    rcx,0x23
    a4d5:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    a4dc:	40 8a 30             	mov    sil,BYTE PTR [rax]
    a4df:	8b bd 8c fb ff ff    	mov    edi,DWORD PTR [rbp-0x474]
    a4e5:	41 89 f8             	mov    r8d,edi
    a4e8:	41 83 c0 01          	add    r8d,0x1
    a4ec:	44 89 85 8c fb ff ff 	mov    DWORD PTR [rbp-0x474],r8d
    a4f3:	48 63 c7             	movsxd rax,edi
    a4f6:	40 88 b4 05 90 fb ff 	mov    BYTE PTR [rbp+rax*1-0x470],sil
    a4fd:	ff 
    a4fe:	48 83 bd 80 fb ff ff 	cmp    QWORD PTR [rbp-0x480],0x0
    a505:	00 
    a506:	0f 85 7d ff ff ff    	jne    0xa489
    a50c:	8b 85 8c fb ff ff    	mov    eax,DWORD PTR [rbp-0x474]
    a512:	89 85 74 fb ff ff    	mov    DWORD PTR [rbp-0x48c],eax
    a518:	8b 85 74 fb ff ff    	mov    eax,DWORD PTR [rbp-0x48c]
    a51e:	89 c1                	mov    ecx,eax
    a520:	83 c1 ff             	add    ecx,0xffffffff
    a523:	89 8d 74 fb ff ff    	mov    DWORD PTR [rbp-0x48c],ecx
    a529:	83 f8 00             	cmp    eax,0x0
    a52c:	0f 84 15 00 00 00    	je     0xa547
    a532:	48 63 85 74 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x48c]
    a539:	8a 84 05 90 fb ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x470]
    a540:	e6 e9                	out    0xe9,al
    a542:	e9 d1 ff ff ff       	jmp    0xa518
    a547:	48 8d 85 6f fb ff ff 	lea    rax,[rbp-0x491]
    a54e:	c6 40 04 00          	mov    BYTE PTR [rax+0x4],0x0
    a552:	c7 00 2c 20 30 78    	mov    DWORD PTR [rax],0x7830202c
    a558:	c7 85 68 fb ff ff 00 	mov    DWORD PTR [rbp-0x498],0x0
    a55f:	00 00 00 
    a562:	48 63 85 68 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x498]
    a569:	80 bc 05 6f fb ff ff 	cmp    BYTE PTR [rbp+rax*1-0x491],0x0
    a570:	00 
    a571:	0f 84 22 00 00 00    	je     0xa599
    a577:	8b 85 68 fb ff ff    	mov    eax,DWORD PTR [rbp-0x498]
    a57d:	89 c1                	mov    ecx,eax
    a57f:	83 c1 01             	add    ecx,0x1
    a582:	89 8d 68 fb ff ff    	mov    DWORD PTR [rbp-0x498],ecx
    a588:	48 63 d0             	movsxd rdx,eax
    a58b:	8a 84 15 6f fb ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x491]
    a592:	e6 e9                	out    0xe9,al
    a594:	e9 c9 ff ff ff       	jmp    0xa562
    a599:	31 c0                	xor    eax,eax
    a59b:	48 8d 8d 20 fb ff ff 	lea    rcx,[rbp-0x4e0]
    a5a2:	48 c7 41 38 00 00 00 	mov    QWORD PTR [rcx+0x38],0x0
    a5a9:	00 
    a5aa:	48 c7 41 30 00 00 00 	mov    QWORD PTR [rcx+0x30],0x0
    a5b1:	00 
    a5b2:	48 c7 41 28 00 00 00 	mov    QWORD PTR [rcx+0x28],0x0
    a5b9:	00 
    a5ba:	48 c7 41 20 00 00 00 	mov    QWORD PTR [rcx+0x20],0x0
    a5c1:	00 
    a5c2:	48 c7 41 18 00 00 00 	mov    QWORD PTR [rcx+0x18],0x0
    a5c9:	00 
    a5ca:	48 c7 41 10 00 00 00 	mov    QWORD PTR [rcx+0x10],0x0
    a5d1:	00 
    a5d2:	48 c7 41 08 00 00 00 	mov    QWORD PTR [rcx+0x8],0x0
    a5d9:	00 
    a5da:	48 c7 01 00 00 00 00 	mov    QWORD PTR [rcx],0x0
    a5e1:	c7 85 1c fb ff ff 00 	mov    DWORD PTR [rbp-0x4e4],0x0
    a5e8:	00 00 00 
    a5eb:	48 8b 4d e8          	mov    rcx,QWORD PTR [rbp-0x18]
    a5ef:	48 89 8d 10 fb ff ff 	mov    QWORD PTR [rbp-0x4f0],rcx
    a5f6:	48 8b 85 10 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4f0]
    a5fd:	48 89 85 08 fb ff ff 	mov    QWORD PTR [rbp-0x4f8],rax
    a604:	48 8b 85 10 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4f0]
    a60b:	48 c1 e8 04          	shr    rax,0x4
    a60f:	48 89 85 10 fb ff ff 	mov    QWORD PTR [rbp-0x4f0],rax
    a616:	48 8b 85 08 fb ff ff 	mov    rax,QWORD PTR [rbp-0x4f8]
    a61d:	48 8b 8d 10 fb ff ff 	mov    rcx,QWORD PTR [rbp-0x4f0]
    a624:	48 c1 e1 04          	shl    rcx,0x4
    a628:	48 29 c8             	sub    rax,rcx
    a62b:	48 89 c1             	mov    rcx,rax
    a62e:	48 83 c1 23          	add    rcx,0x23
    a632:	48 8d 80 a3 a7 00 00 	lea    rax,[rax+0xa7a3]
    a639:	8a 10                	mov    dl,BYTE PTR [rax]
    a63b:	8b b5 1c fb ff ff    	mov    esi,DWORD PTR [rbp-0x4e4]
    a641:	89 f7                	mov    edi,esi
    a643:	83 c7 01             	add    edi,0x1
    a646:	89 bd 1c fb ff ff    	mov    DWORD PTR [rbp-0x4e4],edi
    a64c:	48 63 c6             	movsxd rax,esi
    a64f:	88 94 05 20 fb ff ff 	mov    BYTE PTR [rbp+rax*1-0x4e0],dl
    a656:	48 83 bd 10 fb ff ff 	cmp    QWORD PTR [rbp-0x4f0],0x0
    a65d:	00 
    a65e:	0f 85 92 ff ff ff    	jne    0xa5f6
    a664:	8b 85 1c fb ff ff    	mov    eax,DWORD PTR [rbp-0x4e4]
    a66a:	89 85 04 fb ff ff    	mov    DWORD PTR [rbp-0x4fc],eax
    a670:	8b 85 04 fb ff ff    	mov    eax,DWORD PTR [rbp-0x4fc]
    a676:	89 c1                	mov    ecx,eax
    a678:	83 c1 ff             	add    ecx,0xffffffff
    a67b:	89 8d 04 fb ff ff    	mov    DWORD PTR [rbp-0x4fc],ecx
    a681:	83 f8 00             	cmp    eax,0x0
    a684:	0f 84 15 00 00 00    	je     0xa69f
    a68a:	48 63 85 04 fb ff ff 	movsxd rax,DWORD PTR [rbp-0x4fc]
    a691:	8a 84 05 20 fb ff ff 	mov    al,BYTE PTR [rbp+rax*1-0x4e0]
    a698:	e6 e9                	out    0xe9,al
    a69a:	e9 d1 ff ff ff       	jmp    0xa670
    a69f:	48 8d 85 01 fb ff ff 	lea    rax,[rbp-0x4ff]
    a6a6:	c6 40 02 00          	mov    BYTE PTR [rax+0x2],0x0
    a6aa:	66 c7 00 29 0a       	mov    WORD PTR [rax],0xa29
    a6af:	c7 85 fc fa ff ff 00 	mov    DWORD PTR [rbp-0x504],0x0
    a6b6:	00 00 00 
    a6b9:	48 63 85 fc fa ff ff 	movsxd rax,DWORD PTR [rbp-0x504]
    a6c0:	80 bc 05 01 fb ff ff 	cmp    BYTE PTR [rbp+rax*1-0x4ff],0x0
    a6c7:	00 
    a6c8:	0f 84 22 00 00 00    	je     0xa6f0
    a6ce:	8b 85 fc fa ff ff    	mov    eax,DWORD PTR [rbp-0x504]
    a6d4:	89 c1                	mov    ecx,eax
    a6d6:	83 c1 01             	add    ecx,0x1
    a6d9:	89 8d fc fa ff ff    	mov    DWORD PTR [rbp-0x504],ecx
    a6df:	48 63 d0             	movsxd rdx,eax
    a6e2:	8a 84 15 01 fb ff ff 	mov    al,BYTE PTR [rbp+rdx*1-0x4ff]
    a6e9:	e6 e9                	out    0xe9,al
    a6eb:	e9 c9 ff ff ff       	jmp    0xa6b9
    a6f0:	c7 85 f8 fa ff ff 00 	mov    DWORD PTR [rbp-0x508],0x0
    a6f7:	00 00 00 
    a6fa:	48 63 85 f8 fa ff ff 	movsxd rax,DWORD PTR [rbp-0x508]
    a701:	48 c7 c1 05 00 00 00 	mov    rcx,0x5
    a708:	48 29 c8             	sub    rax,rcx
    a70b:	0f 83 43 00 00 00    	jae    0xa754
    a711:	e9 00 00 00 00       	jmp    0xa716
    a716:	8b 85 f8 fa ff ff    	mov    eax,DWORD PTR [rbp-0x508]
    a71c:	c1 e0 0c             	shl    eax,0xc
    a71f:	89 c1                	mov    ecx,eax
    a721:	81 c1 00 00 10 00    	add    ecx,0x100000
    a727:	48 63 f1             	movsxd rsi,ecx
    a72a:	48 63 d0             	movsxd rdx,eax
    a72d:	48 8d 92 00 b0 00 80 	lea    rdx,[rdx-0x7fff5000]
    a734:	48 c7 c7 00 b0 00 00 	mov    rdi,0xb000
    a73b:	e8 f0 da ff ff       	call   0x8230
    a740:	8b 85 f8 fa ff ff    	mov    eax,DWORD PTR [rbp-0x508]
    a746:	83 c0 01             	add    eax,0x1
    a749:	89 85 f8 fa ff ff    	mov    DWORD PTR [rbp-0x508],eax
    a74f:	e9 a6 ff ff ff       	jmp    0xa6fa
    a754:	90                   	nop
    a755:	90                   	nop
    a756:	90                   	nop
    a757:	48 8b 7d e0          	mov    rdi,QWORD PTR [rbp-0x20]
    a75b:	48 8b 75 e8          	mov    rsi,QWORD PTR [rbp-0x18]
    a75f:	48 c7 c2 00 b0 00 00 	mov    rdx,0xb000
    a766:	e8 95 08 00 80       	call   0xffffffff8000b000
    a76b:	90                   	nop
    a76c:	90                   	nop
    a76d:	90                   	nop
    a76e:	48 81 c4 10 05 00 00 	add    rsp,0x510
    a775:	5d                   	pop    rbp
    a776:	c3                   	ret    
    a777:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
    a77e:	00 00 
    a780:	7a 79                	jp     0xa7fb
    a782:	78 77                	js     0xa7fb
    a784:	76 75                	jbe    0xa7fb
    a786:	74 73                	je     0xa7fb
    a788:	72 71                	jb     0xa7fb
    a78a:	70 6f                	jo     0xa7fb
    a78c:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    a78d:	6d                   	ins    DWORD PTR es:[rdi],dx
    a78e:	6c                   	ins    BYTE PTR es:[rdi],dx
    a78f:	6b 6a 69 68          	imul   ebp,DWORD PTR [rdx+0x69],0x68
    a793:	67 66 65 64 63 62 61 	gs movsxd sp,DWORD PTR fs:[edx+0x61]
    a79a:	39 38                	cmp    DWORD PTR [rax],edi
    a79c:	37                   	(bad)  
    a79d:	36 35 34 33 32 31    	ss xor eax,0x31323334
    a7a3:	30 31                	xor    BYTE PTR [rcx],dh
    a7a5:	32 33                	xor    dh,BYTE PTR [rbx]
    a7a7:	34 35                	xor    al,0x35
    a7a9:	36 37                	ss (bad) 
    a7ab:	38 39                	cmp    BYTE PTR [rcx],bh
    a7ad:	61                   	(bad)  
    a7ae:	62 63 64 65 66       	(bad)  {k5}
    a7b3:	67 68 69 6a 6b 6c    	addr32 push 0x6c6b6a69
    a7b9:	6d                   	ins    DWORD PTR es:[rdi],dx
    a7ba:	6e                   	outs   dx,BYTE PTR ds:[rsi]
    a7bb:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    a7bc:	70 71                	jo     0xa82f
    a7be:	72 73                	jb     0xa833
    a7c0:	74 75                	je     0xa837
    a7c2:	76 77                	jbe    0xa83b
    a7c4:	78 79                	js     0xa83f
    a7c6:	7a 00                	jp     0xa7c8
	...
    b000:	55                   	push   rbp
    b001:	48 89 e5             	mov    rbp,rsp
    b004:	48 83 ec 18          	sub    rsp,0x18
    b008:	48 89 7d f0          	mov    QWORD PTR [rbp-0x10],rdi
    b00c:	48 89 75 f8          	mov    QWORD PTR [rbp-0x8],rsi
    b010:	48 89 55 e8          	mov    QWORD PTR [rbp-0x18],rdx
    b014:	e9 fb ff ff ff       	jmp    0xb014
    b019:	0f 1f 00             	nop    DWORD PTR [rax]
	...
    c000:	00 d0                	add    al,dl
    c002:	00 80 ff ff ff ff    	add    BYTE PTR [rax-0x1],al
	...
    d000:	79 65                	jns    0xd067
    d002:	73 00                	jae    0xd004
    d004:	61                   	(bad)  
    d005:	6c                   	ins    BYTE PTR es:[rdi],dx
    d006:	6c                   	ins    BYTE PTR es:[rdi],dx
    d007:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    d008:	63 61 74             	movsxd esp,DWORD PTR [rcx+0x74]
    d00b:	65 64 20 30          	gs and BYTE PTR fs:[rax],dh
    d00f:	78 00                	js     0xd011
    d011:	6d                   	ins    DWORD PTR es:[rdi],dx
    d012:	61                   	(bad)  
    d013:	70 70                	jo     0xd085
    d015:	69 6e 67 20 30 78 00 	imul   ebp,DWORD PTR [rsi+0x67],0x783020
    d01c:	20 74 6f 20          	and    BYTE PTR [rdi+rbp*2+0x20],dh
    d020:	30 78 00             	xor    BYTE PTR [rax+0x0],bh
    d023:	30 78 00             	xor    BYTE PTR [rax+0x0],bh
    d026:	20 70 6d             	and    BYTE PTR [rax+0x6d],dh
    d029:	6c                   	ins    BYTE PTR es:[rdi],dx
    d02a:	34 5b                	xor    al,0x5b
    d02c:	00 5d 20             	add    BYTE PTR [rbp+0x20],bl
    d02f:	3d 20 30 78 00       	cmp    eax,0x783020
    d034:	30 78 00             	xor    BYTE PTR [rax+0x0],bh
    d037:	20 70 64             	and    BYTE PTR [rax+0x64],dh
    d03a:	70 74                	jo     0xd0b0
    d03c:	5b                   	pop    rbx
    d03d:	00 5d 20             	add    BYTE PTR [rbp+0x20],bl
    d040:	3d 20 30 78 00       	cmp    eax,0x783020
    d045:	30 78 00             	xor    BYTE PTR [rax+0x0],bh
    d048:	20 70 64             	and    BYTE PTR [rax+0x64],dh
    d04b:	5b                   	pop    rbx
    d04c:	00 5d 20             	add    BYTE PTR [rbp+0x20],bl
    d04f:	3d 20 30 78 00       	cmp    eax,0x783020
    d054:	30 78 00             	xor    BYTE PTR [rax+0x0],bh
    d057:	20 70 74             	and    BYTE PTR [rax+0x74],dh
    d05a:	5b                   	pop    rbx
    d05b:	00 5d 20             	add    BYTE PTR [rbp+0x20],bl
    d05e:	3d 20 30 78 00       	cmp    eax,0x783020
    d063:	62                   	(bad)  
    d064:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    d065:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    d066:	74 20                	je     0xd088
    d068:	3d 20 30 78 00       	cmp    eax,0x783020
    d06d:	70 6d                	jo     0xd0dc
    d06f:	6c                   	ins    BYTE PTR es:[rdi],dx
    d070:	34 20                	xor    al,0x20
    d072:	3d 20 30 78 00       	cmp    eax,0x783020
    d077:	2e 74 65             	cs je  0xd0df
    d07a:	78 74                	js     0xd0f0
    d07c:	20 3d 20 30 78 00    	and    BYTE PTR [rip+0x783020],bh        # 0x7900a2
    d082:	2e 64 61             	cs fs (bad) 
    d085:	74 61                	je     0xd0e8
    d087:	20 3d 20 30 78 00    	and    BYTE PTR [rip+0x783020],bh        # 0x7900ad
    d08d:	2e 72 6f             	cs jb  0xd0ff
    d090:	64 61                	fs (bad) 
    d092:	74 61                	je     0xd0f5
    d094:	20 3d 20 30 78 00    	and    BYTE PTR [rip+0x783020],bh        # 0x7900ba
    d09a:	2e 62 73             	cs (bad) 
    d09d:	73 20                	jae    0xd0bf
    d09f:	3d 20 30 78 00       	cmp    eax,0x783020
    d0a4:	70 61                	jo     0xd107
    d0a6:	67 65 73 20          	addr32 gs jae 0xd0ca
    d0aa:	3d 20 00 6b 6d       	cmp    eax,0x6d6b0020
    d0af:	61                   	(bad)  
    d0b0:	69 6e 20 3d 20 30 78 	imul   ebp,DWORD PTR [rsi+0x20],0x7830203d
    d0b7:	00 6d 65             	add    BYTE PTR [rbp+0x65],ch
    d0ba:	6d                   	ins    DWORD PTR es:[rdi],dx
    d0bb:	6f                   	outs   dx,DWORD PTR ds:[rsi]
    d0bc:	72 79                	jb     0xd137
    d0be:	20 3d 20 28 00 2c    	and    BYTE PTR [rip+0x2c002820],bh        # 0x2c00f8e4
    d0c4:	20 30                	and    BYTE PTR [rax],dh
    d0c6:	78 00                	js     0xd0c8
    d0c8:	29 0a                	sub    DWORD PTR [rdx],ecx
    d0ca:	00 00                	add    BYTE PTR [rax],al
    d0cc:	00 00                	add    BYTE PTR [rax],al
    d0ce:	00 00                	add    BYTE PTR [rax],al
    d0d0:	4b                   	rex.WXB
    d0d1:	45 52                	rex.RB push r10
    d0d3:	4e                   	rex.WRX
    d0d4:	45                   	rex.RB
    d0d5:	4c 5f                	rex.WR pop rdi
    d0d7:	56                   	push   rsi
    d0d8:	41                   	rex.B
    d0d9:	44                   	rex.R
    d0da:	44 52                	rex.R push rdx
    d0dc:	20 3d 20 30 78 00    	and    BYTE PTR [rip+0x783020],bh        # 0x790102
