
# mp1.S - missile-command version
# Good luck, and have fun!

.data
    
    # Constants for accessing the fields of a struct missile,
    # struct missile is defined in rtc.h and mp1.h

    NEXT       = 0
    X      = 4
    Y      = 8
    VX      = 12
    VY      = 16
    DEST_X      = 20
    DEST_Y      = 24
    EXPLODED  = 28
    C      = 32

    # Character to draw for an explosion - '@'

    EXPLOSION = 64


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:    .long 0x0    # Head of list pointer
base_alive:        .long 0x0    # Array of 3 bytes, plus a padding byte
mp1_score:        .long 0x0    # Player's current score


# Data private to this  file

base_pic:             .string "/^^^\\"    # Picture of a live base
dead_base_pic:         .string "xxxxx"    # Picture of a dead base
crosshairs_x:        .long 0x0    # X-position of the crosshairs
crosshairs_y:        .long 0x0    # Y-position of the crosshairs

.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
        movl    vmem_base_addr(,1),%edx
        movb    %cl,(%edx,%eax,1)
        ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#    (1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#    (2) Redraw the bases - they may have been overwritten with missiles
#    (3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
        pushl %ebp
        movl %esp, %ebp
        
        # update_missiles - walking down the linked list to udpdate user state
        call update_missiles

        leave
        ret

# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:

        # esp+4: unsigned long arg
        # esp+8: unsigned int cmd

        # -1 - return value for failure
        movl $-1, %eax

        # esp+8 - cmd
        # cmd 0 - startgame
        cmpl $0, 8(%esp)
        je call_start
        # cmd 1 - addmissiles
        cmpl $1, 8(%esp)
        je call_add
        # cmd 2 - movexhairs
        cmpl $2, 8(%esp)
        je call_move
        # cmd 3 - getstatus
        cmpl $3, 8(%esp)
        je call_status
        # cmd 4 - endgame
        cmpl $4, 8(%esp)
        je call_end

        # other cmds - do nothing and return -1
        jmp done_leave

        # 0 cmd - jump to startgame
        call_start:
        jmp *jump_table
        # return 0 for vaild cmd
        movl $0, %eax
        jmp done_leave

        # 1 cmd - jump to addmissile
        call_add:
        jmp *jump_table+4
        # return 0 for vaild cmd
        movl $0, %eax
        jmp done_leave

        # 2 cmd - jump to movexhairs
        call_move:
        jmp *jump_table+8
        # return 0 for vaild cmd
        movl $0, %eax
        jmp done_leave

        # 3 cmd - jump to status
        call_status:
        jmp *jump_table+12
        # return 0 for vaild cmd
        movl $0, %eax
        jmp done_leave

        # 4 cmd - jump to endgame
        call_end:
        jmp *jump_table+16
        # return 0 for vaild cmd
        movl $0, %eax
        jmp done_leave

        # the end of mp1_ioctl
        done_leave:
        ret


# ----------------- Functions private to this file -------------------

# void mp1_missile_remove(struct missile* missile_to_be_removed);
# This function walks down the linked list to find the missile to be removed.
#
#   Inputs: %edi - The pointer to the missile to be removed
#   Outputs: None
#
# Registers: Standard C calling convention

