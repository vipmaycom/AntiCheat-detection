; ============================================================================
; Vanguard Anti-Tamper Module (vgm.exe) Griffin VM Reconstructed Assembly
; Target: C:\vgm.exe (33.8 MB) | Section: .grfn1 (20.34 MB, RVA 0x51C000)
; SHA-256: 55e25cce83c8ec9088b99d1193a7aa750609e69ed8cfc7c758076ec3591de3d9
; EntryPoint: 0x140000000 + 0x19CF13C (.riot1) -> Dispatcher: 0x1883000 (.riot0)
; Griffin Virtual Context Frame: 560 bytes (0x230)
; ============================================================================

.code

; --- Griffin EntryPoint Unpacker Stub (RVA 0x19CF13C) ---
Griffin_EntryPoint proc
    push    rbx
    push    rcx
    push    rdx
    push    r8
    push    r9
    sub     rsp, 20h
    lea     rbx, [rip - 8Eh]        ; Descriptor structure at 0x19CF0B0
    mov     r8, rbx
    call    Griffin_InitUnpacker    ; 0x19C6E27 -> Decrypts .riot0/.grfn1 tables
    add     rsp, 20h
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    test    eax, eax
    jz      Griffin_InitFail
    cmp     eax, 0D4494E49h         ; 'INI\xd4' Magic Verification
    je      Griffin_LaunchDirect
    movsxd  rax, dword ptr [rbx+28h]; Relative Displacement -> .riot0 (0x1883000)
    lea     rax, [rax+rbx]          ; rax = 0x1883000
    mov     rax, [rax+10h]          ; Resolved Target WinMain
    mov     ebx, [rbx+5Ch]          ; RVA Offset Delta
    add     rax, rbx
    pop     rbx
    jmp     rax                     ; Jumps to Reconstructed Vanguard Core WinMain
Griffin_InitFail:
    pop     rbx
    ret
Griffin_LaunchDirect:
    pop     rbx
    ret
Griffin_EntryPoint endp

Griffin_Handler_140530203 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140530203 endp

Griffin_Handler_14054559d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14054559d endp

Griffin_Handler_14054d1cc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14054d1cc endp

Griffin_Handler_14056f8a7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14056f8a7 endp

Griffin_Handler_140578299 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140578299 endp

Griffin_Handler_140583867 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140583867 endp

Griffin_Handler_140586751 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140586751 endp

Griffin_Handler_14058b5dd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14058b5dd endp

Griffin_Handler_14059ba0e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14059ba0e endp

Griffin_Handler_14059d4ae proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14059d4ae endp

Griffin_Handler_1405c2bc0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1405c2bc0 endp

Griffin_Handler_1405c66dd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1405c66dd endp

Griffin_Handler_1405e32b3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1405e32b3 endp

Griffin_Handler_140601c14 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140601c14 endp

Griffin_Handler_14060bdb0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14060bdb0 endp

Griffin_Handler_1406245fe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406245fe endp

Griffin_Handler_14062db97 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14062db97 endp

Griffin_Handler_14062ebe9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14062ebe9 endp

Griffin_Handler_140644389 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140644389 endp

Griffin_Handler_1406970bb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406970bb endp

Griffin_Handler_1406a8665 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406a8665 endp

Griffin_Handler_1406bb486 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406bb486 endp

Griffin_Handler_1406caa9f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406caa9f endp

Griffin_Handler_1406e10fd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1406e10fd endp

Griffin_Handler_14070283d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14070283d endp

Griffin_Handler_14070abd4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14070abd4 endp

Griffin_Handler_140719ba0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140719ba0 endp

Griffin_Handler_14072bd2c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14072bd2c endp

Griffin_Handler_14075cee6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14075cee6 endp

Griffin_Handler_140761711 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140761711 endp

Griffin_Handler_1407c69d7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407c69d7 endp

Griffin_Handler_1407e0cc3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407e0cc3 endp

Griffin_Handler_1407e47a3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407e47a3 endp

Griffin_Handler_1407e583d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407e583d endp

Griffin_Handler_1407ec7d4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407ec7d4 endp

Griffin_Handler_1407f8784 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407f8784 endp

