// Port of the Unix compiler-compiler TMG to C99.
// Based on the original PDP-11 assembly code by M. D. McIlroy.
// (c) 2020, Andriy Makukha, 2-clause BSD License.
//
// Structure of this implementation closely follows the structure of the
// original code. Comments starting with a lowercase letter were either
// copied from the original assembly code or directly stem from it.

// tmg
// main program and parsing rule interpreter

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tmgb.h"
#include "tmgl.h"

#ifdef  SRC_LANGUAGE
#define SRC_LANGUAGE_ SRC_LANGUAGE " "
#else
#define SRC_LANGUAGE_ ""
#endif
#ifdef  DST_LANGUAGE
#define DST_LANGUAGE_ DST_LANGUAGE " "
#else
#define DST_LANGUAGE_ ""
#endif

tptr classtab = (tptr)__classtab;

// This is an address range used to distinguish predefined function pointers
tptr  func_min;
tptr  func_max;

// Statictics
bool verbose    = false;
tword succc     = 0;
tword continc   = 0;
tword failc     = 0;
tword advc      = 0;

// Function declarations
void adv();
void contin();
void diag();
void errcom(const char* error);
void fail();
void flush();
void generate();
void gcontin();
void obuild();
void parse();
void pbundle();
void _process();
void putch();
void succ();
void _tp();

// Description:
//      get interpreted instruction for a parsing rule.
//      negative instruction is a pointer to a parameter in this
//      stack frame: fetch that instead, put environment pointer in r1
// Typically:
//      Typically it will just retrieve next word from the driving table.
// Special case:
//      If that word is negative, it is a parameter reference.
// Return:
//      r0 - Retrieved word from the driving table. OR parameter.
//      r1 - (IFF r0 is a parameter) environment pointer.
void iget() {
    if ((tptr)i >= start && (tptr)i < ARRAY_END(start))
        DEBUG("    iget: i=0x%lx [%ld]=%ld", (tuword)i, ((tword)((tptr)i-start)), *i);
    else
        DEBUG("    iget: i=0x%lx", (tuword)i);
    r1 = (tword)f;
    r0 = *i++;
    if (r0 < 0) {
        // negative instruction is a pointer to a parameter in this
        // stack frame: fetch that instead, put environment pointer in r1
        DEBUG("    iget: negative r0=%ld -> parameter", r0);
        PUSH(r0 & 1);               // save the exit bit
        r0 &= ~(tuword)stack[sp];   // Clear exit bit in r0
        do {                        // chase parameter
            r1 = (tword)((parse_frame_t*)r1)->env; 
            r0 = (tword)( ((parse_frame_t*)r1)->si + (r0 >> 1) );   // It is expected that negative values are doubled
            r0 = *(tptr)r0;
        } while (r0 < 0);
        r1 = (tword)((parse_frame_t*)r1)->env; 
        r0 |= (tword)POP();         // Restore the exit bit
    }
}

void contin() {
    continc++;
    DEBUG("%scontin(): f=%lu, g=%lu", DEPTH, f-(tptr)stkb, g-(tptr)stkb);
#if TRACING
    if (trswitch) {
        r0 = 'r';
        trace();
    }
#endif

    // get interpreted instruction
    // save its exit bit (bit 0) on stack
    // distinguish type of instruction by ranges of value
    iget();
    ((parse_frame_t*)f)->x = r0;
    DEBUG("%s          x==0x%lx", DEPTH, ((parse_frame_t*)f)->x);
    r0 = BIT0_CLEAR(r0);
    if ((tptr)r0 >= start && (tptr)r0 < ARRAY_END(start)) {
        // tmg-coded rule, execute and test its success
        DEBUG("TMG-CODED RULE: %lu", ((tuword)r0-(tuword)start)/sizeof(tptr));
        adv();
        if (failure)
            return fail();  // Tail call
        else {
            DEBUG("%scontin() -> succ", DEPTH);
            return succ();  // Tail call
        }
    } else if ((tptr)r0 >= func_min && (tptr)r0 <= func_max) {
        // machine coded function
        DEBUG("MACHINE-CODED: 0x%lx", r0);
        return (*(void (*)(void))r0)(); // Tail call
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "bad address in parsing: 0x%08lx\n", r0);
        errcom(msg);
    }
}

