; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-linux | FileCheck %s -check-prefixes=X86
; RUN: llc < %s -mtriple=x86_64-linux | FileCheck %s -check-prefixes=X64

define i1 @is_nan_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_nan_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    fldt {{[0-9]+}}(%esp)
; X86-NEXT:    fucomp %st(0)
; X86-NEXT:    fnstsw %ax
; X86-NEXT:    # kill: def $ah killed $ah killed $ax
; X86-NEXT:    sahf
; X86-NEXT:    setp %al
; X86-NEXT:    retl
;
; X64-LABEL: is_nan_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    fldt {{[0-9]+}}(%rsp)
; X64-NEXT:    fucompi %st(0), %st
; X64-NEXT:    setp %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 3)  ; "nan"
  ret i1 %0
}

define i1 @is_nan_f80_strict(x86_fp80 %x) nounwind strictfp {
; X86-LABEL: is_nan_f80_strict:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    andl $32767, %ecx # imm = 0x7FFF
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    cmpl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    movl $-2147483648, %esi # imm = 0x80000000
; X86-NEXT:    sbbl %eax, %esi
; X86-NEXT:    movl $32767, %esi # imm = 0x7FFF
; X86-NEXT:    sbbl %ecx, %esi
; X86-NEXT:    sbbl %edx, %edx
; X86-NEXT:    setl %dl
; X86-NEXT:    testl %ecx, %ecx
; X86-NEXT:    sete %cl
; X86-NEXT:    shrl $31, %eax
; X86-NEXT:    xorb %cl, %al
; X86-NEXT:    xorb $1, %al
; X86-NEXT:    orb %dl, %al
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: is_nan_f80_strict:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    movabsq $-9223372036854775808, %rdx # imm = 0x8000000000000000
; X64-NEXT:    cmpq %rcx, %rdx
; X64-NEXT:    movl $32767, %edx # imm = 0x7FFF
; X64-NEXT:    sbbq %rax, %rdx
; X64-NEXT:    setl %dl
; X64-NEXT:    shrq $63, %rcx
; X64-NEXT:    testq %rax, %rax
; X64-NEXT:    sete %al
; X64-NEXT:    xorb %cl, %al
; X64-NEXT:    xorb $1, %al
; X64-NEXT:    orb %dl, %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 3) strictfp ; "nan"
  ret i1 %0
}

define i1 @is_snan_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_snan_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %ebx
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    xorl %ecx, %ecx
; X86-NEXT:    cmpl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl $-2147483648, %esi # imm = 0x80000000
; X86-NEXT:    sbbl %edx, %esi
; X86-NEXT:    movl $32767, %esi # imm = 0x7FFF
; X86-NEXT:    sbbl %eax, %esi
; X86-NEXT:    movl $0, %esi
; X86-NEXT:    sbbl %esi, %esi
; X86-NEXT:    setl %bl
; X86-NEXT:    cmpl $-1073741824, %edx # imm = 0xC0000000
; X86-NEXT:    sbbl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    sbbl %ecx, %ecx
; X86-NEXT:    setl %al
; X86-NEXT:    andb %bl, %al
; X86-NEXT:    popl %esi
; X86-NEXT:    popl %ebx
; X86-NEXT:    retl
;
; X64-LABEL: is_snan_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    movabsq $-4611686018427387904, %rdx # imm = 0xC000000000000000
; X64-NEXT:    cmpq %rdx, %rcx
; X64-NEXT:    movq %rax, %rdx
; X64-NEXT:    sbbq $32767, %rdx # imm = 0x7FFF
; X64-NEXT:    setl %dl
; X64-NEXT:    movabsq $-9223372036854775808, %rsi # imm = 0x8000000000000000
; X64-NEXT:    cmpq %rcx, %rsi
; X64-NEXT:    movl $32767, %ecx # imm = 0x7FFF
; X64-NEXT:    sbbq %rax, %rcx
; X64-NEXT:    setl %al
; X64-NEXT:    andb %dl, %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 1)  ; "snan"
  ret i1 %0
}

