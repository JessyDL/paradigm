using Paradigm.Psl.Math;
using Paradigm.Psl.Ecs;
using System;
using Paradigm.Core.Ecs.Components;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Collections.Generic;
using System.Text.Json;
using System.Linq;
using System.Text;


namespace Testing {
    class SimpleECSSystem {
        [ECSSystem]
        public void simple_ecs_test(IntPtr commandBuffer, List<(Entity, Immutable<Transform>)> transforms) {
        }
    }
}

namespace Paradigm {
    public static class Interop {
        public readonly struct PinnedStringHandle {
            public IntPtr Handle { get; }

            public PinnedStringHandle(IntPtr handle) {
                Handle = handle;
            }
        }

        public class PinnedString : IDisposable {
            private GCHandle _handle;      // Handle to the pinned object
            private IntPtr _utf8Pointer;  // Pointer to UTF-8 encoded unmanaged memory
            private bool _disposed = false;

            public PinnedString(string str) {
                if (str == null)
                    throw new ArgumentNullException(nameof(str));

                // Pin the string in memory
                _handle = GCHandle.Alloc(str, GCHandleType.Pinned);
                _utf8Pointer = IntPtr.Zero; // Will be lazily allocated if needed
            }

            // Get the pinned UTF-16 pointer
            public IntPtr Utf16Pointer => _handle.AddrOfPinnedObject();

            // Get the pinned UTF-8 pointer (lazily allocates if not already done)
            public IntPtr Utf8Pointer {
                get {
                    if (_utf8Pointer == IntPtr.Zero) {
                        string str = (string)_handle.Target;
                        byte[] utf8Bytes = Encoding.UTF8.GetBytes(str);

                        // Allocate unmanaged memory for the UTF-8 string
                        _utf8Pointer = Marshal.AllocHGlobal(utf8Bytes.Length + 1);
                        Marshal.Copy(utf8Bytes, 0, _utf8Pointer, utf8Bytes.Length);
                        Marshal.WriteByte(_utf8Pointer, utf8Bytes.Length, 0); // Null-terminate
                    }

                    return _utf8Pointer;
                }
            }

            // Implicit cast back to string for managed callers
            public static implicit operator string(PinnedString pinnedString) {
                return pinnedString?._handle.Target as string;
            }

            // Dispose method to clean up resources
            public void Dispose() {
                if (!_disposed) {
                    if (_handle.IsAllocated)
                        _handle.Free();

                    if (_utf8Pointer != IntPtr.Zero) {
                        Marshal.FreeHGlobal(_utf8Pointer);
                        _utf8Pointer = IntPtr.Zero;
                    }

                    _disposed = true;
                }
            }

            ~PinnedString() {
                Dispose();
            }
        }

        public static PinnedStringHandle CreatePinnedString(string str) {
            PinnedString pinnedString = new PinnedString(str);
            GCHandle handle = GCHandle.Alloc(pinnedString, GCHandleType.Normal);
            return new PinnedStringHandle(GCHandle.ToIntPtr(handle));
        }

        public static void FreePinnedString(PinnedStringHandle pinnedStringHandle) {
            GCHandle handle = GCHandle.FromIntPtr(pinnedStringHandle.Handle);
            if (handle.Target is PinnedString pinnedString) {
                pinnedString.Dispose();
            }
            handle.Free();
        }
    }

    namespace _Internal {
        static class Delegates {
            public delegate void void_void();
            public delegate string string_void();
            public delegate IntPtr intPtr_void();
            public delegate int int_string_string_IntPtr_int(IntPtr p0, IntPtr p1, IntPtr p2, int p3);
            public delegate int int_string_IntPtr_int(IntPtr p0, IntPtr p1, int p2);

        }

        static class Debugger {
            public static void Attach() {
                if(!System.Diagnostics.Debugger.IsAttached){
                    System.Diagnostics.Debugger.Launch();

                }
            }
        }

        static class TypeInfo {
            private static string _cachedTypeInfoBuffer; // as we cache the result for the next query invocation, we use this to only have to do the operation once
            private static string _cachedTypeInfoAssembly;
            private static string _cachedTypeInfoName;

            private static string _cachedTypesBuffer;
            private static string _cachedTypesAssembly;

