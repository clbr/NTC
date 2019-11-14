#!/bin/sh -e

lines=`wc -l COPYING`
lines=${lines% *}

for i in `seq 1 $lines`; do
	head -n $i COPYING > test
	./nestextcomp test
	rm test
done
