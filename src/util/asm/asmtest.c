/*
Inline MIPS assembly tests

Extended asm: 
    - read/write C vairables from assembler
    - perform jumps from assember code to C labels
    - statements must be inside a C function

    Use colons(":") to delimit operand params after the assember template:

        asm asm-qualifiers ( AssemblerTemplate 
                            : OutputOperands
                          [ : InputOperands
                          [ : Clobbers ]] ) 
        
        asm asm-qualifiers ( AssemblerTemplate 
                            :
                            : InputOperands
                            : Clobbers
                            : GotoLabels ) 

*/
#include <stdio.h>

#define pint(a) printf("%s = %d\n", #a, a)
#define plong(a) printf("%s = %ld\n", #a, a)

int main() {
    // NOP example
    // "volatile": tells the compiler to exclude assembler code from optimization
    asm volatile("mov r0,r0"); 
  
    // basic right rotate bits
    int x=16, y =0;
    asm volatile("mov    %[result], %[value], ror #1"

           : [result]"=r" (y) /* Rotation result. */
           : [value]"r"   (x) /* Rotated value. */
           : /* No clobbers */
    );

    pint(x); // 16
    pint(y); // 8


    // There is no guarantee that the compiled code will retain the sequence of statements given!
    int b=3, c=4;
    asm volatile("mrs r12, cpsr\n\t"
                 "orr r12, r12, #0xC0\n\t"
                 "msr cpsr_c, r12\n\t" ::: "r12", "cc");
    c *= b; /* This may fail */
    asm volatile("mrs r12, cpsr\n"
                 "bic r12, r12, #0xC0\n"
                 "msr cpsr_c, r12" ::: "r12", "cc");
    pint(b); pint(c);

    // "memory" keyword: tells compiler the assembler instruction may change memory locations.
    //                   forces the compiler to store all cached values before and reload after
    //                   executing assembler instrustions
    b=3; c=4;
    asm volatile("mrs r12, cpsr\n\t"
                 "orr r12, r12, #0xC0\n\t"
                 "msr cpsr_c, r12\n\t" :: : "r12", "cc", "memory");
    c *= b; /* This is safe. */
    asm volatile("mrs r12, cpsr\n"
                 "bic r12, r12, #0xC0\n"
                 "msr cpsr_c, r12" ::: "r12", "cc", "memory");
    pint(b); pint(c);

    // Since invalidating all cached values may be suboptimal, can add a dummy opeand to create an
    // artificial dependency:
    b=3; c=4;
    asm volatile("mrs r12, cpsr\n\t"
                 "orr r12, r12, #0xC0\n\t"
                 "msr cpsr_c, r12\n\t" : "=X" (b) :: "r12", "cc");
    c *= b; /* This is safe. */
    asm volatile("mrs r12, cpsr\n"
                 "bic r12, r12, #0xC0\n"
                 "msr cpsr_c, r12" :: "X" (c) : "r12", "cc");
    // Above code pretends to modify variable b in the 1st asm statement and to use the contents of variable c in the 2nd.
    // This preserves the intended operation sequence w/o invalidating other cached values
    pint(b); pint(c);

    // Constraints: tell inline assembler how to represent constants, pointers, or vars in assembly code

    // "+": read-write operand, must be listed as an output operand
    asm("mov %[value], %[value], ror #1" : [value] "+r" (y));   // rotate right, store in same operand
    pint(y); // 4

    // endian flip macro
    #include "asm_util.h"
    
    {
    int endian_check = 1;
    char *tmp = (char*)&endian_check;
    printf("This is a %s endian machine\n", *tmp+48 == '1' ? "little" : "big");
    }

    long swapval = 16;
    plong(swapval);
    ENDIAN_SWAP(swapval);
    plong(swapval);
    ENDIAN_SWAP(swapval);
    plong(swapval);

    // Must use clobber list to tell the compiler about any scratch registers not passed in as operands
    // e.g. using r3 as a scratch register to adjust a value to a multiple of 4:
    int shiftval = 1;
    asm volatile(
        "ands    r3, %1, #3"     "\n\t" // ands modifies the CPU status flag
        "eor     %0, %0, r3" "\n\t"
        "addne   %0, #4"
        : "=r" (shiftval)
        : "0" (shiftval)
        : "cc"/*ands instruction clobber*/, "r3"/*scratch register used*/
    );
    pint(shiftval);

    // can use the mov instruction to load an immediate constant value into a register
    // limited to value range [0, 255] (basically)
    asm("mov r0, %[flag]" : : [flag] "I" (0x80));
    // larger values can be used when rotating the given range by an even # of bits;
    //      Any result of:  n * 2^x
    //      w/ n in the range [0, 255] and x is an even # in the range [0, 24]

    /*
        Jump to a constant memory address, e.g. defined by a preprocessor macro:  
                ldr r3, =JMPADDR
                bx r3
            
        - works for any legal address value        
        - if the address fits(e.g. 0x20000000), it is trivially converted to:
                mov r3, #0x20000000
                bc r3
        - if it doesn't fit (e.g. 0x00FF000F0), the assembler will load the value
            from the literal pool:
                ldr r3, .L1
                bx  r3
                ...
                .L1: .word 0x00F000F0
        
        With inline assembly, instead of using ldr, can provide a constant as a 
        register value:        
    */
   #define JMPADDR 0x20000000
    asm volatile("bx %0" : : "r" (JMPADDR));

    /*
        Load the link register w/ a constant address:
                ldr  lr, =JMPADDR
                ldr  r3, main
                bx   r3  
    */
   // inline version:
    asm volatile(
        "mov lr, %1\n\t"
        "bx %0\n\t"
        : : "r" (main), "r" (JMPADDR));     
    /*
       Compared to the pure assembly code, we get an additional statement, using an 
       additional register!
                ldr     r3, .L1
                ldr     r2, .L2
                mov     lr, r2
                bx      r3
    */
    return 0;
}

/*
    Typical compiler register usage:
        
        r0/a1       1st fn argument
                    Integer fn result
                    Scratch register
        
        r1/a2       2nd fn argument
                    Scratch register

        r2/a3       3rd fn argument
                    Scratch register
        
        r4/v1       Register variable
        r5/v2       Register variable
        r6/v3       Register variable
        r7/v4       Register variable
        r8/v5       Register variable
        
        r9/v6/rfp   Register variable
                    Real frame pointer
        
        r10/sl      Stack limit

        r11/fp      Argument pointer
        
        r12/ip      Temporary workspace

        r13/sp      Stack pointer

        r14/lr      Link register
                    Workspace
        
        r15/pc      Program counter
*/