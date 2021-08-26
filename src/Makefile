PDP7_UNIX=/home/phil/PDP7/pdp7-unix
PDP7_CMDSRC=$(PDP7_UNIX)/src/cmd
PDP7_TOOLS=$(PDP7_UNIX)/tools
AS7=$(PDP7_TOOLS)/as7

SRCDIR=.

# core code assembled before tmgl generated code
TMGA=	$(PDP7_CMDSRC)/t1.s $(PDP7_CMDSRC)/t2.s \
	$(PDP7_CMDSRC)/t3.s $(PDP7_CMDSRC)/t4.s \
	$(PDP7_CMDSRC)/t5.s $(PDP7_CMDSRC)/t6.s

# storage/defns assembled after tmgl generated code
TMGB=	$(PDP7_CMDSRC)/t7.s $(PDP7_CMDSRC)/t8.s \
	$(PDP7_UNIX)/src/other/pbtz.s

# implicit rules for creating thing.7 (runnable under a7out)
# from thing.t using "make thing.7"
# (thing.t must exist in local directory)
# NOTE: make thing.7 when only .t exists will create AND remove .s file!

.SUFFIXES: .7 .t

# create xxx.7 (a7out format a.out) and xxx.7.n (namelist) from .s file:
.s.7:
	$(AS7) -o $@ -n $(TMGA) $< $(TMGB) && mv n.out $@n

# create PDP-7ish output file:
.t.s:
	$(SRCDIR)/tmgl7a $< | python $(SRCDIR)/fix7.py > $@

################

all:	tmgl7b.s test.ok

test.ok: tmgl7a
	./7tests.sh
	date > test.ok

# tmgl7a.t is a PDP-11 style source file, not a PDP-7 style source file
# used to generate a native binary, so no .s or PDP-7 a.out files!!
tmgl7a.s tmgl7a.7 tmgl7a.7n:
	@echo 'STOP: make sense'; false

# native (C) tmgl, used to generate tmgl7a
tmgl1 tmgl2: build.sh tmgl1.t tmgl2.t tmga.c tmgb.h libs.h tmgc.h
	./build.sh

# build local binary (using C port of PDP-11 TMG)
# that outputs PDP-7 code
# doesn't return exit status (outputs to stderr)
tmgl7a: tmg.sh tmgl1 tmgl2 tmgl7a.t
	./tmg.sh tmgl7a.t tmgl7a.tmp 2>tmgl7a.errs
	./checkerrs.sh tmgl7a.errs
	mv tmgl7a.tmp tmgl7a

# build PDP-7 tmgl7b.s file using tmgl7a
tmgl7b.s: tmgl7a tmgl7b.t fix7.py
	./tmgl7a tmgl7b.t 2>tmgl7b.errs | python fix7.py > tmgl7b.s.tmp
	./checkerrs.sh tmgl7b.errs




	mv tmgl7b.s.tmp tmgl7b.s

# make PDP-7 "binary" runnable under a7out
tmgl7b.7: tmgl7b.s

# tmgl7b.t is generated from tmgl7a.t by
# by applying changes as a series of patches,
# in case any need to be undone.
tmgl7b.t: tmgl7a.t $(echo 7patch/??-*.patch)
	-mv -f tmgl7b.t save/tmgl7b.t.`date '+%Y%m%d%H%M%S'`
	cp tmgl7a.t tmgl7b.t.tmp
	for PATCH in 7patch/??-*.patch; do \
		patch tmgl7b.t.tmp $$PATCH; \
	done
	mv tmgl7b.t.tmp tmgl7b.t
	rm -f tmgl7b.t.tmp.orig

################

clean:
	rm -rf build
	rm -f *.tmp *.7 *.7n tmgl7a tmgl7b* *~ \#*
	rm -f a.out n.out core test.ok
	rm -f test/*/*.[7s]* test/*/*.log* test/*/core
