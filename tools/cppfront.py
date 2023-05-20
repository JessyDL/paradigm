from typing import List
from argparse import ArgumentParser
import subprocess
import os

def get_path(file: str):
    filename, extension = os.path.splitext(file)
    return (file, f"{filename}.gen{extension[:-1]}") if extension in [".cpp2", ".hpp2"] else (None, None)


def clean(files : List[str]):
    for file in files:
        source, destination = get_path(file)
        if source and destination and os.path.exists(destination):
            os.remove(destination)

def generate(compiler : str, files : List[str]):
    for file in files:
        source, destination = get_path(file)
        if source and destination:
            process = subprocess.run([compiler, source, "-o", destination])
            assert(process.returncode == 0)

if __name__ == "__main__":
    argParse = ArgumentParser(description='Generate cppfront files for the current project.')
    argParse.add_argument("-i", "--input", default=[], nargs='?')
    argParse.add_argument("-c", "--compiler")
    argParse.add_argument("--clean", action='store_true')
    argParse.add_argument("--generate", action='store_true')

    args = argParse.parse_args()
    
    if args.input is not None and len(args.input) > 0:    
        if(args.clean):
            clean(args.input)
        if(args.generate):
            generate(args.compiler, args.input)
