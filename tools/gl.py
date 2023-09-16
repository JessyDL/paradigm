import xml.etree.ElementTree as ET

# allows to generate a header file that defines all GL related enums that have _not_ been defined
# yet by the implementation.
# this is useful for handling (for example) texture formats that have not been exposed yet due to
# unused extensions.


def parse(input="gl.xml", supported="gles2", output="gl.h"):
    root = ET.parse(input).getroot()

    enums = {}

    for enum in root.findall("enums/enum"):
        enums[enum.get("name")] = enum.get("value")

    enum_to_ext_mapping = {}
    for extension in root.findall("extensions/extension"):
        if supported in extension.get("supported"):
            for enum in extension.findall("require/enum"):
                if enum.get("name") in enum_to_ext_mapping:
                    enum_to_ext_mapping[enum.get("name")].append(extension.get("name"))
                else:
                    enum_to_ext_mapping[enum.get("name")] = [extension.get("name")]

    extension_to_enum_mapping = {}
    content = (
        "#if defined(GL_ENABLE_ALL_DEFINES) && !defined(_INCL_GL_ENABLE_ALL_DEFINES)\n"
    )
    content += "#define _INCL_GL_ENABLE_ALL_DEFINES\n"
    for enum in enum_to_ext_mapping.keys():
        extension = tuple(enum_to_ext_mapping[enum])
        if extension in extension_to_enum_mapping:
            extension_to_enum_mapping[extension].append(enum)
        else:
            extension_to_enum_mapping[extension] = [enum]

    for extension_tuple in extension_to_enum_mapping.keys():
        content += "#if "
        for i, extension in enumerate(extension_tuple):
            content += "!defined(" + extension + ")"
            if i == len(extension_tuple) - 1:
                content += "\n"
            else:
                content += " && "

        for enum in extension_to_enum_mapping[extension_tuple]:
            content += "#define " + enum + " " + enums[enum] + "\n"

        content += "#endif // " + " && ".join(extension_tuple) + "\n\n"
    content += "#endif\n"
    file = open(output, "w+")
    file.write(content)
    file.flush()
    file.close()