void alt() {
    DEBUG("%salt", DEPTH);
    i++;
    DEBUG("%salt() -> succ", DEPTH);
    return succ();      // Tail call
}

void salt() {
    DEBUG("%ssalt", DEPTH);
    iget();
    i = (tptr)r0;
    return contin();    // Tail call
}

void tgoto() {
    DEBUG("%stgoto", DEPTH);
    return salt();      // Tail call
}

// all functions and rules that fail come here
// if exit bit is on do a fail return
// if following instruction is an alternate (recognized literally)
// do a goto, if a success alternate, do a nop
// otherwise do a fail return
void fail() {
    DEBUG("%sfail(): f=%ld, g=%ld", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb));
    failc++;
    if (!(((parse_frame_t*)f)->x & 1)) {    // Exit bit not set
        DEBUG("%sexit bit not set", DEPTH);
        iget();
        ((parse_frame_t*)f)->x = r0;        // checked both
        DEBUG("%s        x==0x%lx", DEPTH, ((parse_frame_t*)f)->x);
        r0 &= ~(tword)1;
        if (r0 == (tword)&alt)      // TODO: why does it go to salt if equal to alt and v.v.?
            return salt();  // Tail call
        if (r0 == (tword)&salt)
            return alt();   // Tail call
    }

    // do a fail return
    // pop stack
    // do not update j or k
    // restore interpreted instruction counter
    g = f;
    f = (tptr)((parse_frame_t*)f)->prev;
    i = ((parse_frame_t*)f)->si;
    DEBUG_SHALLOWER;
    DEBUG("%s<failure: f=%ld, g=%ld", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb));
    failure = true;
}

// all functions that succeed come here
// test the exit indicator, and leave the rule if on
void succ() {
    ++succc;
    DEBUG("%ssucc() f=%ld, g=%ld, c=%lu", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb), succc);
    if (((parse_frame_t*)f)->x & 1) {       // Exit bit set
        // do a success return
        // bundle translations delivered to this rule,
        // pop stack frame
        // restore  interpreted instruction counter (i)
        // update input cursor (j) for invoking rule
        // update high water mark (k) in ktable
        // if there was a translation delivered, add to stack frame
        // clear the fail flag
        r0 = (tword)f + sizeof(parse_frame_t);
        pbundle();
        g = f;
        f = (tptr)(((parse_frame_t*)f)->prev);
        i = ((parse_frame_t*)f)->si;
        ((parse_frame_t*)f)->j = ((parse_frame_t*)g)->j;
        ((parse_frame_t*)f)->k = ((parse_frame_t*)g)->k;
        if (r0)
            *g++ = r0;      // TODO: what is the meaning of this?
        DEBUG_SHALLOWER;
        DEBUG("%s<success: f=%ld, g=%ld", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb));
        failure = false;
        return;
    }
    DEBUG("%sexit bit not set", DEPTH);
    return contin();    // Tail call
}

void errcom(const char* error) {
    if (error)
        fprintf(dfile, "TMG error: %s\n", error);
    exit(1);
}

// Description:
//      advance stack frame to invoke a parsing rule
//      copy  cursor, water mark, ignored class to new frame
//      set intial frame length to default (g1)
//      check end of stack
// Parameters:
//      r0,r1 are new i,environment
void adv() {
    advc++;
    DEBUG("%s>adv()", DEPTH);
    parse_frame_t* _f = (parse_frame_t*)f;      // Cast for conveniencee
    parse_frame_t* _g = (parse_frame_t*)g;
    _g->prev = _f;
    _f->si = i;
    _g->j = _f->j;
    _g->k = _f->k;
    _g->n = _f->n;
    f = g;
    DEBUG_DEEPER;
    g = (tptr)((tword)g + sizeof(parse_frame_t)); // g1
    if ((uint8_t*)g >= stke)
        errcom("stack overflow");
    i = (tptr)r0;                       // Initially this contains &start[0]
    DEBUG("%s>adv(): env=0x%lX, f=0x%lX", DEPTH, (tuword)r1, (tuword)f);
    ((parse_frame_t*)f)->env = (tptr)r1;
    return contin();    // Tail call
}