mp1_missile_remove:

        # ebx - NEXT field (current missile)
        # ecx - current missile
        # edi - target missile (to be removed)
        # esi - NEXT field (removed missile)

        pushl %ebp
        movl %esp, %ebp

        # save registers to be used
        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # ebp+8 - arg (pointer to the missile to be removed)
        # edi - target missile (to be removed)
        movl 8(%ebp), %edi
        cmpl $0, %edi
        je done_remove
        movl mp1_missile_list, %ecx
        cmpl $0, %ecx
        je done_remove
        
        # remove head missile?
        cmpl %edi, %ecx
        je remove_head

        # walk the linked list to find which missile to remove
        loop_find:

        # ebx - NEXT field of current missile
        cmpl $0, %ecx
        je done_remove
        movl (%ecx), %ebx
        cmpl %edi, %ebx

        # found
        je to_free
        movl %ebx, %ecx
        je done_remove
        jmp loop_find

        to_free:
        movl (%ebx), %esi
        movl %esi, (%ecx)

        # free
        cmpl $0, %edi
        je  done_remove
        pushl %edi
        call mp1_free
        popl %edi
        jmp done_remove

        # remove head missile
        # ecx - previous head (to be removed)
        # ebx - current head / head to be removed
        remove_head:

        # head is NULL?
        cmpl $0, %ecx
        je done_remove
        movl (%ecx), %ebx
        movl %ebx, mp1_missile_list

        # free the removed missile (previous head)
        pushl %ecx
        call mp1_free
        popl %ecx

        done_remove:
        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax

        leave
        ret

# void mp1_draw_other(void);
# This function redraws city bases and crosshairs.
#
#   Inputs: None
#   Outputs: None
#
# Registers: Standard C calling convention

mp1_draw_other:
        pushl %ebp
        movl %esp, %ebp

        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # draw cities
        # esi - city_1
        # ecx - city_2
        # edx - city_3

        movl base_alive, %esi
        movl base_alive, %ecx
        movl base_alive, %edx

        andl $0x000000ff, %esi
        andl $0x0000ff00, %ecx
        andl $0x00ff0000, %edx

        cmpl $0, %esi
        # city1 dead?
        je d1

        # city_1 alive
        # 47 - "/"
        # 3876 - offset at (18, 24), 80 columns, 2 bytes of each character
        movb $47, %cl
        movl $3876, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 94 - "^"
        # 3878 - offset at (19, 24), 80 columns, 2 bytes of each character
        movb $94, %cl
        movl $3878, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3880 - offset at (20, 24), 80 columns, 2 bytes of each character
        movl $3880, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3882 - offset at (21, 24), 80 columns, 2 bytes of each character
        movl $3882, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 92 - "\"
        # 3884 - offset at (22, 24), 80 columns, 2 bytes of each character
        movb $92, %cl
        movl $3884, %eax
        pushl %edx
        call mp1_poke
        popl %edx
        
        # c2 - city2 drawing
        jmp c2

        # d1 - city1 dead
        d1:
        # 120 - "x"
        # 3876 - offset at (18, 24), 80 columns, 2 bytes of each character
        movb $120, %cl
        movl $3876, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3878 - offset at (19, 24), 80 columns, 2 bytes of each character
        movl $3878, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3880 - offset at (20, 24), 80 columns, 2 bytes of each character
        movl $3880, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3882 - offset at (21, 24), 80 columns, 2 bytes of each character
        movl $3882, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3884 - offset at (22, 24), 80 columns, 2 bytes of each character
        movl $3884, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # city2 drawing
        c2:
        movl base_alive, %ecx
        andl $0x0000ff00, %ecx
        cmpl $0, %ecx
        # d2 - city2 dead
        je d2

        # 47 - "/"
        # 3916 - offset at (38, 24), 80 columns, 2 bytes of each character
        movb $47, %cl
        movl $3916, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 94 - "^"
        # 3918 - offset at (39, 24), 80 columns, 2 bytes of each character
        movb $94, %cl
        movl $3918, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3920 - offset at (40, 24), 80 columns, 2 bytes of each character
        movl $3920, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3922 - offset at (41, 24), 80 columns, 2 bytes of each character
        movl $3922, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 92 - "\"
        # 3924 - offset at (42, 24), 80 columns, 2 bytes of each character
        movb $92, %cl
        movl $3924, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # c3 - city3 drawing
        jmp c3

        # city2 dead
        d2:
        # 120 - "x"
        # 3916 - offset at (38, 24), 80 columns, 2 bytes of each character
        movb $120, %cl
        movl $3916, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3918 - offset at (39, 24), 80 columns, 2 bytes of each character
        movl $3918, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3920 - offset at (40, 24), 80 columns, 2 bytes of each character
        movl $3920, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3922 - offset at (41, 24), 80 columns, 2 bytes of each character
        movl $3922, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # 3924 - offset at (42, 24), 80 columns, 2 bytes of each character
        movl $3924, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # city3 drawing
        # 47 - "/"
        # 94  - "^"
        # 92 - "\"
        # 120 - "x"
        # 3956 - offset at (58, 24), 80 columns, 2 bytes of each character
        # 3958 - offset at (59, 24), 80 columns, 2 bytes of each character
        # 3960 - offset at (60, 24), 80 columns, 2 bytes of each character
        # 3962 - offset at (61, 24), 80 columns, 2 bytes of each character
        # 3964 - offset at (62, 24), 80 columns, 2 bytes of each character

        c3:
        cmpl $0, %edx
        je d3

        movb $47, %cl
        movl $3956, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movb $94, %cl
        movl $3958, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3960, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3962, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movb $92, %cl
        movl $3964, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # d_cross - crosshairs drawing
        jmp d_cross
        
        # city3 dead
        d3:
        movb $120, %cl
        movl $3956, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3958, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3960, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3962, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        movl $3964, %eax
        pushl %edx
        call mp1_poke
        popl %edx

        # draw crosshairs
        # esi - crosshairs_x
        # ecx - crosshairs_y
        d_cross:
        
        movl crosshairs_x, %esi
        movl crosshairs_y, %ecx
        imull $160, %ecx
        imull $2, %esi
        addl %ecx, %esi
        movl %esi, %eax
        movb $43, %cl
        pushl %edx
        call mp1_poke
        popl %edx

        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax

        leave
        ret

