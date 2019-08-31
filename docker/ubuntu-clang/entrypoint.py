import shutil
import os
import subprocess
import importlib
import argparse

def onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.

    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.

    If the error is for another reason it re-raises the error.
    
    Usage : ``shutil.rmtree(path, onerror=onerror)``
    """
    import stat
    if not os.access(path, os.W_OK):
        # Is the error an access error ?
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        print(path)
        raise
        
def getClasses(directory):
    classes = {}
    oldcwd = os.getcwd()
    os.chdir(directory)   # change working directory so we know import will work
    for filename in os.listdir(directory):
        if filename.endswith(".py"):
            modname = filename[:-3]
            classes[modname] = getattr(__import__(modname), modname)
    os.setcwd(oldcwd)
    return classes

class Project(object):
    def __init__(self):    
        parser = argparse.ArgumentParser(description='')
        parser.add_argument("--branch", default="develop", nargs=None,
                            help="set the branch of the repo", dest="branch")
        parser.add_argument("--destination", default="/paradigm", nargs=None,
                            help="set the location where the repo will be constructed", dest="location")
        parser.add_argument("--remote", default="https://github.com/JessyDL/paradigm.git", nargs=None,
                            help="set the source of the repot", dest="remote")
        self.args, self.rest = parser.parse_known_args()
        self.location = self.args.location
        self.url = self.args.remote
        self.branch = self.args.branch
        
    def initialize(self):
        build_arguments = ArgumentParser(description='Generate build files for the current project.')
        build_arguments.add_argument("-g", "--generator", const="auto", default="auto", nargs='?',
                            help="Set the generator for the project", dest="generator")
                            
    def clone(self):
        if os.path.exists(self.location):
            shutil.rmtree(self.location, onerror=onerror)
        retCode = subprocess.check_call(["git", "clone",  "--depth", "1", "-b", self.branch, self.url, self.location], shell=False)
        if retCode != 0:
            print("ERROR: could not complete the git command")
            return
        
    def build(self):
        working_dir = os.getcwd()
        os.chdir(self.location)
        module = importlib.import_module("paradigm.build")
        Paradigm = module.Paradigm
        para = Paradigm()
        para(self.rest)
        os.chdir(working_dir)
        
def main():
    proj = Project()
    proj.clone()
    proj.build()

if __name__ == "__main__":main()