// Description:
//      pbundle entered with pointer to earliest element of bundle
//      to reduce from the top of stack in r0
//      exit with pointer to bundle in r0, or zero if bundle is empty
// Parameters:
//      r0 - pointer to earliest element of bundle to be reduced
// Return:
//      r0 - pointer to bundle
void pbundle() {
    if ((tptr)r0 >= g) {
        DEBUG("%sempty bundle", DEPTH);
        r0 = 0;         // empty bundle
        return;
    }
    PUSH(r0);
    r1 = r0;
    r0 = *(tptr)r1;
    r1 += sizeof(tword);
    if ((tptr)r1 != g) {
        // non-trivial bundle
        DEBUG("%snon-trivial bundle", DEPTH);
        do {
            PUSH(r1);
            kput();
            r1 = POP();
            r0 = *(tptr)r1;
            r1 += sizeof(tword);
        } while((tptr)r1 <= g);
        r0 = ((parse_frame_t*)f)->k;
    } else {
        DEBUG("%s\"trivial\" bundle: r1==g", DEPTH);
    }
    g = (tptr)POP();
}

// tmg translation rule interpreter (generator)

void generate() {
    DEBUG("%sgenerate(), exit=%lu, f=%lu, g=%lu", DEPTH, ((parse_frame_t*)f)->x & 1,
                                                  (tuword)(f-(tptr)stkb), (tuword)(g-(tptr)stkb));
    // checked
    if (((parse_frame_t*)f)->x & 1) {       // Exit bit set
        // exit bit is on -> pop stack frame, restore instruction counter and return
        f = (tptr)((tuword)f - fs);
        i = ((parse_frame_t*)f)->si;
        DEBUG_SHALLOWER;
        return;
    }
    return gcontin();   // Tail call
}

void gcontin() {
    DEBUG("%sgcontin(): f=%lu, g=%lu", DEPTH, (tuword)(f-(tptr)stkb), (tuword)(g-(tptr)stkb));
#if TRACING
    if (trswitch) {
        r0 = 'g';
        trace();
    }
#endif
    // get interpreted instruction, decode by range of values
    r0 = (tword)*i++;
    ((parse_frame_t*)f)->x = r0;
    DEBUG("%s           x==0x%lx", DEPTH, r0);
    r0 = BIT0_CLEAR(r0);
    DEBUG("%s          r0==0x%lx", DEPTH, r0);
    if ((tptr)r0 >= start && (tptr)r0 < ARRAY_END(start)) {
        // tmg-coded translation subroutine
        // execute it in current environment
        DEBUG("TMG-CODED ROUTINE: [%lu]", ((tptr)r0-start));
        translation_frame_t* _f = (translation_frame_t*)f;      // Cast for convenience
        translation_frame_t* _n = (translation_frame_t*)((tuword)f + fs);
        _f->si = i;
        i = (tptr)r0;
        _n->ek = _f->ek;
        _n->ep = _f->ep;
        f = (tptr)_n;
        DEBUG_DEEPER;
        DEBUG("%s>gcontin(): f=%lu, g=%lu", DEPTH, (tuword)(f-(tptr)stkb), (tuword)(g-(tptr)stkb));
        gcontin();
        return generate();  // Tail call
    } else if ((tptr)r0 >= func_min && (tptr)r0 <= func_max) {
        // builtin  translation function
        DEBUG("BUILTIN FUNCTION: 0x%lx", r0);
        return (*(void (*)(void))r0)(); // Tail call
    } else if (-r0 < KTAT) {
        // delivered compound translation
        // instruction counter is in ktable
        // set the k environment for understanding 1, 2 ...
        // to designate this frame
        DEBUG("COMPOUND");
        ((translation_frame_t*)f)->ek = f;
        r0 = (tword)(ktab - r0);            // Effectively &ktab[-r0], r0 is negative
        i = (tptr)r0;
        DEBUG("COMPOUND: r0=0x%lx", r0);
	return gcontin();   // Tail call
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "bad address in translation: 0x%08lx", r0);
        errcom(msg);
    }
}

