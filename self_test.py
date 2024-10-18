from sys import argv
from os import listdir
from time import time
from subprocess import run
from json import dumps
import re

def mcs():
    return time() * 1000 * 1000

if len(argv) <= 1:
    print(f'You need to specify folder with tests!')
    exit(1)

folder = argv[1].strip('/')

results = {}

start = mcs()
tests = listdir(folder)
for test in tests:
    res = run(['./dpll', f'{folder}/{test}'], capture_output=True)
    success = res.returncode == 0
    if success:
        output = res.stdout #.decode().strip('\n').split('\n')
    else:
        output = b''
    results[f'{folder}/{test}'] = {'success': success, 'output': output}
    
duration = (mcs() - start) / 1000
print(f'Finished {len(tests)} tests in {duration:.0f} ms')

exited_normally = 0
exited_abnormally = 0
passed = 0
failed = 0
SAT_tests = []
UNSAT_tests = []
unclear_tests = []

for test in results:

    results[test]['output'] = results[test]['output'].decode().strip('\n').split('\n')
    newlines = []
    for line in results[test]['output']:
        line = line.replace('\x1b[0m', '').replace('\x1b[35m', '')
        if line.startswith('>> debug from'):
            line = re.findall(r'>> debug from <[\w\d_\-.]+::\d+>: ([^\n]+)', line)[0]
        newlines.append(line)
    results[test]['output'] = newlines

    if results[test]['success']: 
        exited_normally += 1

        if results[test]['output'][-1].endswith('UNSAT'):
            UNSAT_tests.append(test)
            if test.split('/')[-1].lower().startswith('unsat') or test.split('/')[-1].lower().startswith('uuf'): passed += 1
            else: failed += 1

        elif results[test]['output'][-1].endswith('SAT'):
            SAT_tests.append(test)
            if test.split('/')[-1].lower().startswith('sat') or test.split('/')[-1].lower().startswith('uf'): passed += 1
            else: failed += 1

        else:
            unclear_tests.append(test)
            failed += 1

    else: exited_abnormally += 1

print(f'Exited normally: {exited_normally}/{exited_normally + exited_abnormally}')
print(f'Passed: {passed}/{passed + failed}')
print(f'SAT in {len(SAT_tests)} tests')
print(f'UNSAT in {len(UNSAT_tests)} tests')
print(f'Unclear output in {len(unclear_tests)} tests')

with open(f'{folder}_results.json', 'w') as res_file:
    res_file.write(
        dumps({
            'duration': f'{duration:.0f} ms',
            'passed': f'{passed}/{passed + failed}',
            'failed': f'{failed}/{passed + failed}',
            'exited_normally': f'{exited_normally}/{exited_normally + exited_abnormally}',
            'exited_abnormally': f'{exited_abnormally}/{exited_normally + exited_abnormally}',
            'SAT': f'{len(SAT_tests)}/{exited_normally}',
            'UNSAT': f'{len(UNSAT_tests)}/{exited_normally}',
            'confusing_output': f'{len(unclear_tests)}/{exited_normally}',
            'results': results
        }, ensure_ascii=False, indent=2))