# void update_missiles(void);
# This function updates the state of the game.
#
#   Inputs: None
#   Outputs: None
#
# Registers: Standard C calling convention

update_missiles:
        pushl %ebp
        movl %esp, %ebp

        pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # ebx: head of the linked list
        # ecx: high 16 of x or low 16 of x
        # edx: high 16 of y or low 16 of y
        # edi: dest_x or vx
        # esi: dest_y or explode or vy

        # head NULL, return
        movl mp1_missile_list, %ebx
        cmpl $0, %ebx
        je done

        # main loop to walk through the linked list
        # ebx: pointer to current missile
        # ecx: high 16 of x
        # edx: high 16 of y
        # edi: NEXT field of current missile (ebx)
        loop_update:
        movl (%ebx), %edi
        pushl %edi
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        andl $0xffff0000, %edx
        shrl $16, %ecx
        shrl $16, %edx
        cmpl $0, %ecx
        jl erase
        cmpl $79, %ecx
        jg erase
        cmpl $0, %edx
        jl erase
        cmpl $24, %edx
        jg erase
        jmp process2

        # delete a missile when out of range
        # ebx: pointer to current missile
        # ecx: high 16 of x
        # edx: high 16 of y
        erase:
        # draw " " (erase)
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        shrl $16, %ecx
        andl $0xffff0000, %edx
        shrl $16, %edx
        imull $160, %edx
        imull $2, %ecx
        addl %edx, %ecx
        movl %ecx, %eax
        movb $0x20, %cl
        pushl %edx
        call mp1_poke
        popl %edx

        # remove from the linked list and free the missile
        pushl %ebx
        call mp1_missile_remove
        popl %ebx
        jmp draw_other

        # missile in range, check exploded or destination reached
        process2:
        movl 20(%ebx), %edi
        cmpl %edi, %ecx
        je check_y
        jmp check_explode

        # check y destination reached
        check_y:
        movl 24(%ebx), %esi
        cmpl %esi, %edx
        je explode

        check_explode:
        movl 28(%ebx), %esi
        cmpl $0, %esi
        jg explode

        move:
        movl 12(%ebx), %edi
        movl 16(%ebx), %esi
        cmpl $0, %edi
        je check_vy
        jmp erase_redraw
        
        check_vy:
        cmpl $0, %esi
        je draw

        # erase current missile from old location
        # ebx - pointer to the current missile
        # ecx - x
        # edx - y
        erase_redraw:
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        shrl $16, %ecx
        andl $0xffff0000, %edx
        shrl $16, %edx
        imull $160, %edx
        imull $2, %ecx
        addl %edx, %ecx
        movl %ecx, %eax
        movb $0x20, %cl
        pushl %edx
        call mp1_poke
        popl %edx
        
        # update x and y field
        # ebx - pointer to the current missile
        # ecx - x
        # edx - y
        # edi - vx
        # esi - vy
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        movl 12(%ebx), %edi
        movl 16(%ebx), %esi
        addl %edi, %ecx
        addl %esi, %edx
        movl %ecx, 4(%ebx)
        movl %edx, 8(%ebx)

        # draw current missile at new location
        # ebx - pointer to the current missile
        # ecx - x
        # edx - y
        draw:
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        shrl $16, %ecx
        andl $0xffff0000, %edx
        shrl $16, %edx
        movl $0, %eax
        imull $160, %edx
        imull $2, %ecx
        addl %edx, %eax
        addl %ecx, %eax
        movb 32(%ebx), %cl
        pushl %edx
        call mp1_poke
        popl %edx
        jmp draw_other

        explode:
        pushl %ebx
        call missile_explode
        popl %ebx
        cmpl $0, %eax
        je decrement

        # notify the user-space program
        call mp1_notify_user
        
        decrement:
        addl $-1, 28(%ebx)
        movl 28(%ebx), %esi
        cmpl $0, %esi
        je finish_explode

        # draw EXPLOSION
        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        andl $0xffff0000, %edx
        shrl $16, %ecx
        shrl $16, %edx
        movl $0, %eax

        # calculate offset
        imull $160, %edx
        imull $2, %ecx
        addl %edx, %eax

        # 64 - character "@" representing explosion
        addl %ecx, %eax
        movb $64, %cl
        pushl %edx
        call mp1_poke
        popl %edx
        jmp draw_other

        finish_explode:

        movl 4(%ebx), %ecx
        movl 8(%ebx), %edx
        andl $0xffff0000, %ecx
        andl $0xffff0000, %edx
        shrl $16, %ecx
        shrl $16, %edx
        movl $0, %eax

        # calculate offset
        imull $160, %edx
        imull $2, %ecx
        addl %edx, %eax

        # draw " " when finishes exploding
        addl %ecx, %eax
        movb $0x20, %cl
        pushl %edx
        call mp1_poke
        popl %edx

        # remove the missile when finishes exploding
        pushl %ebx
        call mp1_missile_remove
        popl %ebx
    
        # redraw cities and crosshairs
        draw_other:
        call mp1_draw_other

        # move to the next missile
        move_next:
        cmpl $0, %ebx
        je done
        # movl (%ebx), %edi
        popl %edi
        cmpl $0, %edi
        je done
        movl %edi, %ebx
        jmp loop_update

        # updating and walking through the linked list finishes
        done:
        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx
        popl %eax

        leave
        ret

