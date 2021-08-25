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
	../../run test.7 > test.out 2>/dev/null
	if cmp test.out test.ref; then
	    echo $DIR PASSED
	else
	    echo $DIR FAILED
	    exit 1
	fi
    )
done
