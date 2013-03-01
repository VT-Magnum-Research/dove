#include <string>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h> // mkstemp

#include "helper.hpp"
#include "main_bootstrapper.hpp"

int main(int argc, char** argv) {
    using namespace std;
    for (int i = 1; i < argc; i++) {
        for (int j = 1; j < argc; j++) {
            if (i != j) {
                cout << get_latency(argv[i], argv[j]) << endl;
            }
        }
    }
    return 0;
}

std::string get_latency(std::string from, std::string to) {
    using namespace std;

    string mpirun_bin = "mpirun";
    string rankfile = make_rankfile(to, from);
    string mpiflags = "-np 2 --rankfile " + rankfile;
    string latency_bin = "latency_impl/latency";
#   ifdef LATENCY_BIN
        latency_bin = LATENCY_BIN;
#   endif
    string command = mpirun_bin + " " + mpiflags + " " + latency_bin;

    string result = exec(command);

    string regex_line = "Avg round trip time = [0-9]+";
    string regex_number = "[0-9]+";
    string microsec_line = match(result, regex_line);
    string microsec_str = match(microsec_line, regex_number);
    remove(rankfile.c_str());
    return "<cd f =\"" + from +
           "\"  t=\""   + to +
           "\"  v=\""   + microsec_str + "\" />";
}

std::string make_rankfile(std::string to, std::string from) {
    char sfn[21] = "";
    FILE* sfp;
    int fd = -1;
     
    strncpy(sfn, "/tmp/rankfile.XXXXXX", sizeof sfn);
    if ((fd = mkstemp(sfn)) == -1 ||
       (sfp = fdopen(fd, "w+")) == NULL) {
          if (fd != -1) {
            unlink(sfn);
            close(fd);
          }
       return "";
    }

    fprintf(sfp, "rank 0=10.0.2.4 slot=p0:%s\n", to.c_str());
    fprintf(sfp, "rank 1=10.0.2.4 slot=p0:%s\n", from.c_str());

    // TODO: This isn't returning a value on the stack, is it?
    return std::string(sfn);
}
