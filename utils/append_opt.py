
HTML_file_loc = 'opt.html'
STG_base_directory = '/Users/hamiltont/Research/Qualifier/code/STG-benchmarks/'
table_title = "Optimal schedules for task graphs "
data_marker = "<B><P ALIGN=CENTER>"
data_terminal_marker = "</P></B></TD> "

disclaimer = """# 
# Optimal schedules found using PDF/IHS running on Sun Ultra80 with
# 4 Ultra SPARC IIs 450MHz CPU and 1GB shared main memory, allowed 
# 10 minutes of CPU. See http://www.kasahara.elec.waseda.ac.jp/sche
# dule/optim_v2.html for more info. 
# 
"""

processed_files = []
current_meta = ""

file = ""
processor = ""
opt = ""
upper_bound = ""

f_opt = open(HTML_file_loc)
for line in f_opt:
    '''if table_title in line:
        current_meta = line[line.find(table_title)+len(table_title):]
        current_meta = current_meta[current_meta.find(", ") + 2:]
        current_meta = current_meta[:current_meta.find(")")]
        continue'''
    
    if data_marker in line:
        data = line[len(data_marker):-len(data_terminal_marker)]
        if file == "":
            file = data
        elif processor == "":
            processor = data
        elif opt == "":
            opt = data
            if "-" in data:
                upper_bound = data[:data.find("-")]
                opt = data[data.find("-")+1:]
            else:
                opt = data
                upper_bound = opt
            
            # All data found
            try:
                with open(STG_base_directory + file, "a") as data:
                    if not file in processed_files:
                        data.write(disclaimer)
                        processed_files.append(file)
                    
                    data_line = "#  {0:<18}: {1:>8} {2:>8}\n".format(processor + " Proc.", upper_bound , opt)
                    
                    data.write(data_line);
                    print data_line
            except:
                pass
                    
            file = ""
            processor = ""
            opt = ""
            upper_bound = ""
            
            
            