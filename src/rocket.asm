#start=rocket.exe#

#make_bin#

name "rocket"

; Port value constants
COMMAND_PORT equ 10
STATUS_PORT  equ 11

; Write 69 into status port (11) to specify that emulator is ready
; to the virtual device. Device waits to see the status number
mov al, 45h
out STATUS_PORT, al

; Print control message
mov dx, offset helper_msg
call print_msg

; w = 77h s = 73h d = 64h a = 61h
; Main function of the program (looped infinitely otherwise stated)
main:
    ; Interrupt to scan a code from keyboard (wasd)
    mov ah, 0
    int 16h
    
    ; Character code saved in al. Save in stack before
    ; calling wait_until_ready which needs `al` register
    push ax
    ; Device will set the COMMAND_READY when it's finished executing the last frame and ready 
    ; to accept new commands for the new frame. Not an huge issue, but in case a frame takes longer
    ; time to render we will wait so that the command is disregarded.
    call wait_until_ready
    pop ax
     
    ; Jump to correct sub-routine based on key (if-conditonals) 
    cmp al, 77h ; Pressed `w`
    jz increase_thrust
    cmp al, 73h ; Pressed `s`
    jz decrease_thrust
    cmp al, 64h ; Pressed `d`
    jz rotate_right
    cmp al, 61h ; Pressed `a`
    jz rotate_left
    
    ; If any other character pressed then exit!
    jmp end
    
    
jmp main
jmp end

; Send 0x1 from COMMAND_PORT to increase acceleration
increase_thrust:
    mov al, 1h
    out COMMAND_PORT, al
    mov dx, offset thrust_msg
    call print_msg
    jmp main
    
; Send 0x2 from COMMAND_PORT to decrease acceleration
decrease_thrust:
    mov al, 2h
    out COMMAND_PORT, al
    mov dx, offset dethrust_msg
    call print_msg
    jmp main

; Send 0x3 from COMMAND_PORT to rotate right
rotate_right:
    mov al, 3h
    out COMMAND_PORT, al
    mov dx, offset rotate_right_msg
    call print_msg
    jmp main

; Send 0x4 from COMMAND_PORT to rotate left
rotate_left:
    mov al, 4h
    out COMMAND_PORT, al
    mov dx, offset rotate_left_msg
    call print_msg
    jmp main
             
             
wait_until_ready proc
busy: in al, STATUS_PORT
      cmp al, 64h ; 100
      jnz busy    ; busy, so wait.
ret    
wait_until_ready endp

; Need to set start offset to msg in dx before calling print_msg
; mov dx, offset msg
print_msg proc
mov ah, 9
int 21h
ret
print_msg endp

; Strings
helper_msg db       "Use WASD keys for controlling the rocket. Press any other key to exit!", 0dh,0ah, "$"
thrust_msg db       "Pressed (w) -> Increasing thrust", 0Dh,0Ah, "$"
dethrust_msg db     "Pressed (s) -> Decreasing thrust", 0Dh,0Ah, "$"
rotate_right_msg db "Pressed (d) -> Rotating right",    0Dh,0Ah, "$"
rotate_left_msg db  "Pressed (a) -> Rotating left",     0Dh,0Ah, "$"
exit_msg db         "Shutting down device. Exiting!",   0Dh,0Ah, "$"


; End procedure. Perform de-initialization of virtual device properly
end:
mov dx, offset exit_msg
call print_msg
mov al, 5h ; 0x5 is Command::EXIT
out COMMAND_PORT, al
hlt
