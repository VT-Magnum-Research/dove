import os
import shlex, subprocess
import sys

ants = 1
iterations = 10
stg_path = '/Users/hamiltont/Research/Qualifier/code/STG-benchmarks/'
output_path = '/Users/hamiltont/Research/Qualifier/code/utils/benchmark.csv'
argument_variations = [" --simple ", " --elitist 0.7 ", " --rank 50 ", "  --maxmin ", " --acs "]
cores = [2, 4, 8, 16]
rounds = 2;

fout = open(output_path, 'a+')
fout.write('opt,best,median,ants,iterations,cores,tasks,time,edges,crit_path,parallelism\n');

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
    
def record_iteration(opt, best, median, ants, iterations, cores, tasks, time, edges, crit_path, parallelism):
    fout.write("%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n" % (opt, best, median, ants, iterations, cores, tasks, time, edges, crit_path, parallelism))

def getMedian(numericValues):
    theValues = sorted(numericValues)
    if len(theValues) % 2 == 1:
        return theValues[(len(theValues)+1)/2-1]
    else:
        lower = theValues[len(theValues)/2-1]
        upper = theValues[len(theValues)/2]
        return (float(lower + upper)) / 2  

for dname in os.listdir(stg_path):
    if dname[0] == ".":
        continue
    for fname in os.listdir(stg_path + dname):
        if fname[0] == ".":
            continue        
        meta = get_file_meta("%s%s/%s" % (stg_path, dname, fname))
        
        for arg in argument_variations:
            for core in cores:
                scores = []
                times = []
                
                print "Running %s/%s with %s and %d cores" % (dname, fname, arg, core)
                result = ""
                for i in range(0,rounds):
                    sys.stdout.write("\t Round %d" % i)
                    cmd = "AntHybrid %s -c %d -f %s%s/%s -m %d -i %d" % (arg, core, stg_path, dname, fname, ants, iterations)
                    cargs = shlex.split(cmd)
                    output,error = subprocess.Popen(cargs,stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
                    result = output
                    best_score = result.split(",")[0]
                    scores.append(float(best_score))
                    total_time = result.split(",")[0]
                    times.append(float(total_time))
                    print(" : %s" % best_score)
                    
                iterations = iterations
                ants = ants
                score_best = sorted(scores)[:-1][0] 
                print "Getting median of ..."
                print scores
                score_median = getMedian(scores)
                print "Median was %f" % score_median
                time_median = getMedian(times)
                edges = meta['Edges']
                edges = edges[:edges.find('/')].strip()
                tasks = dname
                core = core
                opt = meta["%d Proc." % core]
                opt = opt[opt.rfind(" "):].strip()
                parallelism = meta["Parallelism"]
                cp = meta["CP Length"]
                
                record_iteration(opt, score_best, score_median, ants, iterations, core, tasks, time_median, edges, cp, parallelism)
                
                
        
        
        
        


