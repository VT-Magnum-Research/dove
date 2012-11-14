from jug.task import TaskGenerator
from jug import Task
import os

ants = 10
iterations = 100
rounds = 100;
stg_path = '/home/hamiltont/STG-benchmarks/'
ant_path = '/home/hamiltont/AntHybrid'
#stg_path = '/Users/hamiltont/Research/Qualifier/code/STG-benchmarks/'
#ant_path = '/Users/hamiltont/Research/Qualifier/code/AntHybrid/build/Debug/AntHybrid'
argument_variations = ["rank 3"] #"simple", "elitist 0.7", "rank 50", "maxmin", "acs"]
cores=[2, 4, 8, 16]
results = [] 

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
   
def getMedian(numericValues):
    theValues = sorted(numericValues)
    if len(theValues) % 2 == 1:
        return theValues[(len(theValues)+1)//2-1]
    else:
        lower = theValues[len(theValues)//2-1]
        upper = theValues[len(theValues)//2]
        return (float(lower + upper)) / 2  

def runACO(arg,core,stg):
    cmd = "%s --%s -c %d -f %s -m %d -i %d" % (ant_path, arg, core, stg, ants, iterations)
    import shlex
    import subprocess 
    cargs = shlex.split(cmd)
    output,errors = subprocess.Popen(cargs,stdout=subprocess.PIPE,stderr=subprocess.PIPE).communicate()
    return output
    
def getBest(numericValues):
    values = sorted(numericValues)
    best = values[:-1]
    return best

def summarize(results):
    scores = [] 
    times = [] 
    for r in results:
        value = str(r)
        score = value.split(",")[0] 
        time = value.split(",")[1] 
        scores.append(float(score)) 
        times.append(float(time)) 
    return ({'s_best':getBest(scores), 
          's_median':getMedian(scores), 
         't_median':getMedian(times)}) 

def runTrial(rounds, path, core):
    scores = []
    times = []
    results = [Task(runACO,arg,core,path) for i in range(0,rounds)]
    return Task(summarize,results)

for dname in os.listdir(stg_path):
    if dname[0] == ".":
        continue
    for fname in os.listdir(stg_path + dname):
        if fname[0] == ".":
            continue        
                
        for arg in argument_variations:
            for core in cores:
                path = "%s%s/%s" % (stg_path,dname,fname)
                data = runTrial(rounds, path, core)
                
                # Stores data for this trial
                meta = get_file_meta("%s%s/%s" % (stg_path, dname, fname))
                edges = meta['Edges']
                edges = edges[:edges.find('/')].strip()
                opt = meta["%d Proc." % core]
                opt = opt[opt.rfind(" "):].strip()
                parallelism = meta["Parallelism"]
                cp = meta["CP Length"]
                meta = 0; # Free up meta with this many tasks
                tasks = dname
                s_best = data['s_best']
                s_median = data['s_median']
                t_median = data['t_median']
                trial = {'arg':arg, 'ants':ants, 'iter':iterations, 'cores': core, 
                         's_opt':opt, 'tasks': tasks, 'edges': edges, 'cp':cp,
                        'parallelism':parallelism, 's_best': s_best, 
                        's_median': s_median, 't_median': t_median}
                results.append(trial)
