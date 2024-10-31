#start=rocket.exe#

#make_bin#

name "rocket"

COMMAND_PORT equ 10
STATUS_PORT  equ 11

; Write 69 into status port (11) to specify that emulator is ready
; to the virtual device. Device waits to see the status number
mov al, 45h
out STATUS_PORT, al

mov dx, offset helper_msg
call print_msg

; w = 77h
; s = 73h
; d = 64h
; a = 61h
main:
    ; Interrupt to scan a code from keyboard (wasd)
    mov ah, 0
    int 16h
    
    ; Character code saved in al. Save in stack before
    ; calling wait_until_ready which needs al
    push ax
    call wait_until_ready
    pop ax
    
    cmp al, 77h
    jz increase_thrust
    cmp al, 73h
    jz decrease_thrust
    cmp al, 64h
    jz rotate_right
    cmp al, 61h
    jz rotate_left
    
    ; If any other character pressed then exit
    jmp end
    
    
jmp main
jmp end


increase_thrust:
    mov al, 1h
    out COMMAND_PORT, al
    mov dx, offset thrust_msg
    call print_msg
    jmp main
    

decrease_thrust:
    mov al, 2h
    out COMMAND_PORT, al
    mov dx, offset dethrust_msg
    call print_msg
    jmp main

rotate_right:
    mov al, 3h
    out COMMAND_PORT, al
    mov dx, offset rotate_right_msg
    call print_msg
    jmp main

rotate_left:
    mov al, 4h
    out COMMAND_PORT, al
    mov dx, offset rotate_left_msg
    call print_msg
    jmp main
             
             
wait_until_ready proc
busy: in al, STATUS_PORT
      cmp al, 64h ; 100
      jnz busy ; busy, so wait.
ret    
wait_until_ready endp

; Need to set start offset to msg in dx before calling print_msg
; mov dx, offset msg
print_msg proc
mov ah, 9
int 21h
ret
print_msg endp

helper_msg db       "Use WASD keys for controlling the rocket. Press any other key to exit!", 0dh,0ah, "$"
thrust_msg db       "Pressed (w) -> Increasing thrust", 0Dh,0Ah, "$"
dethrust_msg db     "Pressed (s) -> Decreasing thrust", 0Dh,0Ah, "$"
rotate_right_msg db "Pressed (d) -> Rotating right",    0Dh,0Ah, "$"
rotate_left_msg db  "Pressed (a) -> Rotating left",     0Dh,0Ah, "$"
exit_msg db         "Shutting down device. Exiting",    0Dh,0Ah, "$"

end:
; Perform de-initialization of virtual device properly
mov dx, offset exit_msg
call print_msg
mov al, 5h ; 5 is Command::EXIT
out COMMAND_PORT, al
hlt
