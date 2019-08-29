import re
import functools
import os
import sys

def patch_file(filename):    
    def decorator(fn):
        @functools.wraps(fn)
        def wrapper(self):
            
            if self.filename.endswith(filename):
                print("patching ", self.filename)
                self.read()
                fn(self)
        
        wrapper._targetable = True
        return wrapper
    return decorator

class File(object):
    def __init__(self, filename):
        self.filename = filename
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
            if not self.dirty:
                self.content = self.obj.read()
                self.obj.seek(0)
            self.dirty = True
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
    @patch_file("src/spdlog.cpp")
    def patch_spdlogcpp(self):
        if not re.search(r'#include <Windows.h>', self.content):
            self.content = "#include <Windows.h>\n" + self.content

    @patch_file("include/spdlog/sinks/wincolor_sink.h")
    def patch_wincolorh(self):
        self.content = re.sub("#include <wincon.h>", "typedef void *HANDLE;\ntypedef unsigned short WORD;\n", self.content)

    @patch_file("core/core.vcxproj")
    def patch_msvc(self):
        self.content = re.sub(r'<ObjectFileName>.*</ObjectFileName>', r'<ObjectFileName>$(IntDir)%(Directory)</ObjectFileName>', self.content)
        
def patch(root):
    files = []

    for r, d, f in os.walk(root):
        for file in f:
            files.append(os.path.join(r, file))

    for f in files:
        fObj = File(f.replace('\\', '/'))
        fObj.patch()

def patch_msvc(root):
    root += "/core/core.vcxproj"
    
    fObj = File(root.replace('\\', '/'))
    fObj.patch()
    
def main():
    patch(sys.argv[1] + "/_deps/")

if __name__ == "__main__":main()