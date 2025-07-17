import os
import subprocess
import sys
from collections import namedtuple

def _install_requirements():
    """Silently install all required packages for this script"""
    if sys.version_info[0] < 3:
        sys.exit("This script requires Python 3. Please run it with Python 3.")

    try:
        subprocess.check_call(
            [
                sys.executable,
                "-m",
                "pip",
                "install",
                "-r",
                os.path.join(
                    os.path.abspath(os.path.dirname(__file__)), "requirements.txt"
                ),
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError as e:
        print("Failed to install requirements:", e)
        sys.exit(1)

_install_requirements()

from pythonnet import load
load("coreclr")
import argparse
import clr
import System
from System.Reflection import Assembly, CustomAttributeExtensions, TypeAttributes

InteropTypeEntry = namedtuple("InteropTypeEntry", ["type", "cpp", "id"])
InteropTypeMap = {entry.type: entry for entry in [
    InteropTypeEntry(type="System.Int32", cpp="std::int32_t", id="int32"),
    InteropTypeEntry(type="System.UInt32", cpp="std::uint32_t", id="uint32"),
    InteropTypeEntry(type="System.Int64", cpp="std::int64_t", id="int64"),
    InteropTypeEntry(type="System.UInt64", cpp="std::uint64_t", id="uint64"),
    InteropTypeEntry(type="System.Single", cpp="float", id="float"),
    InteropTypeEntry(type="System.Double", cpp="double", id="double"),
    InteropTypeEntry(type="System.Boolean", cpp="bool", id="bool"),
    InteropTypeEntry(type="System.String", cpp="std::string", id="string"),
    InteropTypeEntry(type="System.IntPtr", cpp="void*", id="intPtr"),
    InteropTypeEntry(type="System.Void", cpp="void", id="void"),
    InteropTypeEntry(type="System.Char", cpp="char", id="char"),
    InteropTypeEntry(type="System.Byte", cpp="std::uint8_t", id="uint8"),
    InteropTypeEntry(type="System.SByte", cpp="std::int8_t", id="int8"),
    InteropTypeEntry(type="System.Int16", cpp="std::int16_t", id="int16"),
    InteropTypeEntry(type="System.UInt16", cpp="std::uint16_t", id="uint16"),
    InteropTypeEntry(type="System.Object", cpp="void*", id="intPtr"),
    InteropTypeEntry(type="Paradigm.Interop+PinnedStringHandle", cpp="void*", id="intPtr"),
]}

InteropUnsupportedTypeEntry = namedtuple("InteropUnsupportedTypeEntry", ["reason"])
InteropUnsupportedTypeMap = {entry.type: entry for entry in [
    InteropUnsupportedTypeEntry(type="System.String", reason="String marshalling is not supported, please use Paradigm.Interop+PinnedStringHandle"),
]}

class OutputBuffer:
    def __init__(self):
        self.buffer = ""
    def write(self, text, endline=True):
        self.buffer += (text + "\n") if endline else text
    def flush(self):
        pass
    def __str__(self):
        return self.buffer
    def __repr__(self):
        return self.buffer

class NullBuffer(OutputBuffer):
    def write(self, text, endline=True):
        pass
    def flush(self):
        pass

class PrintBuffer(OutputBuffer):
    def write(self, text, endline=True):
        print(text, end='\n' if endline else '')
    def flush(self):
        pass

class ParserBase:
    def __init__(self, buffer : OutputBuffer):
        self.buffer = buffer

class MethodParser(ParserBase):
    def __init__(self, buffer : OutputBuffer):
        super().__init__(buffer)
        pass
    def parse_method(self, typename : System.Type, method : System.Reflection.MethodInfo):
        raise NotImplementedError

class TypeParser(ParserBase):
    def __init__(self, buffer : OutputBuffer):
        super().__init__(buffer)
        pass

    def parse_type(self, type : System.Type):
        raise NotImplementedError

class NativeExportStaticMethodParser(MethodParser):
    def __init__(self, buffer : OutputBuffer):
        super().__init__(buffer)

    def _gen_return_block(self, return_type : str):
        if return_type == "void":
            return "{}"
        elif return_type == "std::string":
            return ""
        return "return {}();".format(InteropTypeMap[return_type].cpp)

    def parse_method(self, assembly : System.Reflection.Assembly, typename : System.Type, method : System.Reflection.MethodInfo):
        attributes = method.GetCustomAttributesData()
        for attr in attributes:
            if attr.AttributeType.FullName == "Paradigm.NativeExportStaticMethod":
                method_name = method.Name
                return_type = method.ReturnType.FullName
                return_type_cpp = InteropTypeMap[return_type].cpp
                parameters = method.GetParameters()

                # find the delegate that satisfies the signature
                return_delegate_name = f"Paradigm._Internal.Delegates+{InteropTypeMap[return_type].id}_{'_'.join(InteropTypeMap[param.ParameterType.FullName].id for param in parameters) if len(parameters) > 0 else 'void'}"
                cpp_delegate_type = f"{InteropTypeMap[return_type].cpp}(CORECLR_DELEGATE_CALLTYPE*)({', '.join(InteropTypeMap[param.ParameterType.FullName].cpp for param in parameters)})"

                # now search the assembly for that delegate
                assert any(tname.FullName == return_delegate_name for tname in assembly.GetTypes()), f"Delegate '{return_delegate_name}' not found in assembly '{assembly.FullName}' for {typename.FullName}:{method_name}({', '.join(param.ParameterType.FullName for param in parameters)}) -> {method.ReturnType.FullName}."
                self.buffer.write(f"// [Paradigm.NativeExportStaticMethod] {typename.FullName}:{method_name}({', '.join(param.ParameterType.FullName for param in parameters)}) -> {method.ReturnType.FullName}")
                self.buffer.write(f"""{{
    using delegate_type = {cpp_delegate_type};
    delegate_type entry_fn = nullptr;
    auto rc = get_function_pointer(L"{typename.FullName}, {assembly.GetName().Name}", L"{method_name}", L"{return_delegate_name}, {assembly.GetName().Name}", nullptr, nullptr, reinterpret_cast<void**>(&entry_fn));
    if(rc != 0 || entry_fn == nullptr) {{
        throw std::runtime_error("Failed to get function pointer for {typename.FullName}.{method_name}");
    }}
    entry_fn();
}}
""")
                
                break

class NativeStructParser(TypeParser):
    def __init__(self, buffer : OutputBuffer):
        super().__init__(buffer)

    def parse_type(self, type_name : System.Type):
        attributes = type_name.GetCustomAttributesData()
        # find the attribute who's name is "NativeStruct"
        for attr in attributes:
            if attr.AttributeType.FullName == "Paradigm.NativeStruct":
                linked_type = attr.ConstructorArguments[0].Value
                # get all fields of the type
                fields = type_name.GetFields()
                # get the bytesize of the type in C# through marshalling
                byte_size = System.Runtime.InteropServices.Marshal.SizeOf(type_name)

                struct_layout_attr = CustomAttributeExtensions.GetCustomAttribute(type_name, System.Runtime.InteropServices.StructLayoutAttribute)
                if not struct_layout_attr:
                    if type_name.Attributes & TypeAttributes.SequentialLayout:
                        pass
                    else:
                        raise ValueError(f"Type '{type_name.FullName}' must have a Sequential Layout")
                else:
                    # make sure it is SequentialLayout
                    if struct_layout_attr.Value != System.Runtime.InteropServices.LayoutKind.Sequential:
                        raise ValueError(f"Type '{type_name.FullName}' must have a Sequential Layout")

                self.buffer.write(f"// [Paradigm.NativeStruct] {type_name.FullName} : {linked_type}")
                self.buffer.write(f"""static_assert({byte_size} == sizeof({linked_type}), "Size of {type_name.Name} and {linked_type} must be equal");)""")
                for field in fields:
                    field_type = field.FieldType.FullName
                    field_name = field.Name
                    field_type = InteropTypeMap.get(field_type, field_type)
                    field_byte_size = System.Runtime.InteropServices.Marshal.SizeOf(field.FieldType)
                    field_byte_offset = System.Runtime.InteropServices.Marshal.OffsetOf(type_name, field_name)
                    self.buffer.write(f"""static_assert({field_byte_size} == sizeof({field_type}), "Size of {field_name} and {field_type} must be equal");""")
                break

def parse_library(library : str):
    if not os.path.exists(library):
        raise FileNotFoundError(f"Library '{library}' not found")

    assembly = Assembly.LoadFrom(library)

    outputBuffer = PrintBuffer()
    type_parsers = [NativeStructParser(outputBuffer)]
    method_parsers = [NativeExportStaticMethodParser(outputBuffer)]

    try:
        for typename in assembly.GetTypes():
            for parser in type_parsers:
                parser.parse_type(typename)

            for method in typename.GetMethods():
                for parser in method_parsers:
                    parser.parse_method(assembly, typename, method)
    except System.Reflection.ReflectionTypeLoadException as ex:
        for loader_exception in ex.LoaderExceptions:
            print(f"Loader Exception: {loader_exception}")
    except Exception as e:
        print(f"Error parsing library '{library}': {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parse a .NET assembly and print all types")
    parser.add_argument("library", type=str, help="The .NET assembly to parse")
    args = parser.parse_args()
    args.library = os.path.abspath(args.library)
    # clr.AddReference("System")
    parse_library(args.library)