# int mp1 ioctl startgame (unsigned long ignore);
# This function initializes base_alive and the location of crosshairs - (40, 12).
#
#   Inputs: unsigned long ignore (unused in the function)
#   Outputs: always 0 return value for success
#
# Registers: Standard C calling convention

mp1_ioctl_startgame:

        # eax - return values
        # ebx - 40 (the starting of crosshairs_x) or 12 (the starting of crosshairs_y)

        pushl %ebp
        movl %esp, %ebp

        # pushl %eax
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # initial city status
        movl $0x00ffffff, base_alive

        # initial user score
        movl $0, mp1_score

        # initial head pointer of the linked list
        movl $0, mp1_missile_list

        # initial crosshairs location
        movl $40, %ebx
        movl %ebx, crosshairs_x
        movl $12, %ebx
        movl %ebx, crosshairs_y

        # always succeeds with return value 0
        movl $0, %eax

        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx
        # popl %eax

        leave
        ret

# int mp1 ioctl addmissile (struct missile* user missile);
# This function adds a missile to the linked list using mp1_malloc and
# mp1_copy_from_user.
#
#   Inputs: A pointer to a struct missile in user space
#   Outputs: 1. A dynamically allocated buffer of a struct missile in kernel space
#            2. 0 return value for success and -1 for failure
#
# Registers: Standard C calling convention