Griffin_Handler_1407fa733 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1407fa733 endp

Griffin_Handler_140800226 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140800226 endp

Griffin_Handler_14080287b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14080287b endp

Griffin_Handler_1408357d0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1408357d0 endp

Griffin_Handler_140843eb8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140843eb8 endp

Griffin_Handler_14087a3e6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14087a3e6 endp

Griffin_Handler_1408830f8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1408830f8 endp

Griffin_Handler_140887085 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140887085 endp

Griffin_Handler_1408a0a2b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1408a0a2b endp

Griffin_Handler_1408eaa79 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1408eaa79 endp

Griffin_Handler_1408f8af5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1408f8af5 endp

Griffin_Handler_1409034bc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409034bc endp

Griffin_Handler_140908a1f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140908a1f endp

Griffin_Handler_14090972d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14090972d endp

Griffin_Handler_14091fd5f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14091fd5f endp

Griffin_Handler_1409205a9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409205a9 endp

Griffin_Handler_14092091e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14092091e endp

Griffin_Handler_14092776c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14092776c endp

Griffin_Handler_14092e2ff proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14092e2ff endp

Griffin_Handler_14096257e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14096257e endp

Griffin_Handler_14098e0b5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14098e0b5 endp

Griffin_Handler_140996154 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140996154 endp

Griffin_Handler_1409a1998 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409a1998 endp

Griffin_Handler_1409af44f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409af44f endp

Griffin_Handler_1409afd0b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409afd0b endp

Griffin_Handler_1409b5857 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409b5857 endp

Griffin_Handler_1409d3a81 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409d3a81 endp

Griffin_Handler_1409e84b4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409e84b4 endp

Griffin_Handler_1409eb6b8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1409eb6b8 endp

Griffin_Handler_140a031f3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a031f3 endp

Griffin_Handler_140a0bafc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a0bafc endp

Griffin_Handler_140a12b21 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a12b21 endp

Griffin_Handler_140a50539 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a50539 endp

Griffin_Handler_140a61d51 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a61d51 endp

Griffin_Handler_140a6bc4d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a6bc4d endp

Griffin_Handler_140a83c2a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140a83c2a endp

Griffin_Handler_140aa1295 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140aa1295 endp

Griffin_Handler_140acaa39 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140acaa39 endp

Griffin_Handler_140b03128 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b03128 endp

Griffin_Handler_140b08294 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b08294 endp

Griffin_Handler_140b274b6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b274b6 endp

Griffin_Handler_140b33163 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b33163 endp

Griffin_Handler_140b39592 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b39592 endp

Griffin_Handler_140b3ac2b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b3ac2b endp

Griffin_Handler_140b3e538 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b3e538 endp

Griffin_Handler_140b4d522 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b4d522 endp

Griffin_Handler_140b56a3c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b56a3c endp

Griffin_Handler_140b92371 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140b92371 endp

Griffin_Handler_140bb27b2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bb27b2 endp

Griffin_Handler_140bbb722 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bbb722 endp

Griffin_Handler_140bbe46d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bbe46d endp

Griffin_Handler_140bcec7c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bcec7c endp

Griffin_Handler_140bd6908 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bd6908 endp

Griffin_Handler_140bda941 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bda941 endp

Griffin_Handler_140bdcbc3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140bdcbc3 endp

Griffin_Handler_140c1ef18 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c1ef18 endp

Griffin_Handler_140c38432 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c38432 endp

Griffin_Handler_140c6ee78 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c6ee78 endp

Griffin_Handler_140c6f09e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c6f09e endp

Griffin_Handler_140c7f41f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c7f41f endp

Griffin_Handler_140c83712 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c83712 endp

Griffin_Handler_140c8c7c9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c8c7c9 endp

Griffin_Handler_140c92325 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c92325 endp

Griffin_Handler_140c99d9d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140c99d9d endp

Griffin_Handler_140ca3ace proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ca3ace endp

Griffin_Handler_140cb22df proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140cb22df endp

Griffin_Handler_140cedda4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140cedda4 endp

Griffin_Handler_140cf7fb5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140cf7fb5 endp

Griffin_Handler_140cfe9fa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140cfe9fa endp