define i1 @is_qnan_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_qnan_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    xorl %ecx, %ecx
; X86-NEXT:    movl $-1073741825, %edx # imm = 0xBFFFFFFF
; X86-NEXT:    cmpl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    movl $32767, %edx # imm = 0x7FFF
; X86-NEXT:    sbbl %eax, %edx
; X86-NEXT:    sbbl %ecx, %ecx
; X86-NEXT:    setl %al
; X86-NEXT:    retl
;
; X64-LABEL: is_qnan_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    movabsq $-4611686018427387905, %rcx # imm = 0xBFFFFFFFFFFFFFFF
; X64-NEXT:    cmpq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    movl $32767, %ecx # imm = 0x7FFF
; X64-NEXT:    sbbq %rax, %rcx
; X64-NEXT:    setl %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 2)  ; "qnan"
  ret i1 %0
}

define i1 @is_zero_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_zero_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_zero_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    orq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 96)  ; 0x60 = "zero"
  ret i1 %0
}

define i1 @is_zero_f80_strict(x86_fp80 %x) nounwind strictfp {
; X86-LABEL: is_zero_f80_strict:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_zero_f80_strict:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    orq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 96) strictfp ; 0x60 = "zero"
  ret i1 %0
}

define i1 @is_poszero_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_poszero_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_poszero_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    orq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 64)  ; 0x40 = "+zero"
  ret i1 %0
}

define i1 @is_negzero_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_negzero_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    xorl $32768, %eax # imm = 0x8000
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_negzero_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    xorq $32768, %rax # imm = 0x8000
; X64-NEXT:    orq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 32)  ; 0x20 = "-zero"
  ret i1 %0
}

define i1 @is_inf_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_inf_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    notl %eax
; X86-NEXT:    movl $-2147483648, %ecx # imm = 0x80000000
; X86-NEXT:    xorl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl %ecx, %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_inf_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    notl %eax
; X64-NEXT:    movabsq $-9223372036854775808, %rcx # imm = 0x8000000000000000
; X64-NEXT:    xorq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    orq %rcx, %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 516)  ; 0x204 = "inf"
  ret i1 %0
}

define i1 @is_posinf_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_posinf_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movl $-2147483648, %ecx # imm = 0x80000000
; X86-NEXT:    xorl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    xorl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl %ecx, %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_posinf_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movabsq $-9223372036854775808, %rcx # imm = 0x8000000000000000
; X64-NEXT:    xorq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    xorq $32767, %rax # imm = 0x7FFF
; X64-NEXT:    orq %rcx, %rax
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 512)  ; 0x200 = "+inf"
  ret i1 %0
}

define i1 @is_neginf_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_neginf_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    xorl $65535, %eax # imm = 0xFFFF
; X86-NEXT:    movl $-2147483648, %ecx # imm = 0x80000000
; X86-NEXT:    xorl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    orl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    orl %ecx, %eax
; X86-NEXT:    sete %al
; X86-NEXT:    retl
;
; X64-LABEL: is_neginf_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    xorq $65535, %rax # imm = 0xFFFF
; X64-NEXT:    movabsq $-9223372036854775808, %rcx # imm = 0x8000000000000000
; X64-NEXT:    xorq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    orq %rax, %rcx
; X64-NEXT:    sete %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 4)  ; "-inf"
  ret i1 %0
}

define i1 @is_normal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_normal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    andl $32767, %ecx # imm = 0x7FFF
; X86-NEXT:    decl %ecx
; X86-NEXT:    movzwl %cx, %ecx
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    cmpl $32766, %ecx # imm = 0x7FFE
; X86-NEXT:    sbbl %edx, %edx
; X86-NEXT:    setb %cl
; X86-NEXT:    shrl $31, %eax
; X86-NEXT:    andb %cl, %al
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    retl
;
; X64-LABEL: is_normal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    shrq $63, %rcx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    decl %eax
; X64-NEXT:    movzwl %ax, %eax
; X64-NEXT:    cmpl $32766, %eax # imm = 0x7FFE
; X64-NEXT:    setb %al
; X64-NEXT:    andb %cl, %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 264)  ; 0x108 = "normal"
  ret i1 %0
}

