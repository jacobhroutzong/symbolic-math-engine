"""Exact, generated local fixtures. Independent Python-int/Fraction and SymPy oracles.

SymPy parsing below receives only this script's trusted generated grammar, never
arbitrary user input. This is mathematical validation, not target differential RE.
"""
import argparse
import fractions
import hashlib
import json
import math
import pathlib
import random
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--cli', required=True)
parser.add_argument('--reference-path')
parser.add_argument('--output')
args = parser.parse_args()
if args.reference_path:
    sys.path.insert(0, args.reference_path)
import sympy as sp


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


rng = random.Random(20261026)
commands = []
integer_cases = []
for _ in range(200):
    a = rng.randrange(-10**100, 10**100)
    b = rng.randrange(-10**60, 10**60) or 1
    integer_cases.append((a, b))
    commands.append(f'integer\t{a}\t{b}')
rational_cases = []
for _ in range(100):
    a = fractions.Fraction(rng.randrange(-10**30, 10**30), rng.randrange(1, 10**20))
    b = fractions.Fraction(rng.randrange(1, 10**30), rng.randrange(1, 10**20))
    rational_cases.append((a, b))
    commands.append(f'rational\t{a}\t{b}')
expressions = ['1/x', 'x^-3', '(x+1)/(x^2+1)', 'x^0', '-x^2', 'x*y+y^2']
for _ in range(60):
    a, b, c = (rng.randrange(-20, 21) for _ in range(3))
    n = rng.randrange(1, 8)
    expressions.append(f'({a}*x+{b})^{n}+{c}*x^2')
for expression in expressions:
    commands.append(f'derive\t{expression}\tx')
run = subprocess.run([args.cli], input='\n'.join(commands)+'\n', text=True,
                     capture_output=True, check=True, timeout=120)
rows = [line.split('\t') for line in run.stdout.splitlines()]
require(len(rows) == len(commands), 'Missing CLI results')
for command, row in zip(commands, rows):
    require(row[0] == 'ok', f'{command}: {row}')
cursor = 0
for a, b in integer_cases:
    q = abs(a)//abs(b)
    if (a < 0) != (b < 0):
        q = -q
    expected = [a+b, a-b, a*b, q, a-q*b, math.gcd(a, b)]
    require(list(map(int, rows[cursor][1:])) == expected, 'Integer oracle mismatch')
    cursor += 1
for a, b in rational_cases:
    require(list(map(fractions.Fraction, rows[cursor][1:])) == [a+b, a-b, a*b, a/b],
            'Rational oracle mismatch')
    cursor += 1
x, y = sp.symbols('x y')
for expression in expressions:
    row = rows[cursor]
    original = sp.sympify(expression.replace('^', '**'), locals={'x': x, 'y': y})
    simplified = sp.sympify(row[1].replace('^', '**'), locals={'x': x, 'y': y})
    derivative = sp.sympify(row[2].replace('^', '**'), locals={'x': x, 'y': y})
    require(sp.cancel(original-simplified) == 0, f'Simplification mismatch: {expression}')
    require(sp.cancel(sp.diff(original, x)-derivative) == 0, f'Derivative mismatch: {expression}')
    if expression in ['1/x', 'x^-3', 'x^0']:
        require('x' in row[3].split(';'), 'Missing original domain condition')
    cursor += 1
report = {'seed': 20261026, 'integer_cases': len(integer_cases),
          'rational_cases': len(rational_cases), 'symbolic_cases': len(expressions),
          'total_cases': len(commands), 'sympy_version': sp.__version__,
          'python': sys.version, 'cli_sha256': hashlib.sha256(pathlib.Path(args.cli).read_bytes()).hexdigest(),
          'limitations': ['Exact selected identities, not general algebraic equivalence',
                          'SymPy cancellation checks identities on shared domains; domain checks are separate',
                          'Not original-native-binary differential validation']}
if args.output:
    pathlib.Path(args.output).write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
