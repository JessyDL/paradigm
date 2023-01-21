from argparse import ArgumentParser
import os
import subprocess
from datetime import datetime

def run_git_command(command=[]):
    tag = subprocess.Popen(["git"] + command, stdout=subprocess.PIPE).stdout.read().decode('utf-8')
    return tag.strip()
    
def all_authors():
    possible_authors = run_git_command(["shortlog", "-s", "-n", "--all", "--no-merges"]).split("\n")
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
    
def generate_header(output=os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'core', 'gen', 'core', 'paradigm.hpp'), force = False):
    version = run_git_command(["tag", "-l", "--sort=-v:refname"])
    version = version.split('\n')[0]
    major, minor, patch = version.split('.')
    sha1 = run_git_command(["rev-parse", "HEAD"]).rstrip()
    unix_timestamp = run_git_command(["log", "-1", "--pretty=format:%ct"])
    utc_timestamp = datetime.utcfromtimestamp(int(unix_timestamp)).strftime('%Y-%m-%d %H:%M:%S')

    authors = all_authors()
    openmode = 'r+' if os.path.exists(output) else 'w'
    with open(output, openmode) as fObj:
        if openmode == 'r+':
            content = fObj.read()
            if not force and f'#define VERSION_SHA1 "{sha1}"' in content:
                print("header file up to date")
                return
            print("header file out of date, updating...")
            fObj.seek(0)
        else:
            print("header file missing, generating...")
        fObj.write(
f"""// generated header file don't edit.
#pragma once
#include "psl/ustring.hpp"
#include <array>

#define VERSION_TIME_UTC "{utc_timestamp}"
#define VERSION_TIME_UNIX "{unix_timestamp}"
#define VERSION_MAJOR {major}
#define VERSION_MINOR {minor}
#define VERSION_PATCH {patch}
#define VERSION_SHA1 "{sha1}"
#define VERSION "{version}.{sha1}"

constexpr static psl::string8::view APPLICATION_NAME {{"Paradigm Engine"}};
constexpr static psl::string8::view APPLICATION_FULL_NAME {{"Paradigm Engine {version}.{sha1} {utc_timestamp}"}};

constexpr static std::array<psl::string8::view, {len(authors)}> APPLICATION_CREDITS
{{
    {f'{os.linesep}    '.join(f'"{author}",' for author in authors)}
}};
""")
        fObj.truncate()

if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--output", type=str, default=os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'core', 'gen', 'core', 'paradigm.hpp'), help="Set the location/name of the output header")
    parser.add_argument("--force", action="store_true", help="Forcibly generate the output header, regardless of the state")
    args = parser.parse_args()
    
    generate_header(args.output, args.force)