define i1 @is_posnormal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_posnormal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl %ecx, %edx
; X86-NEXT:    andl $32767, %edx # imm = 0x7FFF
; X86-NEXT:    decl %edx
; X86-NEXT:    movzwl %dx, %edx
; X86-NEXT:    xorl %esi, %esi
; X86-NEXT:    cmpl $32766, %edx # imm = 0x7FFE
; X86-NEXT:    sbbl %esi, %esi
; X86-NEXT:    setb %dl
; X86-NEXT:    testl $32768, %ecx # imm = 0x8000
; X86-NEXT:    sete %cl
; X86-NEXT:    shrl $31, %eax
; X86-NEXT:    andb %cl, %al
; X86-NEXT:    andb %dl, %al
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: is_posnormal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    movswq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    testq %rcx, %rcx
; X64-NEXT:    setns %dl
; X64-NEXT:    andl $32767, %ecx # imm = 0x7FFF
; X64-NEXT:    decl %ecx
; X64-NEXT:    movzwl %cx, %ecx
; X64-NEXT:    cmpl $32766, %ecx # imm = 0x7FFE
; X64-NEXT:    setb %cl
; X64-NEXT:    shrq $63, %rax
; X64-NEXT:    andb %dl, %al
; X64-NEXT:    andb %cl, %al
; X64-NEXT:    # kill: def $al killed $al killed $rax
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 256)  ; 0x100 = "+normal"
  ret i1 %0
}

define i1 @is_negnormal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_negnormal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl %ecx, %edx
; X86-NEXT:    andl $32767, %edx # imm = 0x7FFF
; X86-NEXT:    decl %edx
; X86-NEXT:    movzwl %dx, %edx
; X86-NEXT:    xorl %esi, %esi
; X86-NEXT:    cmpl $32766, %edx # imm = 0x7FFE
; X86-NEXT:    sbbl %esi, %esi
; X86-NEXT:    setb %dl
; X86-NEXT:    testl $32768, %ecx # imm = 0x8000
; X86-NEXT:    setne %cl
; X86-NEXT:    shrl $31, %eax
; X86-NEXT:    andb %cl, %al
; X86-NEXT:    andb %dl, %al
; X86-NEXT:    # kill: def $al killed $al killed $eax
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: is_negnormal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rax
; X64-NEXT:    movswq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    testq %rcx, %rcx
; X64-NEXT:    sets %dl
; X64-NEXT:    andl $32767, %ecx # imm = 0x7FFF
; X64-NEXT:    decl %ecx
; X64-NEXT:    movzwl %cx, %ecx
; X64-NEXT:    cmpl $32766, %ecx # imm = 0x7FFE
; X64-NEXT:    setb %cl
; X64-NEXT:    shrq $63, %rax
; X64-NEXT:    andb %dl, %al
; X64-NEXT:    andb %cl, %al
; X64-NEXT:    # kill: def $al killed $al killed $rax
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 8)  ; "-normal"
  ret i1 %0
}

define i1 @is_subnormal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_subnormal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X86-NEXT:    xorl %edx, %edx
; X86-NEXT:    addl $-1, %esi
; X86-NEXT:    adcl $-1, %ecx
; X86-NEXT:    adcl $-1, %eax
; X86-NEXT:    adcl $-1, %edx
; X86-NEXT:    cmpl $-1, %esi
; X86-NEXT:    sbbl $2147483647, %ecx # imm = 0x7FFFFFFF
; X86-NEXT:    sbbl $0, %eax
; X86-NEXT:    sbbl $0, %edx
; X86-NEXT:    setb %al
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: is_subnormal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    addq $-1, %rcx
; X64-NEXT:    adcq $-1, %rax
; X64-NEXT:    movabsq $9223372036854775807, %rdx # imm = 0x7FFFFFFFFFFFFFFF
; X64-NEXT:    cmpq %rdx, %rcx
; X64-NEXT:    sbbq $0, %rax
; X64-NEXT:    setb %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 144)  ; 0x90 = "subnormal"
  ret i1 %0
}

