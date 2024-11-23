using Psl.Math;
using Psl.Ecs;
using System;
using Core.Ecs.Components;
using System.Runtime.InteropServices;
using System.Reflection;
using System.Collections.Generic;
using System.Text.Json;

namespace Paradigm {
    public class Example {
        public delegate bool BoolVoidDelegate();
        public static bool ConnectionCheck() {
            return true;
        }

        private static string ECSSystemInfo = "";

        public delegate void VoidVoidDelegate();
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

        [ECSSystem]
        public void simple_ecs_test(IntPtr commandBuffer, [OnAdd(typeof(Transform))] List<(Entity, Immutable<Transform>)> transforms) {
        }
    }
}

namespace Psl.Math {
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3 {
        public float x;
        public float y;
        public float z;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Vec4 {
        public float x;
        public float y;
        public float z;
        public float w;
    }

    [StructLayout(LayoutKind.Sequential)]
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
    public class Component : Attribute {
        public Component(int id = 0, bool isConst = true) {
            this.id = id;
            this.isConst = isConst;
        }

        public int id;
        public bool isConst;
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
}

namespace Core.Ecs.Components {
    [StructLayout(LayoutKind.Sequential)]
    public class Transform {
        public Quaternion rotation;
        public Vec3 position;
        public Vec3 scale;
    }
}