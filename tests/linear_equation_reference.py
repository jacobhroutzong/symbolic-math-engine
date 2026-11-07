"""Trusted generated grammar, exact SymPy oracle plus explicit original holes."""
import argparse,fractions,hashlib,json,pathlib,random,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--probe',required=True);p.add_argument('--reference-path');p.add_argument('--output');args=p.parse_args()
if args.reference_path:sys.path.insert(0,args.reference_path)
import sympy as sp
x=sp.Symbol('x',real=True);rng=random.Random(20261104);cases=[]
for _ in range(160):
    a,b,c,d=[rng.randrange(-1000,1001) for _ in range(4)]
    lhs=f'({a})*x+({b})/7';rhs=f'({c})*x+({d})/11'
    expression=(sp.Rational(a)-c)*x+sp.Rational(b,7)-sp.Rational(d,11)
    cases.append((lhs,rhs,sp.solveset(expression,x,domain=sp.S.Reals),[],expression))
for _ in range(80):
    a,b,c,d=[rng.randrange(-20,21) for _ in range(4)];c=c or 1
    num=a*x+b;den=c*x+d
    lhs=f'(({a})*x+({b}))/(({c})*x+({d}))'
    expected=sp.Complement(sp.solveset(num,x,domain=sp.S.Reals),sp.FiniteSet(-sp.Rational(d,c)))
    cases.append((lhs,'0',expected,[-sp.Rational(d,c)],num))
for lhs,rhs,solution,holes,polynomial in [
 ('x/x','1',sp.Complement(sp.S.Reals,sp.FiniteSet(0)),[0],0),
 ('x/x','0',sp.S.EmptySet,[0],x),
 ('(x-1)/(x-1)','0',sp.S.EmptySet,[1],x-1),
 ('0*(1/x)','0',sp.Complement(sp.S.Reals,sp.FiniteSet(0)),[0],0),
 ('x^0','1',sp.Complement(sp.S.Reals,sp.FiniteSet(0)),[0],0),
 ('x^-1','2',sp.FiniteSet(sp.Rational(1,2)),[0],1-2*x),
 ('x/(x-1)','1',sp.S.EmptySet,[1],1),
 ('x-x','0',sp.S.Reals,[],0),
 ('x+1','x+2',sp.S.EmptySet,[],-1)]:cases.append((lhs,rhs,solution,holes,sp.sympify(polynomial)))
unsupported=['x*x','x^5','x^x','y*x','x^2/x','9'*129+'*x']
commands=[f'{a}\t{b}\tx' for a,b,_,_,_ in cases]+[f'{a}\t1\tx' for a in unsupported]
run=subprocess.run([args.probe],input='\n'.join(commands)+'\n',text=True,capture_output=True,timeout=120,check=True)
rows=[line.split('\t') for line in run.stdout.splitlines()];assert len(rows)==len(commands)
counts={'unique':0,'identity':0,'none':0,'unsupported':0}
for (lhs,rhs,expected,holes,poly),row in zip(cases,rows):
    status,value,conditions,coefficients,_=row
    expected_status='none' if expected==sp.S.EmptySet else 'unique' if isinstance(expected,sp.FiniteSet) else 'identity'
    assert status==expected_status,(lhs,rhs,row,expected)
    if status=='unique':assert sp.Rational(value)==list(expected)[0],(lhs,row,expected)
    if coefficients:
        observed=sum(sp.Rational(v)*x**i for i,v in enumerate(coefficients.split(';')))
        assert sp.expand(observed-poly)==0,(lhs,row,poly)
    parsed=[sp.sympify(s.replace('^','**'),locals={'x':x}) for s in conditions.split(';') if s]
    for hole in holes:assert any(v.subs(x,hole)==0 or v.subs(x,hole).has(sp.zoo,sp.nan) for v in parsed),(lhs,conditions,hole)
    counts[status]+=1
for row in rows[len(cases):]:assert row[0]=='unsupported',row;counts['unsupported']+=1
report={'seed':20261104,'sympy':sp.__version__,'cases':len(rows),'counts':counts,'probe_sha256':hashlib.sha256(pathlib.Path(args.probe).read_bytes()).hexdigest(),'scope':'Exact SymPy solve/cleared polynomial references with separately retained syntactic domain holes; not original-binary equivalence'}
if args.output:pathlib.Path(args.output).write_text(json.dumps(report,indent=2),encoding='utf-8',newline='\n')
print(json.dumps(report,indent=2))
