import bpy
import sys
import argparse


if __name__ == "__main__":
    argparser = argparse.ArgumentParser(description=__doc__)
    argparser.add_argument(
        "--input",
        metavar="i",
        default="",
        help="file (obj) to import",
    )

    args = argparser.parse_args()
    input_file: str = args.input
    file_ext: str = input_file[input_file.find(".") :]
    print(f"Reading input file {input_file} as a {file_ext} file")

    # import obj to blender
    bpy.ops.import_scene.obj(filepath=input_file, axis_forward="-Z", axis_up="Y")

    # export blender file to .blend
    bpy.ops.wm.save_as_mainfile(filepath=input_file.replace("obj", "blend"))