mp1_ioctl_addmissile:

        # eax - return values
        # ebx - byte number (36)
        # ecx - previous head
        # edx - new head
        # esi - user_pointer

        pushl %ebp
        movl %esp, %ebp

        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # ebx - byte number to be allocated
        movl $36, %ebx
        pushl %ebx
        call mp1_malloc
        popl %ebx

        # check whether malloc successful
        cmpl $0, %eax
        je done_add
        movl %eax, %edx

        # esi - memory in user space waiting to be copied
        movl 8(%ebp), %esi
        pushl %ebx # 36 bytes
        pushl %esi # from
        pushl %edx # to
        call mp1_copy_from_user
        popl %edx # to
        popl %esi # from
        popl %ebx # 36 byte
        cmpl $0, %eax
        jg free

        # add into the linked list
        movl mp1_missile_list, %ecx
        movl %ecx, (%edx)
        movl %edx, mp1_missile_list

        # return 1 when succeeds
        movl $0, %eax
        jmp done_add

        free:
        pushl %ecx
        pushl %ebx
        call mp1_free
        popl %ebx
        popl %ecx

        # return 1 when fails
        movl $-1, %eax

        # addmissile finishes
        done_add:
        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx

        leave
        ret

# int mp1 ioctl movexhairs (unsigned long xhair_delta_packed);
# This function initializes base_alive and the location of crosshairs - (40, 12).
#
#   Inputs: A 32-bit unsigned integer representing the change of the crosshairs'
#           location.
#           1. low 16 bits of xhair_delta_packed - change in x location
#           2. high 16 bits of xhair_delta_packed - change in y location
#   Outputs: always 0 return value for success
#
# Registers: Standard C calling convention

mp1_ioctl_movexhairs:

        # eax - 0 (the return value)
        # ebx - crosshair_x
        # ecx - crosshair_y
        # edx - dx (low 16 of arg)
        # edi - dy (high 16 of arg)

        pushl %ebp
        movl %esp, %ebp

        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # erase at old location
        movl crosshairs_x, %ebx
        movl crosshairs_y, %ecx

        # calculate offset - 80 columns, 2 bytes for each character
        imull $2, %ebx
        imull $160, %ecx
        addl %ecx, %ebx
        movl %ebx, %eax

        movb $0x20, %cl
        pushl %edx
        call mp1_poke
        popl %edx

        # edx - dx (low 16 of arg)
        # edi - dy (high 16 of arg)
        movl 8(%ebp), %edx
        movswl %dx, %edx
        # andl $0x0000ffff, %edx
        movl 8(%ebp), %edi
        andl $0xffff0000, %edi
        shrl $16, %edi
        movswl %di, %edi
    
        # x outside?
        movl crosshairs_x, %ebx
        addl %edx, %ebx
        cmpl $0, %ebx
        jl load_x_0
        cmpl $79, %ebx
        jg load_x_79

        # load new crosshairs_x
        movl %ebx, crosshairs_x

        # y outside?
        move_cross_y:
        movl crosshairs_y, %ecx
        addl %edi, %ecx
        cmpl $0, %ecx
        jl load_y_0
        cmpl $24, %ecx
        jg load_y_24

        # load new crosshairs_y
        movl %ecx, crosshairs_y
        jmp draw_new

        load_x_0:
        movl $0, crosshairs_x
        jmp move_cross_y

        load_x_79:
        movl $79, crosshairs_x
        jmp move_cross_y

        load_y_0:
        movl $0, crosshairs_y
        jmp draw_new

        load_y_24:
        movl $24, crosshairs_y

        # draw at new location
        draw_new:
        movl crosshairs_x, %ebx
        movl crosshairs_y, %ecx

        # calculate offset - 80 columns, 2 bytes for each character
        imull $2, %ebx
        imull $160, %ecx
        addl %ecx, %ebx
        movl %ebx, %eax

        # 43 - character "+"
        movb $43, %cl
        pushl %edx
        call mp1_poke
        popl %edx
        
        # pop all saved registers and return 0
        done_move:

        # always successful
        movl $0, %eax

        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx

        leave
        ret

