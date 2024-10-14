from sys import argv
from os import listdir
from time import time
from subprocess import run
from json import dumps

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
        output = res.stdout.decode().strip('\n').split('\n')
    else:
        output = ''
    results[f'{folder}/{test}'] = {'success': success, 'output': output}
    
duration = (mcs() - start) / 1000
print(f'Passed {len(tests)} tests in {duration:.0f} ms')

succ_count = 0
err_count = 0
SAT_tests = []
UNSAT_tests = []
unclear_tests = []

for test in results:
    if results[test]['success']: 
        succ_count += 1

        if results[test]['output'][-1].endswith('UNSAT'):
            UNSAT_tests.append(test)
        elif results[test]['output'][-1].endswith('SAT'):
            SAT_tests.append(test)
        else:
            unclear_tests.append(test)
    else: err_count += 1

print(f'Completed correctly: {succ_count}/{succ_count + err_count}')
print(f'SAT in {len(SAT_tests)} tests')
print(f'UNSAT in {len(UNSAT_tests)} tests')
print(f'Unclear output in {len(unclear_tests)} tests')

with open(f'{folder}_results.json', 'w') as res_file:
    res_file.write(
        dumps({
            'duration': f'{duration:.0f} ms',
            'completed': f'{succ_count}/{succ_count + err_count}',
            'terminated': f'{err_count}/{succ_count + err_count}',
            'SAT': f'{len(SAT_tests)}/{succ_count}',
            'UNSAT': f'{len(UNSAT_tests)}/{succ_count}',
            'confusing_output': f'{len(unclear_tests)}/{succ_count}',
            'results': results
        }, ensure_ascii=False, indent=2))
