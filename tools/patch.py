from argparse import ArgumentParser
import codecs
import re
import functools
import os
import sys

def patch_file(filename):    
    def decorator(fn):
        @functools.wraps(fn)
        def wrapper(self):
            
            if self.filename.endswith(filename):
                self.read()
                temp = self.content
                fn(self)
                if temp != self.content:
                    print(f"patched {self.filename}")
                    self.dirty = True
        
        wrapper._targetable = True
        return wrapper
    return decorator

class File(object):
    def __init__(self, filename):
        if not os.path.exists(filename):
            raise Exception(f"The file at '{filename}' could not be found, did you supply an incorrect root directory?")
        self.filename = filename
        self.is_read = False
        self.dirty = False
        self.content = ""
        try:
            self.obj = open(self.filename, 'r+')
        except:
            self.obj = []
            pass

    def patch(self):
        for key, value in self.__class__.__dict__.items():
            if(getattr(value, '_targetable', False)):
                getattr(self, key)()

    def __del__(self):
        if self.dirty:
            self.obj.write(self.content)
            self.obj.truncate()
            self.obj.close()

    def read(self):
        try:
            if not self.is_read:
                self.content = self.obj.read()
                self.obj.seek(0)
            self.is_read = True
        except:
             pass

    @patch_file("include/vulkan/vulkan.h")
    def patch_vulkan(self):
        self.content = re.sub('^#include <windows.h>$', \
          r'''typedef unsigned long DWORD;\n'''
          r'''typedef const wchar_t* LPCWSTR;\n'''
          r'''typedef void* HANDLE;\n'''
          r'''typedef struct HINSTANCE__* HINSTANCE;\n'''
          r'''typedef struct HWND__* HWND;\n'''
          r'''typedef struct HMONITOR__* HMONITOR;\n'''
          r'''typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;''', \
          self.content, flags = re.M)

    @patch_file("core/core.vcxproj")
    def patch_msvc(self):
        self.content = re.sub(r'<ObjectFileName>.*</ObjectFileName>', r'<ObjectFileName>$(IntDir)%(Directory)</ObjectFileName>', self.content)
        
def patch(root):
    patch_includes(root)
    patch_msvc(root)
    
def patch_includes(root):
    root = root.replace('\\', '/') + "/_deps/"
    files = []

    for r, d, f in os.walk(root):
        for file in f:
            files.append(os.path.join(r, file))

    for f in files:
        fObj = File(f.replace('\\', '/'))
        fObj.patch()

def patch_msvc(root):
    root = root.replace('\\', '/')
    
    if os.path.isfile(os.path.join(root, "core/core.vcxproj")):
        fObj = File(os.path.join(root, "core/core.vcxproj"))
    elif os.path.isfile(os.path.join(root, "paradigm/core/core.vcxproj")):
        fObj = File(os.path.join(root, "paradigm/core/core.vcxproj"))
    else:
        raise Exception(f"No project files found at path '{root}', did you supply the correct path?")
    fObj.patch()

def patch_utf8(folders):
    for folder in folders:
        for r, d, f in os.walk(folder):
            for file in f:
                try:
                    BUFSIZE = 4096
                    BOMLEN = len(codecs.BOM_UTF8)

                    with open(os.path.join(r, file), "r+b") as fp:
                        chunk = fp.read(BUFSIZE)
                        if chunk.startswith(codecs.BOM_UTF8):
                            i = 0
                            chunk = chunk[BOMLEN:]
                            while chunk:
                                fp.seek(i)
                                fp.write(chunk)
                                i += len(chunk)
                                fp.seek(BOMLEN, os.SEEK_CUR)
                                chunk = fp.read(BUFSIZE)
                            fp.seek(-BOMLEN, os.SEEK_CUR)
                            fp.truncate()
                except:
                    continue

if __name__ == "__main__":
    parser = ArgumentParser(description='Patch project files.')
    parser.add_argument("--project",  
                    help="Override for the project files directory, this is relative to the root")
    parser.add_argument("--utf8", nargs='+', 
                    help="Override for the project files directory, this is relative to the root")
    args = parser.parse_args()

    if args.project:
        patch(args.project)
    if args.utf8:
        patch_utf8(args.utf8)