Griffin_Handler_140d0649a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d0649a endp

Griffin_Handler_140d254d5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d254d5 endp

Griffin_Handler_140d2a303 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d2a303 endp

Griffin_Handler_140d58623 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d58623 endp

Griffin_Handler_140d58da9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d58da9 endp

Griffin_Handler_140d6505d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d6505d endp

Griffin_Handler_140d7067b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d7067b endp

Griffin_Handler_140d8c75f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140d8c75f endp

Griffin_Handler_140da38ed proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140da38ed endp

Griffin_Handler_140da73c4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140da73c4 endp

Griffin_Handler_140daa60f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140daa60f endp

Griffin_Handler_140daa908 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140daa908 endp

Griffin_Handler_140db124a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140db124a endp

Griffin_Handler_140db3c78 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140db3c78 endp

Griffin_Handler_140dd91f9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140dd91f9 endp

Griffin_Handler_140ddcace proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ddcace endp

Griffin_Handler_140dddd08 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140dddd08 endp

Griffin_Handler_140de8476 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140de8476 endp

Griffin_Handler_140dfcd12 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140dfcd12 endp

Griffin_Handler_140e04d69 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e04d69 endp

Griffin_Handler_140e09b9d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e09b9d endp

Griffin_Handler_140e2725c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e2725c endp

Griffin_Handler_140e36d9a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e36d9a endp

Griffin_Handler_140e471ec proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e471ec endp

Griffin_Handler_140e4fe17 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e4fe17 endp

Griffin_Handler_140e66f1d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e66f1d endp

Griffin_Handler_140e7b425 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e7b425 endp

Griffin_Handler_140e8f433 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e8f433 endp

Griffin_Handler_140e92b83 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e92b83 endp

Griffin_Handler_140e9d776 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140e9d776 endp

Griffin_Handler_140ea95a0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ea95a0 endp

Griffin_Handler_140eaf6f6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140eaf6f6 endp

Griffin_Handler_140eb907a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140eb907a endp

Griffin_Handler_140ec2037 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ec2037 endp

Griffin_Handler_140ecf02f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ecf02f endp

Griffin_Handler_140ed2e47 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ed2e47 endp

Griffin_Handler_140ed61ba proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ed61ba endp

Griffin_Handler_140ee810b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140ee810b endp

Griffin_Handler_140eeb34c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140eeb34c endp

Griffin_Handler_140f112d3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f112d3 endp

Griffin_Handler_140f1968f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f1968f endp

Griffin_Handler_140f28316 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f28316 endp

Griffin_Handler_140f3f9eb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f3f9eb endp

Griffin_Handler_140f4607b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f4607b endp

Griffin_Handler_140f492f5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f492f5 endp

Griffin_Handler_140f86de5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140f86de5 endp

Griffin_Handler_140fa2bd1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140fa2bd1 endp

Griffin_Handler_140fa6301 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140fa6301 endp

Griffin_Handler_140fad56f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140fad56f endp

Griffin_Handler_140fd9007 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140fd9007 endp

Griffin_Handler_140fedaed proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_140fedaed endp

Griffin_Handler_141006891 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141006891 endp

Griffin_Handler_141015078 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141015078 endp

Griffin_Handler_141032f8a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141032f8a endp

Griffin_Handler_141037d20 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141037d20 endp

Griffin_Handler_14104b430 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14104b430 endp

Griffin_Handler_14104fa0d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14104fa0d endp

Griffin_Handler_141055fef proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141055fef endp

Griffin_Handler_14105718b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14105718b endp

Griffin_Handler_14105918c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14105918c endp

Griffin_Handler_141085486 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141085486 endp

Griffin_Handler_141086881 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141086881 endp

Griffin_Handler_14108737c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14108737c endp

Griffin_Handler_1410a6a06 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410a6a06 endp

Griffin_Handler_1410bc26d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410bc26d endp

Griffin_Handler_1410d76ff proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410d76ff endp

Griffin_Handler_1410e539b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410e539b endp

Griffin_Handler_1410e772f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410e772f endp

Griffin_Handler_1410f08a6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410f08a6 endp

Griffin_Handler_1410f6ab7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1410f6ab7 endp

