#include <string>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h> // mkstemp

// Provides regex and exec 
#include "helper.hpp"
#include "main_bootstrapper.hpp"

// Command line pasing
#include "libs/tclap/CmdLine.h"
static void parse_options(int argc, char *argv[]);
static std::vector<int> sockets;
static std::vector<int> cores;

int main(int argc, char** argv) {
  try {
    parse_options(argc, argv);
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    exit(EXIT_SUCCESS);
  }
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
    return "<cd f=\"" + from +
           "\"  t=\""   + to +
           "\"  v=\""   + microsec_str + "\" />";
}

std::string make_rankfile(std::string to, std::string from) {
    char sfn[21] = ""; FILE* sfp; int fd = -1;
     
    strncpy(sfn, "/tmp/rankfile.XXXXXX", sizeof sfn);
    fd = mkstemp(sfn);
    if (fd == -1) return "";

    sfp = fdopen(fd, "w+");
    if (sfp == NULL) {
        unlink(sfn);
        close(fd);
        return "";
    }

    fprintf(sfp, "rank 0=10.0.2.4 slot=p0:%s\n", to.c_str());
    fprintf(sfp, "rank 1=10.0.2.4 slot=p0:%s\n", from.c_str());

    // TODO: This isn't returning a value on the stack, is it?
    return std::string(sfn);
}

static void parse_options(int argc, char *argv[]) {
  TCLAP::CmdLine cmd("Multi-core Deployment Optimization Model --> MPI Code Generator ", ' ', "0.1");
  TCLAP::MultiArg<int> sock_arg("s", "socket", "Each socket(processor) that should "
      "be tested. These should be the physical IDs of the sockets", true, 
      "int");
  cmd.add(sock_arg);
  TCLAP::MultiArg<int> core_arg("c", "core", "Each core that should "
      "be tested. These should be the physical IDs of the cores", true, 
      "int");
  cmd.add(core_arg);
  
  cmd.parse(argc, argv);
  sockets = sock_arg.getValue();
  cores = core_arg.getValue();
}


