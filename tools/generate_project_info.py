import os
import subprocess
from datetime import datetime

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
    
def all_authors():
    possible_authors = run_command(["git", "shortlog", "-s", "-n", "--all", "--no-merges"]).split("\n")
    author_exemptions = ["Travis-CI"]
    author_alias = {'JessyDL':'Jessy De Lannoit'}
    authors = {}
    for author in possible_authors:
        if author and not any(s in author for s in author_exemptions):
            author = author.strip()
            number, name = author.split(None,1)
            if name in author_alias.keys():
                name = author_alias[name]
                
            if name not in authors:
                authors[name] = number
            else:
                authors[name] += number
            
    list(sorted(authors.items()))
    return authors.keys()
    
def generate_header(force = False):
    version = run_command(["git", "tag", "-l", "--sort=-v:refname"])
    version = version.split('\n')[0]
    major, minor, patch = version.split('.')
    sha1 = run_command(["git", "rev-parse", "HEAD"]).rstrip()
    unix_timestamp = run_command(["git", "log", "-1", "--pretty=format:%ct"])
    utc_timestamp = datetime.utcfromtimestamp(int(unix_timestamp)).strftime('%Y-%m-%d %H:%M:%S')
    filepath = os.path.dirname(os.path.realpath(__file__)) +"/../core/inc/paradigm.hpp"
    authors = all_authors()
    if os.path.exists(filepath) and not force:
        fObj = open(filepath, 'r')
        content = fObj.read()
        if content.find("#define VERSION_SHA1 " + sha1):
            print("header file up to date")
            return
        print("header file out of date, updating...")
        fObj.close()
    fObj = open(filepath, 'w+')
    fObj.write("// generated header file don't edit.\n")
    fObj.write("#pragma once\n#include \"psl/ustring.h\"\n")
    fObj.write("#include \"psl/array.h\"\n\n")
    fObj.write("#define VERSION_TIME_UTC \""+utc_timestamp+"\"\n")
    fObj.write("#define VERSION_TIME_UNIX \""+unix_timestamp+"\"\n")
    fObj.write("#define VERSION_MAJOR "+major+"\n")
    fObj.write("#define VERSION_MINOR "+minor+"\n")
    fObj.write("#define VERSION_PATCH "+patch+"\n")
    fObj.write("#define VERSION_SHA1 \""+sha1+"\"\n")
    fObj.write("#define VERSION \""+version + "." +sha1+"\"\n\n")
    fObj.write("constexpr static psl::string8::view APPLICATION_NAME {\"Paradigm Engine\"};\n")
    fObj.write("constexpr static psl::string8::view APPLICATION_FULL_NAME {\"Paradigm Engine "+ version + "." +sha1+ " "+ utc_timestamp +"\"};\n")
    fObj.write("\n constexpr static std::array<psl::string8::view, "+str(len(authors))+ "> APPLICATION_CREDITS\n{{\n")
    for i, author in enumerate(authors):
        if i < len(authors) - 1:
            fObj.write('\t"' + author + '",')
        else:
            fObj.write('\t"' + author + '"')
    fObj.write("\n}};")
    fObj.truncate()
    fObj.close()

if __name__ == "__main__":
    generate_header()