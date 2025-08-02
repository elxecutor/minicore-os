; task_switch.asm - Context switching for cooperative multitasking

section .text
global task_switch

; void task_switch(uint32_t* old_esp, uint32_t new_esp)
; Simple task switching - saves and restores basic CPU state
task_switch:
    push ebp
    mov ebp, esp
    
    ; Save current task's registers
    pushf               ; Save flags
    pusha               ; Save all general purpose registers
    
    ; Get parameters: old_esp and new_esp
    mov eax, [ebp+8]    ; old_esp pointer
    mov edx, [ebp+12]   ; new_esp value
    
    ; Save current ESP to old task's ESP storage
    mov [eax], esp
    
    ; Switch to new task's stack
    mov esp, edx
    
    ; Restore new task's registers
    popa                ; Restore all general purpose registers
    popf                ; Restore flags
    
    pop ebp
    ret
