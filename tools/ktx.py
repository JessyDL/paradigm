import json
import sys
import urllib.request


def parse_format(file):
    parsed_json = json.loads(file)
    formats = []

    for format in parsed_json:
        data = {
            "type": "",
            "glDesktopOnly": True,
            "typeSize": 0,
            "vkFormat": "",
            "glInternalFormat": "",
            "glFormat": "",
            "glType": "",
            "dxgiFormat": "",
            "mtlFormat": "",
            "gfxFormat": "",
        }
        if format["glDesktopOnly"]:
            continue

        if (
            not format["glType"]
            and not format["glInternalFormat"]
            and not format["glFormat"]
        ):
            continue
        data["type"] = format["type"]
        data["gles"] = not format["glDesktopOnly"]
        data["typeSize"] = format["typeSize"]
        vkFormat = ""

        is_astc = False
        for substr in format["vkFormat"][10:].split("_"):
            title = substr.title()

            if is_astc:
                title = substr.lower()
                is_astc = False

            if substr == "ASTC":
                is_astc = True
            if title == "Img":
                vkFormat += "IMG"
            elif title == "Khr":
                vkFormat += "KHR"
            else:
                vkFormat += title

        data["vkFormat"] = "vk::Format::e" + vkFormat
        data["glInternalFormat"] = format["glInternalFormat"]
        data["glFormat"] = format["glFormat"]
        data["glType"] = format["glType"]
        data["dxgiFormat"] = format["dxgiFormat"]
        data["mtlFormat"] = format["mtlFormat"]
        data["gfxFormat"] = format["vkFormat"][10:].lower()

        formats.append(data)

    return formats


def conversion_gles(formats):
    data = (
        "inline bool to_gles(format value, GLint& internalFormat, GLint& format, GLint& type) noexcept\n"
        "{\n"
        "\tinternalFormat = -1;\n"
        "\tswitch(value)\n"
        "\t{\n"
    )
    for format in formats:
        data += "\t\tcase format::" + format["gfxFormat"] + ":\n"
        if format["glInternalFormat"]:
            data += "\t\t\tinternalFormat = " + format["glInternalFormat"] + ";\n"
        else:
            data += "\t\t\tinternalFormat = 0;\n"

        if format["glFormat"]:
            data += "\t\t\tformat = " + format["glFormat"] + ";\n"
        else:
            data += "\t\t\tformat = 0;\n"

        if format["glType"]:
            data += "\t\t\ttype = " + format["glType"] + ";\n"
        else:
            data += "\t\t\ttype = 0;\n"

        data += "\t\t\tbreak;\n"

    data += "\t}\n\treturn internalFormat != -1;\n}\n"
    return data


def size_of_format(formats):
    data = (
        "inline size_t packing_size(format value) noexcept\n"
        "{\n"
        "\tswitch(value)\n"
        "\t{\n"
    )
    for format in formats:
        data += "\t\tcase format::" + format["gfxFormat"] + ":\n"
        data += "\t\t\treturn " + str(format["typeSize"]) + ";\n"
        data += "\t\t\tbreak;\n"

    data += "\t}\n\treturn 0;\n}\n"
    return data


def conversion_vk(formats):
    data = (
        "inline vk::Format to_vk(format value) noexcept\n"
        "{\n"
        "\tswitch(value)\n"
        "\t{\n"
    )
    for format in formats:
        data += "\t\tcase format::" + format["gfxFormat"] + ":\n"
        data += "\t\t\treturn " + format["vkFormat"] + ";\n"
        data += "\t\t\tbreak;\n"
    data += "\t}\n\treturn vk::Format::eUndefined;\n}\n"
    return data


if __name__ == "__main__":
    url = "https://raw.githubusercontent.com/KhronosGroup/KTX-Specification/ee688e65bb5cd6d30152cb3086df51664fdbc572/formats.json"
    if len(sys.argv) >= 2:
        url = sys.argv[1]

    formats = []
    with urllib.request.urlopen(url) as data:
        formats = parse_format(data.read().decode())
    content = ""
    content += conversion_gles(formats)
    content += conversion_vk(formats)
    content += size_of_format(formats)
    file2 = open("output", "w+")
    file2.write(content)
    file2.flush()
    file2.close()
