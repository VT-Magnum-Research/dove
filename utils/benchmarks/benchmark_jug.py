from jug.task import TaskGenerator
from jug import Task
import os
import shlex
import subprocess 
from copy import deepcopy

ants = 15
iterations = 10
rounds = 4
stg_path = '/home/hamiltont/STG-benchmarks/'
ant_path = '/home/hamiltont/AntHybrid'
#stg_path = '/Users/hamiltont/Research/Qualifier/code/STG-benchmarks/'
#ant_path = '/Users/hamiltont/Research/Qualifier/code/AntHybrid/build/Debug/AntHybrid'
argument_variations = ["simple"] #, "rank 5", "elitist 0.7", "rank 50", "maxmin", "acs"]
cores=[2, 4]#, 8, 16] 

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
   
'''def getMedian(numericValues):
    theValues = sorted(numericValues)
    if len(theValues) % 2 == 1:
        return theValues[(len(theValues)+1)//2-1]
    else:
        lower = theValues[len(theValues)//2-1]
        upper = theValues[len(theValues)//2]
        return (float(lower + upper)) / 2  

@TaskGenerator
def runACO(arg,core,stg):
    cmd = "%s --%s -c %d -f %s -m %d -i %d" % (ant_path, arg, core, stg, ants, iterations)
    print "-----------------------------------------------------------"
    print cmd
    import shlex
    import subprocess 
    cargs = shlex.split(cmd)
    output,errors = subprocess.Popen(cargs,stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
    print "Output was %s" % output
    return output'''
    
@TaskGenerator
def runACOcomplete(task):
    arg = task['arg']
    core = task['cores']
    stg = task['stg']
    ant = task['ants']
    iter = task['iter']
    cmd = "%s --%s -c %d -f %s -m %d -i %d" % (ant_path, arg, core, stg, ant, iter)
    print "-----------------------------------------------------------"
    print cmd
    cargs = shlex.split(cmd)
    output,errors = subprocess.Popen(cargs,stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
    value = str(output)
    print "Output was %s" % value
    score = value.split(",")[0] 
    runtime = value.split(",")[1] 
    newTask = deepcopy(task)
    newTask.pop('stg')
    newTask.update({'score':float(score), 'time':float(runtime)})
    return newTask
    
'''@TaskGenerator
def summarize(results):
    scores = [] 
    times = [] 
    for r in results:
        value = str(r)
        score = value.split(",")[0] 
        time = value.split(",")[1] 
        scores.append(float(score)) 
        times.append(float(time))
        print "\tAchieved %s in  %s cpu cycles" % (score,time)
     print "Best was %f, s_medians %5.5f, t_median %5.5f" % (min(scores),getMedian(scores),getMedian(times))
    return ({'s_best':min(scores), 
          's_median':getMedian(scores), 
         't_median':getMedian(times)}) '''

tasks = []
for dname in os.listdir(stg_path):
    if dname[0] == ".":
        continue
    for fname in os.listdir(stg_path + dname):
        if fname[0] == ".":
            continue        
                
        for arg in argument_variations:
            for core in cores:
                path = "%s%s/%s" % (stg_path,dname,fname)
                meta = get_file_meta("%s%s/%s" % (stg_path, dname, fname))
                edges = meta['Edges']
                edges = edges[:edges.find('/')].strip()
                opt = meta["%d Proc." % core]
                opt = opt[opt.rfind(" "):].strip()
                parallelism = meta["Parallelism"]
                cp = meta["CP Length"]
                task_count = dname

                # Build the task inputs
                task = {'cores':core,'arg':arg,'stg':path,'ants':ants,
                        'iter':iterations,'opt':opt,'tasks':task_count,'edges':edges,
                        'cp':cp,'parallelism':parallelism}
                for round in range(0, rounds):
                    tasks.append(deepcopy(task))
results = []
for task in tasks:
	results.append(runACOcomplete(task))

