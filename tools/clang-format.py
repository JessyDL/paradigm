from argparse import ArgumentParser
import io
import os
import re
import subprocess
import json
import platform

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
PROJECT_DIR = os.path.join(CURRENT_DIR, os.path.pardir)
CONFIG_FILE = os.path.join(CURRENT_DIR, ".config.json")
DIRECTORIES = [
    os.path.join(PROJECT_DIR, "core"),
    os.path.join(PROJECT_DIR, "psl"),
    os.path.join(PROJECT_DIR, "tests"),
    os.path.join(PROJECT_DIR, "benchmarks"),
]


def load_config(file: str = CONFIG_FILE):
    res = {
        "formatting": {
            "run-on-wsl": False,
            "wsl-python": "python3",
            "shell": False,
            "clang-format": ["clang-format"],
        }
    }
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, "r") as f:
            res = res | json.load(f)
            if isinstance(res["formatting"]["clang-format"], str):
                res["formatting"]["clang-format"] = [res["formatting"]["clang-format"]]
    return res


CONFIG = load_config()


def run_command(
    command=[],
    directory=None,
    print_stdout=False,
    catch_stdout=False,
    error_out=True,
    shell=False,
    env=os.environ,
):
    process = subprocess.Popen(
        command,
        stdout=os.sys.stdout if print_stdout and not catch_stdout else subprocess.PIPE,
        stderr=os.sys.stderr if print_stdout else subprocess.PIPE,
        cwd=directory,
        shell=shell,
        env=env,
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
    if error_out and process.returncode != 0:
        raise Exception(
            f"Raised exitcode '{process.returncode}' while trying to run the command '{' '.join(comm for comm in command)}'"
        )
    return [output, process.returncode]


def format(cformat: str = None, dry_run: bool = False, only_staged: bool = True):
    print("formatting...")
    if platform.system() == "Windows" and CONFIG["formatting"]["run-on-wsl"]:
        print("running on WSL")
        command = [
            "wsl",
            "-e",
            CONFIG["formatting"]["wsl-python"],
            "tools/clang-format.py",
        ]
        if dry_run:
            command.append("--verify")
        if only_staged:
            command.append("--staged")
        [_, errorCode] = run_command(
            command,
            print_stdout=True,
            error_out=False,
            shell=False,
            directory=PROJECT_DIR,
        )

        if errorCode != 0:
            print("ERROR: clang-format should be run before committing")
            exit(1)
        return

    clang_format = CONFIG["formatting"]["clang-format"]
    if cformat is not None:
        clang_format = [cformat]

    folders = [os.path.abspath(folder) for folder in DIRECTORIES]
    files = []
    if only_staged:
        staged_files = run_command(
            ["git", "diff", "--name-only", "--staged"],
            directory=PROJECT_DIR,
            catch_stdout=True,
        )
        print(f"Staged files: {files}")
        staged_files = staged_files[0]
        if len(staged_files) == 1:
            staged_files = staged_files[0].split("\n")
            files = [
                file
                for file in staged_files
                if re.search(".*?\.(cpp|hpp|h)", file)
                and any(os.path.abspath(file).startswith(folder) for folder in folders)
            ]
    else:
        files = [
            os.path.relpath(
                os.path.abspath(os.path.join(r, file)), PROJECT_DIR
            ).replace("\\", "/")
            for folder in folders
            for r, d, f, in os.walk(folder)
            for file in f
            if re.search(".*?\.(cpp|hpp|h)", file)
        ]

    marked_files = []
    files = [file for file in files if re.search(".*?\.(cpp|hpp|h)", file)]

    for file in files:
        commands = clang_format + [file, "-i", "-style=file"]
        if dry_run:
            commands.extend(["--dry-run", "-Werror"])

        [_, errorCode] = run_command(
            commands,
            print_stdout=not dry_run,
            error_out=not dry_run,
            directory=PROJECT_DIR,
            shell=CONFIG["formatting"]["shell"],
        )
        if dry_run and errorCode != 0:
            marked_files.append(file)

    if dry_run and len(marked_files) > 0:
        marked_file_str = ", ".join(f"'{file}'" for file in marked_files)
        print(
            f"ERROR: clang-format should be run before committing on the following files: {marked_file_str}"
        )
        exit(1)

    print("formatting finished")


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument(
        "--staged", action="store_true", help="Check only staged files instead of all"
    )
    parser.add_argument(
        "--clang-format",
        type=str,
        default=None,
        help="Path to clang-format in case it's not in the path, or you wish to override it.",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify that the code is formatted correctly",
    )
    args = parser.parse_args()

    format(args.clang_format, dry_run=args.verify, only_staged=args.staged)
