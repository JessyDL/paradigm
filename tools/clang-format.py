from argparse import ArgumentParser
import io
import os
import re
import subprocess
import json
import platform
import asyncio

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))
PROJECT_DIR = os.path.join(CURRENT_DIR, os.path.pardir)
CONFIG_FILE = os.path.join(CURRENT_DIR, ".config.json")
DIRECTORIES = [
    os.path.join(PROJECT_DIR, "core"),
    os.path.join(PROJECT_DIR, "psl"),
    os.path.join(PROJECT_DIR, "tests"),
    os.path.join(PROJECT_DIR, "benchmarks"),
    os.path.join(PROJECT_DIR, "assembler"),
    os.path.join(PROJECT_DIR, "examples"),
]


def load_config(file: str = CONFIG_FILE):
    # https://stackoverflow.com/a/24837438
    def merge(dict1: dict, dict2: dict):
        """Recursively merges dict2 into dict1"""
        if not isinstance(dict1, dict) or not isinstance(dict2, dict):
            return dict2
        for k in dict2:
            if k in dict1:
                dict1[k] = merge(dict1[k], dict2[k])
            else:
                dict1[k] = dict2[k]
        return dict1

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
            res = merge(res, json.load(f))

            if isinstance(res["formatting"]["clang-format"], str):
                res["formatting"]["clang-format"] = [res["formatting"]["clang-format"]]
    return res


CONFIG = load_config()


async def run_command(
    command=[],
    directory=None,
    print_stdout=False,
    catch_stdout=False,
    error_out=True,
    shell=False,
    env=os.environ,
):
    process = await asyncio.create_subprocess_exec(
        *command,
        stdout=os.sys.stdout if print_stdout and not catch_stdout else subprocess.PIPE,
        stderr=os.sys.stderr if print_stdout else subprocess.PIPE,
        cwd=directory,
        shell=shell,
        env=env,
    )
    output = []
    if catch_stdout:
        async for line in process.stdout:
            output.append(line.decode().rstrip())
            if print_stdout:
                print(line.decode().rstrip())
    await process.wait()
    if error_out and process.returncode != 0:
        raise Exception(
            f"Raised exitcode '{process.returncode}' while trying to run the command '{' '.join(command)}'"
        )
    return [output, process.returncode]


async def format(cformat: str = None, dry_run: bool = False, only_staged: bool = True):
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
        [_, errorCode] = await run_command(
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
        staged_files, _ = await run_command(
            ["git", "diff", "--name-only", "--staged"],
            directory=PROJECT_DIR,
            catch_stdout=True,
        )
        print(f"Staged files: {staged_files}")
        if len(staged_files) == 1:
            staged_files = staged_files[0].split("\n")
            files = [
                file
                for file in staged_files
                if re.search(r".*?\.(cpp|hpp|h)", file)
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
            if re.search(r".*?\.(cpp|hpp|h)", file)
        ]

    marked_files = []
    files = [file for file in files if re.search(r".*?\.(cpp|hpp|h)", file)]

    tasks = []

    for file in files:
        commands = clang_format + [file, "-i", "-style=file"]
        if dry_run:
            commands.extend(["--dry-run", "-Werror"])

        tasks += [
            run_command(
                commands,
                print_stdout=not dry_run,
                error_out=not dry_run,
                directory=PROJECT_DIR,
                shell=CONFIG["formatting"]["shell"],
            )
        ]
    results = await asyncio.gather(*tasks)

    results = [result + [file] for result, file in zip(results, files)]
    for _, errorCode, file in results:
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

    asyncio.run(format(args.clang_format, dry_run=args.verify, only_staged=args.staged))
