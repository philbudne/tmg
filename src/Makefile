PDP7_UNIX=../../pdp7-unix
PDP7_CMDSRC=$(PDP7_UNIX)/src/cmd

AS7=$(PDP7_UNIX)/tools/as7

TMGA=	$(PDP7_CMDSRC)/t1.s $(PDP7_CMDSRC)/t2.s \
	$(PDP7_CMDSRC)/t3.s $(PDP7_CMDSRC)/t4.s \
	$(PDP7_CMDSRC)/t5.s $(PDP7_CMDSRC)/t6.s \
	$(PDP7_CMDSRC)/t7.s $(PDP7_CMDSRC)/t8.s
TMGB=	$(PDP7_UNIX)/src/other/pbtz.s

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
	./tmgl7a $< | python fix7.py > $@

################

all:	tmgl7b.s hello.out

# tmgl7a is a PDP-11 style source file, not a PDP-7 style source file!!
tmgl7a.s tmgl7a.7 tmgl7a.7n:
	@echo 'STOP: make sense'; false

tmgl1 tmgl2: build.sh tmgl1.t tmgl2.t tmga.c tmgb.h libs.h tmgc.h
	./build.sh

# build local binary using C port of PDP-11 TMG
tmgl7a: tmg.sh tmgl1 tmgl2 tmgl7a.t
	./tmg.sh tmgl7a.t tmgl7a

# build PDP-7 tmgl7b.s file using tmgl7a
tmgl7b.s: tmgl7a tmgl7b.t fix7.py
	./tmgl7a tmgl7b.t | python fix7.py > tmgl7b.s

# apply changes as a series of patches,
# in case any need to be undone.
tmgl7b.t: tmgl7a.t $(echo 7patch/??-*.patch)
	-mv -f tmgl7b.t save/tmgl7b.t.`date '+%Y%m%d%H%M%S'`
	cp tmgl7a.t tmgl7b.t.tmp
	for PATCH in 7patch/??-*.patch; do \
		patch tmgl7b.t.tmp $$PATCH; \
	done
	mv tmgl7b.t.tmp tmgl7b.t
	rm -f tmgl7b.t.tmp.orig

hello.s: ../examples/hello_world.t tmgl7a fix7.py
	./tmgl7a ../examples/hello_world.t | python fix7.py > hello.s

hello.out: hello.7
	./run hello.7 | grep 'Hello, World' > hello.out.tmp
	mv hello.out.tmp hello.out

################

clean:
	rm -rf build
	rm -f core *.7 *.7n tmgl7a a.out tmgl7b* *~ \#*
	rm -f hello*