            public static int GetTypeInfo(IntPtr assemblyPtr, IntPtr typeNamePtr, IntPtr outBuffer, int outBufferSize) {
                string assembly = Marshal.PtrToStringUTF8(assemblyPtr);
                string typeName = Marshal.PtrToStringUTF8(typeNamePtr);
                if (assembly != _cachedTypeInfoAssembly && typeName != _cachedTypeInfoName) {
                    var result = GetTypeInfoImpl(assembly, typeName);
                    if (result != "") {
                        _cachedTypeInfoAssembly = assembly;
                        _cachedTypeInfoName = typeName;
                        _cachedTypeInfoBuffer = result;
                    } else {
                        return -1;
                    }
                }
                if (outBuffer == IntPtr.Zero) {
                    return _cachedTypeInfoBuffer.Length;
                }

                byte[] bytes = System.Text.Encoding.UTF8.GetBytes(_cachedTypeInfoBuffer);
                int length = Math.Min(bytes.Length, outBufferSize - 1);
                Marshal.Copy(bytes, 0, outBuffer, length);
                Marshal.WriteByte(outBuffer, length, 0);
                return _cachedTypeInfoBuffer.Length;
            }

            private static string GetTypeInfoImpl(string assembly, string typeName) {
                var type = Type.GetType(typeName + ", " + assembly);
                if (type == null) {
                    return "";
                }
                var typeInfo = new Dictionary<string, object>();
                typeInfo["name"] = type.Name;
                typeInfo["namespace"] = type.Namespace;
                typeInfo["assembly"] = type.Assembly.FullName;
                var methods = new List<Dictionary<string, object>>();
                foreach (var method in type.GetMethods()) {
                    var methodInfo = new Dictionary<string, object>();
                    methodInfo["name"] = method.Name;
                    var parameters = new List<Dictionary<string, object>>();
                    foreach (var parameter in method.GetParameters()) {
                        var parameterInfo = new Dictionary<string, object>();
                        parameterInfo["name"] = parameter.Name;
                        parameterInfo["type"] = parameter.ParameterType.ToString();
                        var attributes = new List<string>();
                        foreach (var attribute in parameter.GetCustomAttributes()) {
                            attributes.Add(attribute.ToString());
                        }
                        parameterInfo["attributes"] = attributes;
                        parameters.Add(parameterInfo);
                    }
                    methodInfo["parameters"] = parameters;
                    methods.Add(methodInfo);
                }
                typeInfo["methods"] = methods;
                return JsonSerializer.Serialize(typeInfo);
            }

            public static int GetTypesInAssembly(IntPtr assemblyPtr, IntPtr outBuffer, int outBufferSize) {
                string assembly = Marshal.PtrToStringUTF8(assemblyPtr);
                if (assembly != _cachedTypesAssembly) {
                    var result = GetTypesInAssemblyImpl(assembly);
                    if (result != "") {
                        _cachedTypesAssembly = assembly;
                        _cachedTypesBuffer = result;
                    } else {
                        return -1;
                    }
                }
                if (outBuffer == IntPtr.Zero) {
                    return _cachedTypesBuffer.Length;
                }
                byte[] bytes = System.Text.Encoding.UTF8.GetBytes(_cachedTypesBuffer);
                int length = Math.Min(bytes.Length, outBufferSize - 1);
                Marshal.Copy(bytes, 0, outBuffer, length);
                Marshal.WriteByte(outBuffer, length, 0);
                return _cachedTypesBuffer.Length;
            }

            private static string GetTypesInAssemblyImpl(string assembly) {
                var types = new List<Dictionary<string, object>>();
                foreach (var type in Assembly.Load(assembly).GetTypes()) {
                    var typeInfo = new Dictionary<string, object>();
                    typeInfo["name"] = type.Name;
                    typeInfo["namespace"] = type.Namespace;
                    typeInfo["attributes"] = type.GetCustomAttributes().Select(a => a.ToString()).ToList();
                    typeInfo["methods"] = type.GetMethods().Select(m => m.Name).ToList();
                    types.Add(typeInfo);
                }
                return JsonSerializer.Serialize(types);
            }
        }
    }

    [AttributeUsage(AttributeTargets.Method)]
    public class NativeExportStaticMethod : Attribute {
        public NativeExportStaticMethod() {
        }
    }

    [AttributeUsage(AttributeTargets.Struct)]
    public class NativeStruct : Attribute {
        public NativeStruct(string name) {
            this.name = name;
        }

        public string name;
    }

    namespace Core {
        public class RendererECS : NativeState {
            public static RendererECS Instance { get; private set; } = new RendererECS();
        }
    }
    public class Example {
        public delegate bool BoolVoidDelegate();
        public static bool ConnectionCheck() {
            return true;
        }

        private static string ECSSystemInfo = "";

        [NativeExportStaticMethod]
        public static void HelloWorld() {
            Console.WriteLine("Hello World!");
        }


        [NativeExportStaticMethod]
        public static Interop.PinnedStringHandle HelloWorldReturn() {
            return Interop.CreatePinnedString("Hello World!");
        }

