MK="make -f ../../Makefile SRCDIR=../../"
export PDP7_TOOLS=../..
for DIR in 7test/[0-9]*; do
    (
	cd $DIR
	rm -f *.s *.7 *.7n
	if ! $MK test.s > make.log 2>&1; then
	    echo $DIR make .s FAILED
	    exit 1
	fi
	if ! $MK test.7 > make.log2 2>&1; then
	    echo $DIR make .7 FAILED
	    exit 1
	fi
	if [ -f test.in ]; then
	    IN=test.in
	else
	    IN=/dev/null
	fi
	# setting bit zero of console switches prevents automatic core dump!
	${PDP7_TOOLS}/a7out -n test.7n -s 400000 test.7 < $IN > test.out 2>test.err
	# XXX examine test.err file??
	if cmp test.out test.ref; then
	    echo $DIR PASSED
	else
	    echo $DIR FAILED: see test.out and test.err
	    exit 1
	fi
    )
done
