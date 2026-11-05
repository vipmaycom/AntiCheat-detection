; ============================================================================
; Vanguard User-Mode Service Griffin VM Reconstructed & Devirtualized Assembly
; Target: C:\vgc.exe (107.5 MB) | Section: .grfn1 (92.07 MB, RVA 0x5EB000)
; SHA-256: c8f330a7d5fae656d0d2ff86906f97ef7be05650130d220f12443d3fb49d5bf1
; EntryPoint: 0x564700000 + 0x5F7C13C (.riot1) -> Dispatcher: 0x5DBB000 (.riot0)
; Griffin Virtual Context Frame: 560 bytes (0x230)
; ============================================================================

.code

; --- Griffin EntryPoint Unpacker Stub (RVA 0x5F7C13C) ---
Griffin_EntryPoint proc
    push    rbx
    push    rcx
    push    rdx
    push    r8
    push    r9
    sub     rsp, 20h
    lea     rbx, [rip - 8Eh]        ; Descriptor structure at 0x5F7C0B0
    mov     r8, rbx
    call    Griffin_InitUnpacker    ; 0x5F73E27 -> Decrypts .riot0/.grfn1 tables
    add     rsp, 20h
    pop     r9
    pop     r8
    pop     rdx
    pop     rcx
    test    eax, eax
    jz      Griffin_InitFail
    cmp     eax, 0D4494E49h         ; 'INI\xd4' Magic Verification
    je      Griffin_LaunchDirect
    movsxd  rax, dword ptr [rbx+28h]; Relative Displacement -> .riot0 (0x5DBB000)
    lea     rax, [rax+rbx]          ; rax = 0x5DBB000
    mov     rax, [rax+10h]          ; Resolved Target WinMain / ServiceMain
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

Griffin_Handler_564cfff72 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564cfff72 endp

Griffin_Handler_564d13da2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d13da2 endp

Griffin_Handler_564d1998b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d1998b endp

Griffin_Handler_564d4392a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d4392a endp

Griffin_Handler_564d44b7e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d44b7e endp

Griffin_Handler_564d667fc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d667fc endp

Griffin_Handler_564d6bdb0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d6bdb0 endp

Griffin_Handler_564d7f851 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d7f851 endp

Griffin_Handler_564d84daa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d84daa endp

Griffin_Handler_564d920bc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564d920bc endp

Griffin_Handler_564dbbd9f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564dbbd9f endp

Griffin_Handler_564dd10e4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564dd10e4 endp

Griffin_Handler_564dd1876 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564dd1876 endp

Griffin_Handler_564ddfd8f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564ddfd8f endp

Griffin_Handler_564df6bc8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564df6bc8 endp

Griffin_Handler_564dfc105 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564dfc105 endp

Griffin_Handler_564dfdaf6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564dfdaf6 endp

Griffin_Handler_564e0da32 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e0da32 endp

Griffin_Handler_564e12342 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e12342 endp

Griffin_Handler_564e35e10 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e35e10 endp

Griffin_Handler_564e5e381 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e5e381 endp

Griffin_Handler_564e66755 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e66755 endp

Griffin_Handler_564e733b1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e733b1 endp

Griffin_Handler_564e904cd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e904cd endp

Griffin_Handler_564e9b0ad proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e9b0ad endp

Griffin_Handler_564e9f117 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564e9f117 endp

Griffin_Handler_564ebfa6e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564ebfa6e endp

Griffin_Handler_564ec3ec6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564ec3ec6 endp

Griffin_Handler_564ee6b7c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564ee6b7c endp

Griffin_Handler_564efa0aa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564efa0aa endp

Griffin_Handler_564f0fc19 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f0fc19 endp

Griffin_Handler_564f1917a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f1917a endp

Griffin_Handler_564f22b28 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f22b28 endp

Griffin_Handler_564f23ca0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f23ca0 endp

Griffin_Handler_564f3d010 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f3d010 endp

Griffin_Handler_564f4e5fe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f4e5fe endp

Griffin_Handler_564f5ea71 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564f5ea71 endp

Griffin_Handler_564fa11b7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564fa11b7 endp

Griffin_Handler_564fec886 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564fec886 endp

Griffin_Handler_564ff44dd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_564ff44dd endp

Griffin_Handler_565004ae1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565004ae1 endp

Griffin_Handler_56501b40c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56501b40c endp

Griffin_Handler_5650462e6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650462e6 endp

Griffin_Handler_565077d2b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565077d2b endp

Griffin_Handler_5650786a5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650786a5 endp

Griffin_Handler_565078c95 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565078c95 endp

Griffin_Handler_565081db5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565081db5 endp

Griffin_Handler_5650a64a7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650a64a7 endp

