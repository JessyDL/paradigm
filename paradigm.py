from tools import build
from tools import test
from argparse import ArgumentParser

def main():
    parser = ArgumentParser(description='Generate build files for the current project.')
    parser.add_argument("--run", action='store', default="build", type=str, nargs=None,choices=['build','prepare', 'test'],dest="run")
    args, remaining_argv = parser.parse_known_args()
    
    if args.run == "build":
        target = build.Paradigm()
        target()
    elif args.run == "test":
        target = test.Tester()
        target(remaining_argv)
    elif args.run == "prepare":
        generate_project_info.generate_header()

if __name__ == "__main__":main()