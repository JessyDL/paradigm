import os
import subprocess

def run_command(command = []):
    proc = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE)
    data = ""
    while proc.poll() is None:
        output = proc.stdout.readline()
        if output:
            data = data + output.decode("utf-8") 
            
    output = proc.communicate()[0]
    if output:
        data = data + output.decode("utf-8") 
        
    return data
    
def generate_header():
    version = run_command(["git", "tag", "-l", "--sort=-v:refname"])
    print(version)
    version = version.split('\n')[0]
    print(version)
    major, minor, patch = version.split('.')
    sha1 = run_command(["git", "rev-parse", "HEAD"]).rstrip()
    
    filepath = os.path.dirname(os.path.realpath(__file__)) +"/../core/inc/paradigm.hpp"
    fObj = open(filepath, 'w+')
    fObj.write("// generated header file don't edit.\n")
    fObj.write("#pragma once\n#include \"psl/ustring.h\"\n\n")
    fObj.write("#define VERSION_MAJOR "+major+"\n")
    fObj.write("#define VERSION_MINOR "+minor+"\n")
    fObj.write("#define VERSION_PATCH "+patch+"\n")
    fObj.write("#define VERSION_SHA1 \""+sha1+"\"\n")
    fObj.write("#define VERSION \""+version + "." +sha1+"\"\n\n")
    fObj.write("constexpr static psl::string8::view APPLICATION_NAME {\"Paradigm Engine\"};\n")
    fObj.write("constexpr static psl::string8::view APPLICATION_FULL_NAME {\"Paradigm Engine "+ version + "." +sha1+"\"};\n")
    fObj.truncate()
    fObj.close()