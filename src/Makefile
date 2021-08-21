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

.SUFFIXES: .7 .t

# create xxx.7 (a7out format a.out) and xxx.7.n (namelist) from .s file:
.s.7:
	$(AS7) -o $@ -n $(TMGA) $< $(TMGB) && mv n.out $@n

# create PDP-7ish output file:
.t.s:
	./tmgl7a $< | python fix7.py > $@

################

all:	hhh.7 hello.7

################
# build local binary using C port of PDP-11 TMG

tmgl7a:	tmgl7a.t tmgl1
	./tmg.sh tmgl7a.t tmgl7a

################

tta.s:	tmgl7a tmgl7a.t
	./tmgl7a tmgl7a.t > tta.s

ttb.s:	tta.s fix7.py
	python fix7.py < tta.s > ttb.s

################

# hhh.s is hand made!
clean:
	rm -rf build
	rm -f core *.7 *.7n hhh hello hello.s tmgl7a a.out tt[ab].s *~ \#*
