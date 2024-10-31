#start=rocket.exe#

#make_bin#

name "rocket"


; Write 69 into status port (11) to specify that emulator is ready
; to the virtual device
mov al, 45h
out 11, al


call wait_until_ready


increase_thrust proc
mov al, 1h
out 10, al
ret   
increase_thurst endp

decrease_thrust proc
mov al, 2h
out 10, al
ret   
decrease_thurst endp


wait_until_ready proc
busy: in al, 11
      cmp al, 64h
      jnz busy ; busy, so wait.
ret    
wait_until_ready endp



hlt


