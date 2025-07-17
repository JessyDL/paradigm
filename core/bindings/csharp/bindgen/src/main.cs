using System;
using System.Reflection;
using CommandLine;
using System.Collections.Generic;
using System.Linq;

namespace CsharpBindgen {
    public class Program {
        public class Options {
            [Option('v', "verbose", Required = false, HelpText = "Set output to verbose messages.")]
            public bool Verbose { get; set; }
            [Option("dll", Required = true, HelpText = "Target C# dll to reflect.")]
            public string Dll { get; set; }
            [Option('o', "output", Required = false, HelpText = "Output file to write the generated bindings to.")]
            public string Output { get; set; }
        }

        public static void Main(string[] args) {
            var result = Parser.Default.ParseArguments<Options>(args)
                .WithParsed<Options>(opts => RunOptionsAndReturnExitCode(opts))
                .WithNotParsed<Options>((errs) => HandleParseError(errs));

            
            if (args.Length != 1) {
                Console.WriteLine("Please provide the path to the dll to be used to resolve the ");
                return;
            }

            return;
        }

        private static void RunOptionsAndReturnExitCode(Options opts) {
            var dllPath = opts.Dll;
            // make sure the path is absolute
            if (!System.IO.Path.IsPathRooted(dllPath)) {
                dllPath = System.IO.Path.GetFullPath(dllPath);
            }
            // check if the file exists
            if (!System.IO.File.Exists(dllPath)) {
                throw new Exception($"File {dllPath} does not exist.");
            }
            var dll = Assembly.LoadFile(dllPath);

            try {
                // get all types in the assembly and print the attributes
                foreach (var type in dll.GetTypes()) {
                    Console.WriteLine(type.Name);
                    foreach (var attr in type.GetCustomAttributes()) {
                        Console.WriteLine(attr);
                    }
                }
            } catch (Exception ex) {
                if (ex is System.Reflection.ReflectionTypeLoadException) {
                    var typeLoadException = ex as ReflectionTypeLoadException;
                    var loaderExceptions  = typeLoadException.LoaderExceptions;

                    throw new Exception("Error loading types from assembly: " + string.Join("\n", loaderExceptions.Select(e => e.Message)));
                }

                throw;
            }
        }

        private static void HandleParseError(IEnumerable<Error> errs) {
            var error_msgs = errs.Select(e => e.ToString());
            throw new Exception("Error parsing command line arguments. " + string.Join("\n", error_msgs));
        }
    }
}