define i1 @is_possubnormal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_possubnormal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    addl $-1, %ecx
; X86-NEXT:    adcl $-1, %edx
; X86-NEXT:    adcl $-1, %eax
; X86-NEXT:    movzwl %ax, %eax
; X86-NEXT:    xorl %esi, %esi
; X86-NEXT:    cmpl $-1, %ecx
; X86-NEXT:    sbbl $2147483647, %edx # imm = 0x7FFFFFFF
; X86-NEXT:    sbbl $0, %eax
; X86-NEXT:    sbbl %esi, %esi
; X86-NEXT:    setb %al
; X86-NEXT:    popl %esi
; X86-NEXT:    retl
;
; X64-LABEL: is_possubnormal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rcx
; X64-NEXT:    addq $-1, %rcx
; X64-NEXT:    adcq $-1, %rax
; X64-NEXT:    movzwl %ax, %eax
; X64-NEXT:    movabsq $9223372036854775807, %rdx # imm = 0x7FFFFFFFFFFFFFFF
; X64-NEXT:    cmpq %rdx, %rcx
; X64-NEXT:    sbbq $0, %rax
; X64-NEXT:    setb %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 128)  ; 0x80 = "+subnormal"
  ret i1 %0
}

define i1 @is_negsubnormal_f80(x86_fp80 %x) nounwind {
; X86-LABEL: is_negsubnormal_f80:
; X86:       # %bb.0: # %entry
; X86-NEXT:    pushl %edi
; X86-NEXT:    pushl %esi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edi
; X86-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X86-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; X86-NEXT:    movl %eax, %ecx
; X86-NEXT:    andl $32767, %ecx # imm = 0x7FFF
; X86-NEXT:    xorl %esi, %esi
; X86-NEXT:    addl $-1, %edi
; X86-NEXT:    adcl $-1, %edx
; X86-NEXT:    adcl $-1, %ecx
; X86-NEXT:    adcl $-1, %esi
; X86-NEXT:    cmpl $-1, %edi
; X86-NEXT:    sbbl $2147483647, %edx # imm = 0x7FFFFFFF
; X86-NEXT:    sbbl $0, %ecx
; X86-NEXT:    sbbl $0, %esi
; X86-NEXT:    setb %cl
; X86-NEXT:    testl $32768, %eax # imm = 0x8000
; X86-NEXT:    setne %al
; X86-NEXT:    andb %cl, %al
; X86-NEXT:    popl %esi
; X86-NEXT:    popl %edi
; X86-NEXT:    retl
;
; X64-LABEL: is_negsubnormal_f80:
; X64:       # %bb.0: # %entry
; X64-NEXT:    movzwl {{[0-9]+}}(%rsp), %eax
; X64-NEXT:    movswq %ax, %rcx
; X64-NEXT:    movq {{[0-9]+}}(%rsp), %rdx
; X64-NEXT:    andl $32767, %eax # imm = 0x7FFF
; X64-NEXT:    addq $-1, %rdx
; X64-NEXT:    adcq $-1, %rax
; X64-NEXT:    movabsq $9223372036854775807, %rsi # imm = 0x7FFFFFFFFFFFFFFF
; X64-NEXT:    cmpq %rsi, %rdx
; X64-NEXT:    sbbq $0, %rax
; X64-NEXT:    setb %dl
; X64-NEXT:    testq %rcx, %rcx
; X64-NEXT:    sets %al
; X64-NEXT:    andb %dl, %al
; X64-NEXT:    retq
entry:
  %0 = tail call i1 @llvm.is.fpclass.f80(x86_fp80 %x, i32 16)  ; 0x10 = "-subnormal"
  ret i1 %0
}

declare i1 @llvm.is.fpclass.f80(x86_fp80, i32)
