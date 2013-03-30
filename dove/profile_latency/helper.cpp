#include <string>
#include <iostream>
#include <stdio.h>
#include <regex.h>

#include "helper.hpp"

// http://stackoverflow.com/questions/478898/
std::string exec(std::string cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL) {
            result += buffer;
        }
    }
    pclose(pipe);
    return result;
}

// http://stackoverflow.com/questions/5877267
std::string match(std::string str, std::string pattern) {
    int status;
    regex_t re;
    regmatch_t rm;

    if (regcomp(&re, pattern.c_str(), REG_EXTENDED) != 0) {
        return "Bad pattern";
    }
    status = regexec(&re, str.c_str(), 1, &rm, 0);
    regfree(&re);
    if (status != 0) {
        return "No match";
    }
    return std::string(str.c_str()+rm.rm_so, str.c_str()+rm.rm_eo);
}
