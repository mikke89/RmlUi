#!/usr/bin/env python3
"""Compiles the RmlUi SDL GPU shaders to SPIR-V, MSL and DXIL.

Every .vert and .frag in this directory is compiled to all three formats and written to compiled/<name>.h as C arrays.
One header per shader: the blobs are large, and a single file would turn any change to one shader into a diff nobody
can read.

Requires the SDL_shadercross tool. Prebuilt binaries can be found at
https://github.com/libsdl-org/SDL_shadercross/actions -- click the latest workflow and download them for your platform.

    python3 compile_shaders.py <shadercross_path>
"""

import os
import subprocess
import sys

FORMATS = ("spirv", "msl", "dxil")
OUT_DIR = "compiled"

current_dir = os.path.dirname(os.path.realpath(__file__))
out_dir = os.path.join(current_dir, OUT_DIR)


def shader_version(shadercross_path):
    """Records what produced the headers, so that a diff on regeneration can be told from a diff on a source change."""
    try:
        result = subprocess.run([shadercross_path, "--version"], capture_output=True, text=True)
        text = (result.stdout + result.stderr).strip()
        if text and "Unknown argument" not in text:
            return text.splitlines()[0]
    except OSError:
        pass
    return "unknown version"


def compile_shader(shadercross_path, source, target):
    """Returns the compiled bytes, or raises with whatever the compiler had to say."""
    temp_path = os.path.join(current_dir, ".temp.{}".format(target))
    try:
        result = subprocess.run(
            [shadercross_path, "-s", "hlsl", source, "-o", temp_path, "-d", target, "-I", current_dir],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            raise RuntimeError(
                "{} -> {} failed:\n{}".format(os.path.basename(source), target, (result.stdout + result.stderr).strip())
            )
        with open(temp_path, "rb") as f:
            return f.read()
    finally:
        if os.path.exists(temp_path):
            os.remove(temp_path)


def write_array(out, name, data):
    out.write("\nalignas(uint32_t) static const unsigned char {}[] = {{".format(name))
    for i, b in enumerate(data):
        if i % 20 == 0:
            out.write("\n\t")
        out.write("0x%02X," % b)
    out.write("\n};\n")


# Metal Shading Language is source code, not bytecode: SDL hands it to the Metal compiler as text. Writing it as a
# byte array costs five characters per byte and hides the shader from anyone reading or grepping the tree, so it goes
# in as a raw string literal instead. The delimiter is spelled out to keep the literal closed even if the source ever
# grows a `)"` of its own. Consumers must pass `sizeof(x) - 1`: Metal takes the length, and the terminator is not part
# of the shader.
def write_text(out, name, data):
    text = data.decode("utf-8")
    if ')MSL"' in text:
        raise RuntimeError("{}: the shader contains the raw string delimiter".format(name))
    out.write('\nstatic const char {}[] = R"MSL({})MSL";\n'.format(name, text))


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 compile_shaders.py <shadercross_path>")
        return 1

    shadercross_path = os.path.realpath(sys.argv[1])
    if not os.path.isfile(shadercross_path):
        print("Error: the specified shadercross path '{}' does not exist.".format(shadercross_path))
        return 1

    version = shader_version(shadercross_path)
    os.makedirs(out_dir, exist_ok=True)

    # Sorted, so that the set of generated files does not depend on the order the filesystem hands them back.
    sources = sorted(f for f in os.listdir(current_dir) if f.endswith((".vert", ".frag")))
    if not sources:
        print("Error: no shader sources found in {}.".format(current_dir))
        return 1

    names = []
    for source in sources:
        stem = os.path.splitext(source)[0]
        names.append(stem)
        blobs = []
        for target in FORMATS:
            try:
                blobs.append((target, compile_shader(shadercross_path, os.path.join(current_dir, source), target)))
            except RuntimeError as error:
                print("Error: {}".format(error))
                return 1

        out_path = os.path.join(out_dir, stem + ".h")
        with open(out_path, "w") as out:
            out.write("// Generated from {} by compile_shaders.py using {}. Do not edit manually.\n".format(source, version))
            out.write("#pragma once\n\n#include <stdint.h>\n")
            for target, data in blobs:
                name = "{}_{}".format(stem, target)
                if target == "msl":
                    write_text(out, name, data)
                else:
                    write_array(out, name, data)
        print("{} -> {}/{}.h  ({})".format(source, OUT_DIR, stem, ", ".join("{} {} B".format(t, len(d)) for t, d in blobs)))

    # An index, so that adding a shader means adding one file rather than editing the renderer's include list.
    with open(os.path.join(out_dir, "ShadersCompiled.h"), "w") as out:
        out.write("// Generated by compile_shaders.py using {}. Do not edit manually.\n".format(version))
        out.write("#pragma once\n\n")
        for name in names:
            out.write('#include "{}.h"\n'.format(name))

    print("\nWrote {} shaders to {}.".format(len(names), out_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
