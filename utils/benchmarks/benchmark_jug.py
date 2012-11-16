from jug.task import TaskGenerator
from jug import Task
import os
import shlex
import subprocess 
from copy import deepcopy
import random

ants                = 60
iterations          = 100
alpha               = 2
beta                = 0.85
argument_variations = ["--acs --acs_q0 0.5 --acs_xi 0.1"]
cores               = [2, 4, 8, 16] 
task_heterogeneity  = [1, 2, 4, 8, 16]
core_heterogeneity  = [1, 2, 4, 8, 16]
route_heter         = [1, 2, 4, 8, 16]

# Total tasks per sample: 
#   4 core
#   5 task heter
#   5 core heter
#   5 route heter
# * 6 STG task counts
#  ________
#   3000 per             

# How many times should we run a complete analysis
samples = 1;

#stg_path    = '/home/hamiltont/STG-benchmarks'
#binary_path = '/home/hamiltont/AntHybrid'
stg_path    = '/Users/hamiltont/Research/Qualifier/code/STG-benchmarks'
binary_path = '/Users/hamiltont/Research/Qualifier/code/AntHybrid/build/Debug/AntHybrid'

def get_file_meta(file_path):
    meta = {}
    with open(file_path) as file:
        for line in file:
            if line[0] != "#":
                continue
            if not ":" in line:
                continue
            line = line[2:]
            key = line[:line.find(":")].strip()
            value = line[line.find(":")+1:].strip()
            meta[key] = value
    return meta
   
@TaskGenerator
def runACOcomplete(task):

    stg = task['stg']
    ant = task['ants']
    iter = task['iter']
    cmd = ("{binary} {args} -c {cores} -f {stg_file} "
    "-m {ants} -i {iterations} --task_heter {task_h} "
    "--route_default 1 --route_heter {route_h} "
    "--core_heter {core_h} ").format(binary     =  binary_path, 
                                     args       =  task['arg'], 
                                     cores      =  task['cores'], 
                                     stg_file   =  task['stg_path'], 
                                     ants       =  task['ants'], 
                                     iterations =  task['iter'],
                                     task_h     =  task['task_h'],
                                     route_h    =  task['route_h'],
                                     core_h     =  task['core_h']) 
    print "-----------------------------------------------------------"
    print cmd
    cargs = shlex.split(cmd)
    output,errors = subprocess.Popen(cargs,stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
    value = str(output)
    print "Output was %s" % value
    score = value.split(",")[0] 
    runtime = value.split(",")[1] 
    newTask = deepcopy(task)
    newTask.pop('stg_path')
    newTask.update({'score':float(score), 'time':float(runtime)})
    return newTask
    
tasks = []
# 6 times multiplier
for dir in os.listdir(stg_path):
    if dir[0] == ".":
        continue
    if dir == "CUST":
        continue
    
    # Arbitrary multiplier
    random.seed(42)
    for sample in range(samples): 
        stg_file = 'rand{num:=04}.stg'.format(num=random.randint(0, 178))
                          
        # 500 times multiplier
        for core in cores:
            for task_h in task_heterogeneity:
                for core_h in core_heterogeneity:
                    for route_h in route_heter:
                        path = "%s/%s/%s" % (stg_path, dir, stg_file)
                        meta = get_file_meta("%s/%s/%s" % (stg_path, dir, stg_file))
                        edges = meta['Edges']
                        edges = edges[:edges.find('/')].strip()
                        opt = meta["%d Proc." % core]
                        opt = opt[opt.rfind(" "):].strip()
                        parallelism = meta["Parallelism"]
                        cp = meta["CP Length"]
                        task_count = dir
                        
                        # Build the task inputs
                        task = {'cores'       : core,
                                'stg'         : stg_file,
                                'stg_path'    : path,
                                'ants'        : ants,
                                'iter'        : iterations,
                                'opt'         : opt,
                                'sample'      : sample,
                                'tasks'       : task_count,
                                'edges'       : edges,
                                'cp'          : cp,
                                'parallelism' : parallelism,
                                'task_h'      : task_h,
                                'core_h'      : core_h, 
                                'route_h'     : route_h}
                        tasks.append(task)
results = []
for task in tasks:
	results.append(runACOcomplete(task))

