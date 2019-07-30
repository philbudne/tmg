// Port of the Unix compiler-compiler TMG to C99.
// Based on the original PDP-11 assembly code by M. D. McIlroy.
// (c) 2019, Andriy Makukha, 2-clause BSD License.

#include <limits.h>
#include <stdint.h>
#include "libs.h"

// Debugging output enabled? (see tmgc.h)
#if DEBUG_MODE
int _depth = 0;
const char* _space = "......................................................................";
#define DEPTH               ((const char*)(_space + strlen(_space) - 2*_depth))
#define DEBUG(msg, ...)     do { if (verbose) fprintf(dfile, msg "\n", ##__VA_ARGS__); } while(0)
#define DEBUG_DEEPER        _depth++
#define DEBUG_SHALLOWER     _depth--
#else
#define DEBUG(msg, ...)
#define DEBUG_DEEPER
#define DEBUG_SHALLOWER
#endif

// Defined in tmga.c
extern bool verbose;

extern tword iget();
extern void  errcom(const char* msg);
extern void  generate();
extern void  obuild();
extern void  parse();
extern void  putch();
extern void  succ();
extern void  _tp();

tword trswitch = 0;

// Function declarations
void _px();
void _pxs();
void _tx();
void _txs();
void kput();
void octal();
void _octal();
void putcall();
void puthex();
void putoct();
void sprv();
void trans();
void trace();

// Function definitions
void _px() {}
void _pxs() {}
    
void _tx() {
    DEBUG("    _tx()");
    r0 = *i++;
    DEBUG("    _tx: r0=%lx", (tword)r0);
    // Unlike in the original, r0 contains address of address of C-string
    // (rather than address of the string directly)
    r0 = *(tptr)r0;
    obuild();
    return generate();  // Tail call
}

void _txs() {
    DEBUG("    _txs()");
    r0 = (tword)i;
    i++;
    obuild();
    return generate();  // Tail call
}

void kput() {
    ((parse_frame_t*)f)->k -= sizeof(tword);
    r1 = -((parse_frame_t*)f)->k;
    if (r1 >= ktat)
        errcom("translation overflow");
    *((tptr)(ktab + r1)) = r0;
}

void octal() {
    DEBUG("    octal()");
    r0 = 1 + (tword)&_octal;
    putcall();
    iget();
    r0 = *(tptr)r0;
    kput();
    return succ();  // Tail call
}

void _octal() {
    r0 = *i;
    putoct();
    return generate();  // Tail call
}

void putcall() {
    kput();
    *g++ = ((parse_frame_t*)f)->k;
}

void puthex() {
    // Derived from putoct (hex is much more convenient for modern machines)
    PUSH(r0);
    r0 = (tword)BIT_CLEAR(15, r0);
    stack[sp] = (tword)BIT_CLEAR(r0, stack[sp]);
    r0 >>= 4;
    if (r0) puthex();
    r0 = POP();
    if (r0 < 10) r0 += '0';
    else r0 += 'W';
    putch();
}

void putoct() {
    PUSH(r0);
    r0 = (tword)BIT_CLEAR(7, r0);
    stack[sp] = (tword)BIT_CLEAR(r0, stack[sp]);
    r0 >>= 3;
    if (r0) putoct();
    r0 = POP() + '0';
    putch();
}

// from: arith.s
// make sp hold a simple rvalue (forget it might be a table value)
void sprv() {
    r0 = (tword)POP();
    if (stack[sp+1]==-1) {
	//mov	(sp)+,(sp)
	//mov	(sp)+,(sp)
        stack[sp+1] = stack[sp];
        sp++;
        stack[sp+1] = stack[sp];
        sp++;
    }
    return (*(void (*)(void))r0)(); // Tail call
}

void trans() {
    DEBUG("    trans()");
    *g++ = iget();
    return succ();  // Tail call
}

void trace() {
    DEBUG("    trace(): %c%lx", (char)r0, (tuword)i);
    putch();
    r0 = 'x';
    putch();
    r0 = (tword)i;
    puthex();
    r0 = '\n';
    putch();
}

// from: arith.s
// update a stored value, used by all assignments
void update() {
    DEBUG("    update()");
    if (stack[sp+2] != -1) {
        *(tptr)stack[sp+2] = stack[sp+1];
        return;
    }
    // TODO: what's the meaning of this?
    r1 = stack[sp+3];
    r0 = stack[sp+4];
    seekchar();
    r0 = stack[sp+1];
    return alterword(); // Tail call
}
