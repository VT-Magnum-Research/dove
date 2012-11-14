import jug
jug.init('jug_benchmark.py','jug_benchmark.jugdata')
import jug_benchmark
data = jug.task.value(jug_benchmark.results)
for r in data:
	print r
