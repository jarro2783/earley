#!/usr/bin/env python3

from random import choice, randrange
import sys

symbols = ['+', '/', '*', '-']

def generate(length):
    print(randrange(1, 500), end='')
    for i in range(1, length):
        print(' {} {}'.format(choice(symbols), randrange(1, 500)), end='')
    print()

if __name__ == '__main__':
    generate(int(sys.argv[1]))