        [NativeExportStaticMethod]
        public static void Initialize() {
            var ecsSystemMethods = new List<Dictionary<string, object>>();
            // get all of them in all assemblies
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies()) {
                foreach (Type type in assembly.GetTypes()) {
                    foreach (MethodInfo method in type.GetMethods()) {
                        if (method.GetCustomAttributes(typeof(ECSSystem), false).Length > 0) {
                            var ecsSystemDescription = new Dictionary<string, object>();
                            ecsSystemDescription["name"] = method.Name;
                            var parameters = new List<object>();
                            // now get all their parameters's types
                            foreach (var param in method.GetParameters()) {
                                var parameter = new Dictionary<string, object>();
                                parameter["name"] = param.Name;
                                parameter["type"] = param.ParameterType.ToString().Replace("`", "/");
                                var attributes = new List<string>();
                                foreach (var attr in param.GetCustomAttributes()) {
                                    attributes.Add(attr.ToString());
                                }
                                parameter["attributes"] = attributes;
                                parameters.Add(parameter);
                            }
                            ecsSystemDescription["parameters"] = parameters;


                            ecsSystemMethods.Add(ecsSystemDescription);
                        }
                    }
                }
            }

            ECSSystemInfo = JsonSerializer.Serialize(ecsSystemMethods);
            Console.WriteLine(ECSSystemInfo);
        }

        [NativeExportStaticMethod]
        public static string GetAllEcsSystemMethodsString() {
            return ECSSystemInfo;
        }

        public delegate int GetAllEcsSystemMethodsDelegate(IntPtr buffer, int bufferSize);
        public static int GetAllEcsSystemMethods(IntPtr buffer, int bufferSize) {
            if(buffer != IntPtr.Zero) {
                byte[] bytes = System.Text.Encoding.UTF8.GetBytes(ECSSystemInfo);
                int length = Math.Min(bytes.Length, bufferSize - 1);
                Marshal.Copy(bytes, 0, buffer, length);
                Marshal.WriteByte(buffer, length, 0);
            }
            return ECSSystemInfo.Length;
        }

        public static void InitializeRendererECS() {
            Paradigm.Core.RendererECS.Instance.Declare(new Testing.SimpleECSSystem().simple_ecs_test);
        }

        [ECSSystem]
        public void simple_ecs_test(IntPtr commandBuffer, List<(Entity, Immutable<Transform>)> transforms) {
        }
    }

namespace Psl.Math {
    [StructLayout(LayoutKind.Sequential), NativeStruct("psl::tvec<float, 3>")]
    public struct Vec3 {
        public float x;
        public float y;
        public float z;
    }

    [StructLayout(LayoutKind.Sequential), NativeStruct("psl::tvec<float, 4>")]
    public struct Vec4 {
        public float x;
        public float y;
        public float z;
        public float w;
    }

    [StructLayout(LayoutKind.Sequential), NativeStruct("psl::tquat<float>")]
    public struct Quaternion {
        public float x;
        public float y;
        public float z;
        public float w;
    }
}

namespace Psl.Ecs {
    [AttributeUsage(AttributeTargets.Method)]
    public class ECSSystem : Attribute {
        public ECSSystem() {
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class OnAdd : Attribute {
        public OnAdd(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class OnRemove : Attribute {
        public OnRemove(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class OnCombine : Attribute {
        public OnCombine(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class OnBreak : Attribute {
        public OnBreak(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class Except : Attribute {
        public Except(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    [AttributeUsage(AttributeTargets.Parameter)]
    public class Filter : Attribute {
        public Filter(params Type[] types) {
            this.types = types;
        }
        public Type[] types;

        override public string ToString() {
            string[] typeNames = new string[types.Length];
            for (int i = 0; i < types.Length; i++) {
                typeNames[i] = types[i].ToString();
            }
            return this.GetType().ToString() + "[" + string.Join(",", typeNames) + "]";
        }
    }

    public struct Entity {
        public int id;
    }


    public class Pack<T> where T : class {
    }
    public class Immutable<T> where T : class {
        private readonly T _value;
        public Immutable(T value) {
            _value = value;
        }
        public T Value => _value;
    }

    public class NativeState {
        private List<Delegate> _ecsSystems = new List<Delegate>();

        public void Declare(Delegate method) {
            if (method.Method.GetCustomAttributes(typeof(ECSSystem), false).Length == 0) {
                throw new Exception("Method must have the ECSSystem attribute");
            }

            _ecsSystems.Add(method);
        }

        private void Tick() {
            foreach (var ecsSystem in _ecsSystems) {
                ecsSystem.DynamicInvoke();
            }
        }
    }
}

namespace Core.Ecs.Components {
    [StructLayout(LayoutKind.Sequential)]
    public class Transform {
        public Quaternion rotation;
        public Vec3 position;
        public Vec3 scale;
    }
    }
}