import json
import subprocess
import os

CURRENT_DIR = os.path.dirname(os.path.realpath(__file__))


def install_dependencies(target: str | None = None):
    with open(os.path.join(CURRENT_DIR, "dependencies.json")) as f:
        data = json.load(f)
        if target is None:
            # install all dependencies
            for dependencies in data.values():
                for dependency in dependencies:
                    print(f"Installing {dependency}")
                    # install the dependency using pip
                    subprocess.check_call(["pip", "install", "-q", dependency])
        else:
            for dependency in data[target]:
                # install the dependency using pip
                subprocess.check_call(["pip", "install", "-q", dependency])


if __name__ == "__main__":
    install_dependencies()