// Description:
//      execute rule called for by 1 2 ...
//      found relative to instruction counter in the k environment
//      this frame becomes th p environment for
//      any parameters passed with this invocation
//      e.g. for 1(x) see also .tq
void _tp() {
    DEBUG("%s_tp(): f=0x%lx, g=0x%lx", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb));
    r0 = *(char*)i;
    r2 = *((char*)i + 1); 
    i++;    // Using only two bytes of the word
    //r0 = (r0 + 1)<<1;    // Accounting for word size
    r0 = (r0 + 1)*sizeof(tword);
    translation_frame_t* _f = (translation_frame_t*)f;    // Cast for convenience
    translation_frame_t* _n = (translation_frame_t*)((tword)f + fs);
    _f->si = i;
    _n->ep = (tptr)f;
    r1 = (tword)_f->ek;
    i = ((translation_frame_t*)r1)->si;
    i = (tptr)((tword)i - r0);
    f = (tptr)_n;
    DEBUG_DEEPER;
    ((translation_frame_t*)f)->ek = f;
    //r2 <<= 1;     // Accounting for word size
    r2 *= sizeof(tword);
    if (r2 != 0) {
        //element is 1.1, 1.2, .. 2.1,...
        i = (tptr)-*i;
        if ((tword)i < 0)
            errcom("not a bundle");
        if ((tword)i >= KTAT) {
            char msg[100];
            snprintf(msg, sizeof(msg), "bad address in _tp: %ld > %ld", (tword)i, KTAT);
            errcom(msg);
        }
        i = (tptr)((tword)i + ktab - r2);
    }
    gcontin();
    return generate();  // Tail call
}

// diag and parse builtins
// set current file to diagnostic or output
// save and restore ktable water mark around parse-translate
// also current file and next frame pointer (g)
// execute parsing rule
void diag() {
    r1 = (tword)dfile;
    return _process();  // Tail call
}

void parse() {
    DEBUG("    parse");
    r1 = (tword)ofile;
    return _process();  // Tail call
}

void _process() {
    DEBUG("    _process");
    PUSH(cfile);
    cfile = (FILE*) r1;
    PUSH(((parse_frame_t*)f)->k);
    PUSH(g);
    iget();
    adv();
    if (!failure) {
        // rule succeeded
        // if it delivered translation, put it in ktable and set
        // instruction counter for
        // translation generator to point there
        // go generate
        if (g > (tptr)POP()) {
            r0 = *(--g);        // TODO: what is the point here? reversing what's in succ?
            kput();
            i = (tptr)(ktab - ((parse_frame_t*)f)->k);
            PUSH(f);
            f = g;
            DEBUG("%sx==0, f=%ld, f=%ld", DEPTH, (tword)(f-(tptr)stkb), (tword)(g-(tptr)stkb));
            ((parse_frame_t*)f)->x = 0;     // checked
            DEBUG(">generating (in parse)");
            generate();
            DEBUG("<generated");
            f = (tptr)POP();
            i = ((parse_frame_t*)f)->si;
        }
        ((parse_frame_t*)f)->k = POP();
        cfile = (FILE*)POP();
        DEBUG("%s_process -> succ", DEPTH);
        return succ();  // Tail call
    } else {
        g = (tptr)POP();
        ((parse_frame_t*)f)->k = POP();
        cfile = (FILE*)POP();
        return fail();  // Tail call
    }
}

// tmg output routines

// Description:
//      adds 1 or 2 (or more, depending on architecture) characters in r0 to output
// Parameters:
//      r0 - a word containing the string; may be not nul-terminated
void putch() {
    PUSH(0);
    PUSH(r0);
    r0 = (tword)&stack[sp];
    obuild();
    POP();
    POP();
}

// Description:
//      r0 points to string to put out on current output file (cfile)
//      string terminated by 0
//      if last file differed from current file, flush output buffer first
//      in any case flush output buffer when its write pointer (outw)
//      reaches its top (outt)
// Parameters:
//      r0 - C-string to print into the current output file
void obuild() {
    //DEBUG("obuild(): %s", (tptr)r0);
    if (cfile != lfile) {
        flush();
        lfile = cfile;
    }
    r1 = outw;
    do {
        if (!*(char*)r0) {
#if NOBUFFER
            flush();
#endif
            return;
        }
        outb[r1++] = *(char*)r0;
        r0++;           // TODO: use outw directly
        outw = r1;
    } while (r1 <= OUTT);
    flush();
    return obuild();    // Tail call
}

// copy output buffer onto last output file and clear buffer
void flush() {
    fwrite(outb, 1, outw, lfile);
    outw = 0;
}