Griffin_Handler_5650ab2f6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650ab2f6 endp

Griffin_Handler_5650be470 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650be470 endp

Griffin_Handler_5650e70e7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5650e70e7 endp

Griffin_Handler_56512bf13 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56512bf13 endp

Griffin_Handler_56515eb58 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56515eb58 endp

Griffin_Handler_565171430 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565171430 endp

Griffin_Handler_56518e00b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56518e00b endp

Griffin_Handler_565194ffe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565194ffe endp

Griffin_Handler_5651a5ca9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651a5ca9 endp

Griffin_Handler_5651c7089 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651c7089 endp

Griffin_Handler_5651d4094 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651d4094 endp

Griffin_Handler_5651dd7f3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651dd7f3 endp

Griffin_Handler_5651e20aa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651e20aa endp

Griffin_Handler_5651ff8d6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5651ff8d6 endp

Griffin_Handler_56520288e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56520288e endp

Griffin_Handler_5652247da proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652247da endp

Griffin_Handler_56523f718 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56523f718 endp

Griffin_Handler_5652adc72 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652adc72 endp

Griffin_Handler_5652af4d8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652af4d8 endp

Griffin_Handler_5652d4d03 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652d4d03 endp

Griffin_Handler_5652d723e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652d723e endp

Griffin_Handler_5652e3753 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652e3753 endp

Griffin_Handler_5652f88c2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5652f88c2 endp

Griffin_Handler_5653002db proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653002db endp

Griffin_Handler_565308f50 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565308f50 endp

Griffin_Handler_565318326 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565318326 endp

Griffin_Handler_56531a78a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56531a78a endp

Griffin_Handler_56531beec proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56531beec endp

Griffin_Handler_56533a559 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56533a559 endp

Griffin_Handler_56534ea65 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56534ea65 endp

Griffin_Handler_56534f035 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56534f035 endp

Griffin_Handler_56535aded proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56535aded endp

Griffin_Handler_56535f9e3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56535f9e3 endp

Griffin_Handler_565375274 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565375274 endp

Griffin_Handler_5653795ee proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653795ee endp

Griffin_Handler_565387edb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565387edb endp

Griffin_Handler_5653bd710 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653bd710 endp

Griffin_Handler_5653d57e6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653d57e6 endp

Griffin_Handler_5653ee655 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653ee655 endp

Griffin_Handler_5653f60c1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653f60c1 endp

Griffin_Handler_5653f8ab3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5653f8ab3 endp

Griffin_Handler_5654475e9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654475e9 endp

Griffin_Handler_56546c8e5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56546c8e5 endp

Griffin_Handler_56546e4c6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56546e4c6 endp

Griffin_Handler_5654762fb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654762fb endp

Griffin_Handler_56547a7c5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56547a7c5 endp

Griffin_Handler_56547e6c6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56547e6c6 endp

Griffin_Handler_56549c0b4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56549c0b4 endp

Griffin_Handler_5654a7ac2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654a7ac2 endp

Griffin_Handler_5654a876a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654a876a endp

Griffin_Handler_5654b7f9b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654b7f9b endp

Griffin_Handler_5654caa6a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654caa6a endp

Griffin_Handler_5654e91f2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5654e91f2 endp

Griffin_Handler_565529ad7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565529ad7 endp

Griffin_Handler_56552eaea proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56552eaea endp

Griffin_Handler_565535f81 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565535f81 endp

Griffin_Handler_5655390b5 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655390b5 endp

Griffin_Handler_56553928c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56553928c endp

Griffin_Handler_5655431aa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655431aa endp

Griffin_Handler_565560363 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565560363 endp

Griffin_Handler_5655a5e89 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655a5e89 endp

Griffin_Handler_5655ad78b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655ad78b endp

Griffin_Handler_5655be062 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655be062 endp

Griffin_Handler_5655f9d8c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5655f9d8c endp

Griffin_Handler_565602ee2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565602ee2 endp

Griffin_Handler_56562234f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56562234f endp

Griffin_Handler_565641945 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565641945 endp

Griffin_Handler_56564414a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56564414a endp

Griffin_Handler_56564ad75 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56564ad75 endp

Griffin_Handler_56565950b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56565950b endp

Griffin_Handler_56566d14d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56566d14d endp

Griffin_Handler_56568cbec proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56568cbec endp

Griffin_Handler_56569a7c8 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56569a7c8 endp

Griffin_Handler_5656b484a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5656b484a endp

Griffin_Handler_5656b5b80 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5656b5b80 endp

Griffin_Handler_5656e078b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5656e078b endp

Griffin_Handler_565708c60 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565708c60 endp

Griffin_Handler_56570aa6e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56570aa6e endp

