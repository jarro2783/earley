#!/usr/bin/env python3

from generate import generate
from random import choices, randint
import sys

def identifier():
  alpha = [chr(x) for x in range(ord('a'), ord('z') + 1)]
  return ''.join(choices(alpha, k=randint(2,7)))

def expressions(length):
  for i in range(1, length):
    if randint(0, 10) < 2:
      print("{} =".format(identifier()), end=' ')
    generate(randint(2, 12))

if __name__ == '__main__':
  expressions(int(sys.argv[1]))