int main(int argc, char* argv[]) {
    // These have to be initialized here because stdout/stderr are not compile-time constants
    input = NULL;
    cfile = stdout;
    lfile = stdout;
    dfile = stderr;
    ofile = stdout;

    // Verbose?
    r1 = 1;
    if (r1 < argc && !strcmp(argv[r1], "-v")) {
        verbose = true;
        r1++;
    }

    // Help message
    if (r1 >= argc || !strcmp(argv[r1], "-h")) {
        printf(SRC_LANGUAGE_ "compiler (%lu-bit)\n", 8*sizeof(tword));
        printf("Usage: %s [-v] [-h] input [output]\n", argv[0]);
        printf("\tinput \t- " SRC_LANGUAGE_ "program\n");
        printf("\toutput\t- " DST_LANGUAGE_ "translation\n");
        printf("\t-v    \t- verbose output\n");
        printf("\t-h    \t- display this message and exit\n");
        
        if (verbose) {
            DEBUG("Driving table values:");
            for (int i=0; i<sizeof(start)/sizeof(*start) && i<25; i++)
                printf("%08lx\n", (tword)start[i]);
        }
        return 0;
    }

    // get arguments from shell
    // arg1 is input file
    // arg2 is output file (standard output if missing)
    input = fopen(argv[r1], "r");
    if (!input) {
        fprintf(dfile, "could not open: %s\n", argv[r1]);
        return 1;
    }
    if (++r1 < argc) {
        ofile = fopen(argv[r1], "w");
        if (!ofile) {
            fprintf(dfile, "could not open for writing: %s\n", argv[r1]);
            return 1;
        }
    }

    // Replace global label references in the driving table
    for (tword j = 0; j < sizeof(start)/sizeof(*start); j++)
        if ((tptr)start[j] >= labels && (tptr)start[j] < ARRAY_END(labels))
            start[j] = *((tword *)BIT0_CLEAR(start[j])) | (start[j] & 1);   // Preserve exit bit
    if (verbose) {
        fprintf(dfile, "Driving table size = %lu words (%lu bytes)\n", 
                        sizeof(start)/sizeof(*start), sizeof(start));
        fprintf(dfile, "Driving table range: 0x%08lX..0x%08lX\n", (tuword)start, (tuword)start + sizeof(start));
        fprintf(dfile, "Ktable range: 0x%08lX..0x%08lX\n", (tuword)ktab, (tuword)ktab + sizeof(ktab));
    }

    // Compute function address range
    tptr funcs[] = {
        (tptr)&adv,
        (tptr)&alt,     (tptr)&salt,    (tptr)&succ,    (tptr)&fail,
        (tptr)&parse,
        (tptr)&contin,
        // tmgb functions
        (tptr)&trans,   (tptr)&_tp,
        (tptr)&_l,      (tptr)&_p,      (tptr)&_t,      (tptr)&_u,      (tptr)&_st,
        (tptr)&_da,     (tptr)&_ia,     (tptr)&_db,     (tptr)&_ib,
        (tptr)&_px,     (tptr)&_pxs,    (tptr)&_tx,     (tptr)&_txs,
        (tptr)&_ge,     (tptr)&_ne,     (tptr)&_eq,
        (tptr)&_a,      (tptr)&_s,      (tptr)&_n,      (tptr)&_o,      (tptr)&_x,
        (tptr)&decimal, (tptr)&octal,
        (tptr)&smark,   (tptr)&any,     (tptr)&string,
        (tptr)&scopy,   (tptr)&_scopy,
    };
    func_max = 0;
    func_min = (tptr)SIZE_MAX;
    for (tword j = 0; j < sizeof(funcs)/sizeof(*funcs); j++) {
        if (funcs[j] > func_max)  func_max = funcs[j];
        if (funcs[j] < func_min)  func_min = funcs[j];
    }
    if (verbose)
        fprintf(dfile, "Functions range: 0x%08lX..0x%08lX\n", (tuword)func_min, (tuword)func_max);

    // set up tables
    // initialize stack
    // go interpret beginning at "start"
    // finish up
    f = (tptr) stkb;
    parse_frame_t* _f = (parse_frame_t*) f;
    _f->j = 0;
    _f->k = 0;
    _f->n = 0;
    g = (tptr)((uint8_t*)f + sizeof(parse_frame_t));
    r0 = (tword)start;
    adv();
    flush();
    //unlink("alloc.d");    // TODO

    return 0;
}