Griffin_Handler_56570e31d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56570e31d endp

Griffin_Handler_565717e7f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565717e7f endp

Griffin_Handler_56572b856 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56572b856 endp

Griffin_Handler_5657526a6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5657526a6 endp

Griffin_Handler_565756bfd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565756bfd endp

Griffin_Handler_565766812 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565766812 endp

Griffin_Handler_565767c4c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565767c4c endp

Griffin_Handler_56576c194 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56576c194 endp

Griffin_Handler_56576de00 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56576de00 endp

Griffin_Handler_56576f9db proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56576f9db endp

Griffin_Handler_565789ce0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565789ce0 endp

Griffin_Handler_5657946c9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5657946c9 endp

Griffin_Handler_565798b68 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565798b68 endp

Griffin_Handler_5657bfd0f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5657bfd0f endp

Griffin_Handler_5657e2ddd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5657e2ddd endp

Griffin_Handler_565812c49 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565812c49 endp

Griffin_Handler_56581b1fe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56581b1fe endp

Griffin_Handler_56581bc90 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56581bc90 endp

Griffin_Handler_56581ef7b proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56581ef7b endp

Griffin_Handler_5658268fe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658268fe endp

Griffin_Handler_56583bef9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56583bef9 endp

Griffin_Handler_5658404f1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658404f1 endp

Griffin_Handler_56586518a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56586518a endp

Griffin_Handler_56586a697 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56586a697 endp

Griffin_Handler_56586c8cf proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56586c8cf endp

Griffin_Handler_565873fb9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565873fb9 endp

Griffin_Handler_565896f20 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565896f20 endp

Griffin_Handler_5658a364e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658a364e endp

Griffin_Handler_5658acefe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658acefe endp

Griffin_Handler_5658c5b27 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658c5b27 endp

Griffin_Handler_5658c9f95 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658c9f95 endp

Griffin_Handler_5658cea47 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658cea47 endp

Griffin_Handler_5658db7fc proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658db7fc endp

Griffin_Handler_5658df21d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658df21d endp

Griffin_Handler_5658e86ca proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658e86ca endp

Griffin_Handler_5658ecdab proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658ecdab endp

Griffin_Handler_5658f3560 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658f3560 endp

Griffin_Handler_5658fce74 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5658fce74 endp

Griffin_Handler_5659090bb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659090bb endp

Griffin_Handler_565913f32 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565913f32 endp

Griffin_Handler_5659323a3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659323a3 endp

Griffin_Handler_56593b606 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56593b606 endp

Griffin_Handler_56593da0f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56593da0f endp

Griffin_Handler_56594ac57 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56594ac57 endp

Griffin_Handler_565953d5a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565953d5a endp

Griffin_Handler_56596d6d6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56596d6d6 endp

Griffin_Handler_565983b47 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565983b47 endp

Griffin_Handler_56598e52c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56598e52c endp

Griffin_Handler_5659a6306 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659a6306 endp

Griffin_Handler_5659b1eb6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659b1eb6 endp

Griffin_Handler_5659b7c72 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659b7c72 endp

Griffin_Handler_5659d53fe proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5659d53fe endp

Griffin_Handler_565a091ea proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a091ea endp

Griffin_Handler_565a0def1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a0def1 endp

Griffin_Handler_565a412ce proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a412ce endp

Griffin_Handler_565a43dbb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a43dbb endp

Griffin_Handler_565a4f9df proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a4f9df endp

Griffin_Handler_565a665b1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a665b1 endp

Griffin_Handler_565a66909 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a66909 endp

Griffin_Handler_565a6dc3a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565a6dc3a endp

Griffin_Handler_565aa7c04 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565aa7c04 endp

Griffin_Handler_565b31fb2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565b31fb2 endp

Griffin_Handler_565b45a88 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565b45a88 endp

Griffin_Handler_565b5bb1a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565b5bb1a endp

Griffin_Handler_565b65163 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565b65163 endp

Griffin_Handler_565b890e3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565b890e3 endp

Griffin_Handler_565ba1391 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565ba1391 endp

Griffin_Handler_565baec20 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565baec20 endp

Griffin_Handler_565bd4cab proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565bd4cab endp

Griffin_Handler_565bddb9a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565bddb9a endp

Griffin_Handler_565c17614 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c17614 endp

Griffin_Handler_565c29ef1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c29ef1 endp

Griffin_Handler_565c493d7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c493d7 endp

Griffin_Handler_565c493f6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c493f6 endp

Griffin_Handler_565c5eedb proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c5eedb endp

Griffin_Handler_565c6d259 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c6d259 endp

Griffin_Handler_565c7b74c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c7b74c endp