# int mp1 ioctl getstatus (unsigned long* user status);
# This function copies city status and score to user space.
#
#   Inputs: A pointer to a 32-bit integer in user space
#           1. low 16 bits - mp1_score
#           2. 16, 17 and 18th bit - three cities' status
#   Outputs: 0 for a successful copy and -1 for a failed copy
#
# Registers: Standard C calling convention

mp1_ioctl_getstatus:

        # eax - -1/0 (the return value)
        # ebx - arg (pointer)
        # ecx - user_status
        # edx - city status
        # edi - 0/1 (the bit to be shifted to the 16/17/18th bit)

        pushl %ebp
        movl %esp, %ebp

        # ebx - arg (pointer)
        # ecx - user_status
        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # ebx - arg (pointer, stored in ebp+8)
        movl 8(%ebp), %ebx
        
        # arg (user_status) is NULL, return -1
        cmpl $0, %ebx
        je ret_n1

        # copy mp1_score to cx
        movl $0, %ecx
        movw mp1_score, %cx

        # edx - city status
        movl base_alive, %edx

        andl $0xff000000, %edx
        cmpl $0, %edx
        je city_2
        # 16th bit - city1
        addl $0x00010000, %ecx

        city_2:
        movw mp1_score, %cx
        movl base_alive, %edx
        andl $0x00ff0000, %edx
        cmpl $0, %edx
        je city_3
        # 17th bit - city2
        addl $0x00020000, %ecx
        
        city_3:
        movw mp1_score, %cx
        movl base_alive, %edx
        andl $0x0000ff00, %edx
        cmpl $0, %edx
        je ret_area
        # 18th bit - city3
        addl $0x00040000, %ecx
        
        ret_area:
        # copy ecx to ebx points to
        cmpl $0, %ebx
        je done_status
        movl %ecx, (%ebx)

        # check whether the copy successful
        cmpl $0, %ebx
        je done_status
        cmpl %ecx, (%ebx)
        jne ret_n1

        # succeeds, return 0
        movl $0, %eax
        jmp done_status
        
        # fails, return -1
        ret_n1:
        movl $-1, %eax
        
        # getstatus finishes
        done_status:

        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx

        leave
        ret

# int mp1 ioctl endgame (unsigned long ignore);
# This function ends the game and frees all missiles.
#
#   Inputs: None
#   Outputs: always 0 return value for success
#
# Registers: Standard C calling convention

mp1_ioctl_endgame:
        pushl %ebp
        movl %esp, %ebp

        pushl %ebx
        pushl %ecx
        pushl %edx
        pushl %edi
        pushl %esi

        # move 0, initial city status
        movl $0, base_alive

        # move 0, initial user score
        movl $0, mp1_score

        # eax -  head of the linked list
        movl mp1_missile_list, %eax
        cmpl $0, %eax
        je done_end

        # walk through the linked list to free memory
        # eax - current missile to be freed
        # ebx - next missile to be freed
        loop_end:
        movl (%eax), %ebx
        pushl %eax
        call mp1_free
        # popl %eax
        addl $4, %esp
        movl %ebx, %eax
        cmpl $0, %eax
        je done_end
        jmp loop_end

        # endgame finishes
        done_end:

        # move 0, initial head pointer
        movl $0, mp1_missile_list

        popl %esi
        popl %edi
        popl %edx
        popl %ecx
        popl %ebx

        # always successful
        movl $0, %eax

        leave
        ret

jump_table:
.long mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame
