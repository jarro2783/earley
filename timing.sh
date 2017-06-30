#!/bin/bash

STATS="stats-$(date +%s)"
rm -f $STATS

for i in 10 50 100 500 1000 2000; do
  echo -n "$i " | tee -a $STATS
  ./earley "$(./generate.py $i)" -t >> $STATS
done

echo
