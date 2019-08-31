import shutil
import os
import subprocess
import importlib

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
    def __init__(self, location="/paradigm", url="https://github.com/JessyDL/paradigm.git", branch="develop"):
        self.location = location
        self.url = url
        self.branch = branch
        
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
        para()
        os.chdir(working_dir)
        
def main():
    proj = Project()
    proj.clone()
    proj.build()

if __name__ == "__main__":main()