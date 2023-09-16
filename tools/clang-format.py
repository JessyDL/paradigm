from argparse import ArgumentParser
import io
import os
import re
import subprocess


def run_command(command=[], directory=None, print_stdout=False, catch_stdout=False):
    process = subprocess.Popen(
        command,
        stdout=os.sys.stdout if print_stdout and not catch_stdout else subprocess.PIPE,
        stderr=os.sys.stderr,
        cwd=directory,
    )
    output = []
    if catch_stdout:
        if print_stdout:
            for line in io.TextIOWrapper(process.stdout, newline=""):
                if not line.endswith("\r"):
                    output.append(
                        line[: len(line) - 1] if line[-1] == os.linesep else line
                    )
        else:
            for line in io.TextIOWrapper(process.stdout, newline=os.linesep):
                line = line.rsplit("\r", maxsplit=1)[-1]
                output.append(line[: len(line) - 1])
        process.stdout.close()
    process.wait()
    if process.returncode != 0:
        raise Exception(
            f"Raised exitcode '{process.returncode}' while trying to run the command '{' '.join(comm for comm in command)}'"
        )
    return output


def format(folders, cformat):
    if cformat is None:
        cformat = "clang-format"
    for folder in folders:
        for r, d, f in os.walk(folder):
            for file in f:
                if re.search(".*?\.(cpp|hpp|h)", file):
                    run_command(
                        [cformat, "-i", "-style=file", os.path.join(r, file)],
                        print_stdout=True,
                    )


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "--directory",
        nargs="+",
        default=[
            os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "core"),
            os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "psl"),
            os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "tests"),
            os.path.join(
                os.path.dirname(os.path.realpath(__file__)), "..", "benchmarks"
            ),
        ],
        help="Set the directories you wish to format recursively",
    )
    parser.add_argument(
        "--clang-format",
        type=str,
        default=None,
        help="Path to clang-format in case it's not in the path, or you wish to override it.",
    )
    args = parser.parse_args()

    format(args.directory, args.clang_format)