Griffin_Handler_565c86f0f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c86f0f endp

Griffin_Handler_565c9946f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565c9946f endp

Griffin_Handler_565cb0c0f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cb0c0f endp

Griffin_Handler_565cb2fb2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cb2fb2 endp

Griffin_Handler_565cbb015 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cbb015 endp

Griffin_Handler_565cc05e1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cc05e1 endp

Griffin_Handler_565cc920d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cc920d endp

Griffin_Handler_565cd702f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565cd702f endp

Griffin_Handler_565ed9a2c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_565ed9a2c endp

Griffin_Handler_566008bf7 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566008bf7 endp

Griffin_Handler_566093289 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566093289 endp

Griffin_Handler_5660a633c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5660a633c endp

Griffin_Handler_5661516e9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5661516e9 endp

Griffin_Handler_56632989c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56632989c endp

Griffin_Handler_5667e9036 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5667e9036 endp

Griffin_Handler_566b1fd0f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566b1fd0f endp

Griffin_Handler_566d32fc4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566d32fc4 endp

Griffin_Handler_566d358dd proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566d358dd endp

Griffin_Handler_566d7478d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566d7478d endp

Griffin_Handler_566dc0d12 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_566dc0d12 endp

Griffin_Handler_5670748e6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5670748e6 endp

Griffin_Handler_56709cce4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56709cce4 endp

Griffin_Handler_56718fa7d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56718fa7d endp

Griffin_Handler_567227b13 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567227b13 endp

Griffin_Handler_567471688 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567471688 endp

Griffin_Handler_56750d5b1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56750d5b1 endp

Griffin_Handler_56750d601 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56750d601 endp

Griffin_Handler_56750d629 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56750d629 endp

Griffin_Handler_56750d651 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56750d651 endp

Griffin_Handler_567564e18 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567564e18 endp

Griffin_Handler_5675e8776 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5675e8776 endp

Griffin_Handler_5676361ca proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5676361ca endp

Griffin_Handler_56772bcfa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56772bcfa endp

Griffin_Handler_56788b7ec proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56788b7ec endp

Griffin_Handler_5679e74c9 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5679e74c9 endp

Griffin_Handler_567c728b6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567c728b6 endp

Griffin_Handler_567c7672d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567c7672d endp

Griffin_Handler_567d70fb2 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567d70fb2 endp

Griffin_Handler_567d7fe00 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567d7fe00 endp

Griffin_Handler_567e65385 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567e65385 endp

Griffin_Handler_567f652f0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_567f652f0 endp

Griffin_Handler_56801129d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56801129d endp

Griffin_Handler_568162f26 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_568162f26 endp

Griffin_Handler_56818a8c4 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56818a8c4 endp

Griffin_Handler_568301f4a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_568301f4a endp

Griffin_Handler_5684e2b90 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5684e2b90 endp

Griffin_Handler_5686bc59d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5686bc59d endp

Griffin_Handler_56887054a proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56887054a endp

Griffin_Handler_5689975df proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5689975df endp

Griffin_Handler_568d0b274 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_568d0b274 endp

Griffin_Handler_568df507d proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_568df507d endp

Griffin_Handler_568f0e5b0 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_568f0e5b0 endp

Griffin_Handler_569212e01 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569212e01 endp

Griffin_Handler_5692e2f62 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5692e2f62 endp

Griffin_Handler_56963f62f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56963f62f endp

Griffin_Handler_5697b9556 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_5697b9556 endp

Griffin_Handler_569b18946 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569b18946 endp

Griffin_Handler_569b8dde6 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569b8dde6 endp

Griffin_Handler_569bae0ea proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569bae0ea endp

Griffin_Handler_569bc3cd1 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569bc3cd1 endp

Griffin_Handler_569d9587e proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569d9587e endp

Griffin_Handler_569e7614c proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569e7614c endp

Griffin_Handler_569ecccab proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_569ecccab endp

Griffin_Handler_56a16ecfa proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56a16ecfa endp

Griffin_Handler_56a22e06f proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56a22e06f endp

Griffin_Handler_56a294c04 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56a294c04 endp

Griffin_Handler_56a3ac0b3 proc
    lea     rsp, [rsp + 230h]   ; Restores 560-byte Griffin virtual execution frame
    popfq                       ; Restores host RFLAGS
    ret
Griffin_Handler_56a3ac0b3 endp

; ============================================================================
; Griffin User-Mode VM Devirtualization Summary:
;   Total VM Handlers Recovered: 270
;   Memory Loads via Frame:      285042
;   Memory Stores to Frame:      248774
;   Arithmetic Transformations:  403735
;   Branch Operations:           410032
; ============================================================================

end