Griffin_Handler_1411200fb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411200fb endp

Griffin_Handler_14112409f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14112409f endp

Griffin_Handler_141138ae0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141138ae0 endp

Griffin_Handler_141147328 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141147328 endp

Griffin_Handler_1411519de proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411519de endp

Griffin_Handler_141158566 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141158566 endp

Griffin_Handler_1411588b7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411588b7 endp

Griffin_Handler_141167510 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141167510 endp

Griffin_Handler_141199aba proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141199aba endp

Griffin_Handler_14119afa9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14119afa9 endp

Griffin_Handler_14119d241 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14119d241 endp

Griffin_Handler_1411c1b7a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411c1b7a endp

Griffin_Handler_1411c7acd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411c7acd endp

Griffin_Handler_1411e010f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411e010f endp

Griffin_Handler_1411e392a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411e392a endp

Griffin_Handler_1411eb7af proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1411eb7af endp

Griffin_Handler_14120f21a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14120f21a endp

Griffin_Handler_1412118dc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412118dc endp

Griffin_Handler_14121658f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14121658f endp

Griffin_Handler_141231857 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141231857 endp

Griffin_Handler_141234c34 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141234c34 endp

Griffin_Handler_141246cdb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141246cdb endp

Griffin_Handler_1412527ee proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412527ee endp

Griffin_Handler_141254a86 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141254a86 endp

Griffin_Handler_1412564f0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412564f0 endp

Griffin_Handler_14127321d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14127321d endp

Griffin_Handler_14127bde1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14127bde1 endp

Griffin_Handler_1412a1c18 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412a1c18 endp

Griffin_Handler_1412d4e9d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412d4e9d endp

Griffin_Handler_1412e1aa4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412e1aa4 endp

Griffin_Handler_1412e5852 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412e5852 endp

Griffin_Handler_1412f4f50 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1412f4f50 endp

Griffin_Handler_141315ad8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141315ad8 endp

Griffin_Handler_14131b794 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14131b794 endp

Griffin_Handler_14132568a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14132568a endp

Griffin_Handler_141328267 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141328267 endp

Griffin_Handler_141338e1f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141338e1f endp

Griffin_Handler_14134322a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14134322a endp

Griffin_Handler_14138e429 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14138e429 endp

Griffin_Handler_1413b2560 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413b2560 endp

Griffin_Handler_1413d084c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413d084c endp

Griffin_Handler_1413e0496 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413e0496 endp

Griffin_Handler_1413f37fa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413f37fa endp

Griffin_Handler_1413f734f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413f734f endp

Griffin_Handler_1413f97c9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1413f97c9 endp

Griffin_Handler_141402c52 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141402c52 endp

Griffin_Handler_141405b9b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141405b9b endp

Griffin_Handler_141408823 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141408823 endp

Griffin_Handler_14140f375 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14140f375 endp

Griffin_Handler_14142d87f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14142d87f endp

Griffin_Handler_1414339e7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414339e7 endp

Griffin_Handler_141434bb2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141434bb2 endp

Griffin_Handler_141454e0d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141454e0d endp

Griffin_Handler_141464658 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141464658 endp

Griffin_Handler_1414775e7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414775e7 endp

Griffin_Handler_14149f78a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_14149f78a endp

Griffin_Handler_1414b1727 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414b1727 endp

Griffin_Handler_1414b8b56 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414b8b56 endp

Griffin_Handler_1414b93df proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414b93df endp

Griffin_Handler_1414df65c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414df65c endp

Griffin_Handler_1414f3895 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1414f3895 endp

Griffin_Handler_141512267 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141512267 endp

Griffin_Handler_1415b94e4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1415b94e4 endp

Griffin_Handler_141783e09 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_141783e09 endp

Griffin_Handler_1418083c5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_1418083c5 endp

; ============================================================================
; Griffin User-Mode Anti-Tamper VM Devirtualization Summary:
;   Total VM Handlers Recovered: 240
;   Memory Loads via Frame:      6694
;   Memory Stores to Frame:      5818
;   Arithmetic Transformations:  11838
;   Branch Operations:           143997
; ============================================================================

end
