#!/usr/bin/env python3

import os
import subprocess
import sys
import argparse
import glob

PRINT_READ = "\033[91m"
PRINT_RESET = "\033[0m"
VERBOSE = False
BUILD_TYPE = "armino_doc"

def run_cmd(cmd):
    if VERBOSE:
        print(cmd)
    process = subprocess.Popen(cmd, shell=True)
    process.wait()
    return process

def log_error(log):
    print(f"{PRINT_READ}{log}{PRINT_RESET}")

def print_error_lines(file_path, error_log):
    ret = False

    try:
        with open(file_path, 'r') as file:
            for line in file:
                if error_log in line:
                    if ret is False:
                        log_error(f"Error found in file: {file}")
                        ret = True

                    log_error(line.strip())

    except FileNotFoundError:
        log_error("File not found!")
        ret = True
    except Exception as e:
        log_error("An error occurred: " + str(e))
        ret = True

    return ret

def latex_error_check(path):
    #print("check path: " + path)
    ret = False

    files = glob.glob(f"{path}/*.log")

    for file in files:
        ret = print_error_lines(file, "Error:")

    return ret

def build_armino_doc(source_path, dest_path, build_path, landir, version):
    print(f"found souce: {source_path} dest: {dest_path} build: {build_path}")
    command = (
        f"make -C {source_path} arminodocs -j8 "
        f"TARGET_DIR={dest_path} TARGET_VERSION={version}"
    )
    print(f"\t{command}")

    run_cmd(f'rm -rf {build_path}/{landir}')
    run_cmd(f'rm -rf {source_path}/source')

    p = run_cmd(command)
    if p.returncode:
        log_error(f"### make arminodocs failed (rc={p.returncode}), skip publish ###")
        sys.exit(p.returncode)

    if latex_error_check(os.path.join(dest_path, "latex")) is True:
        log_error("### Build Docs Error, Exit ###")
        sys.exit(1)

    run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')

    run_cmd(f'rm -rf {os.path.join(source_path, "xml")}')
    run_cmd(f'rm -rf {os.path.join(source_path, "xml_in")}')
    run_cmd(f'rm -rf {os.path.join(source_path, "man")}')
    run_cmd(f'rm -rf {os.path.join(source_path, "..", "__pycache__")}')
    run_cmd(f'rm -rf {os.path.join(build_path, landir, "inc")}')
    run_cmd(f'rm -rf {os.path.join(build_path, landir, "latex")}')

def build_html(source_path, dest_path, build_path, landir):
	print("found souce: " + source_path + " dest: " + dest_path + " build: " + build_path)
	command = "make -C " + source_path + " arminodocs -j8 " + "TARGET_DIR=" + dest_path
	print("\t" + command)


	#clean before build
	run_cmd(f'rm -rf {build_path}/{landir}')

	if run_cmd(command) is not True:
		log_error("### Build Docs Error, Exit ###")
		exit(-1)

	#copy
	run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')
	#run_cmd(f'cp -rf {build_path}/{landir}/latex/AVDKDocument.pdf {build_path}/{landir}/AVDKDocument.pdf')
  
	#clean after build
	#run_cmd(f'rm -rf {source_path}/xml')
	#run_cmd(f'rm -rf {source_path}/xml_in')
	#run_cmd(f'rm -rf {source_path}/man')
	#run_cmd(f'rm -rf {docs_path}/__pycache__')
	#run_cmd(f'rm -rf {dest_path}')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/inc')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/latex')

def build_pdf(source_path, dest_path, build_path, landir):
	print("found souce: " + source_path + " dest: " + dest_path + " build: " + build_path)
	command = "make -C " + source_path + " latexpdf " + "TARGET_DIR=" + dest_path
	print("\t" + command)


	#clean before build
	run_cmd(f'rm -rf {build_path}/{landir}')

	run_cmd(command)

	#copy
	run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')
	#run_cmd(f'cp -rf {build_path}/{landir}/latex/AVDKDocument.pdf {build_path}/{landir}/AVDKDocument.pdf')
  
	#clean after build
	#run_cmd(f'rm -rf {source_path}/xml')
	#run_cmd(f'rm -rf {source_path}/xml_in')
	#run_cmd(f'rm -rf {source_path}/man')
	#run_cmd(f'rm -rf {docs_path}/__pycache__')
	#run_cmd(f'rm -rf {dest_path}')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/inc')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/latex')

def build_doc(target, docs_path, build_path, version):
    print("build %s docs" % target)

    subdirectories = [d for d in os.listdir(docs_path) if os.path.isdir(os.path.join(docs_path, d))]
    white_list = {"en", "zh_CN"}
    target_dirs = [x for x in subdirectories if x in white_list]

    if not os.path.exists(build_path):
        run_cmd(f'mkdir -p {build_path}')

    for subdir in target_dirs:
        source_path = os.path.join(docs_path, subdir)
        dest_path = os.path.join(docs_path, subdir, "_build")
        if BUILD_TYPE == "all":
            build_armino_doc(source_path, dest_path, build_path, subdir, version)
            # build_html(source_path, dest_path, build_path, subdir)
            # build_pdf(source_path, dest_path, build_path, subdir)
        elif BUILD_TYPE == "armino_doc":
            build_armino_doc(source_path, dest_path, build_path, subdir, version)
        elif BUILD_TYPE == "html":
            # build_html(source_path, dest_path, build_path, subdir)
            pass
        elif BUILD_TYPE == "pdf":
            # build_pdf(source_path, dest_path, build_path, subdir)
            pass
        else:
            print("Error not found build type: %s" % BUILD_TYPE)
            return

def build_all(docs_path, build_path, version):
    print("build all docs")

    subdirectories = [d for d in os.listdir(docs_path) if os.path.isdir(os.path.join(docs_path, d))]
    black_list = {"common", ".git"}
    target_dirs = [x for x in subdirectories if x not in black_list]

    for subdir in target_dirs:
        if VERBOSE:
            print("found target: " + subdir)
        build_doc(subdir, os.path.join(docs_path, subdir), os.path.join(build_path, subdir), version)

def clean_docs():
    root_path = os.getcwd()
    docs_path = os.path.join(root_path, "docs")
    build_path = os.path.join(root_path, "build", "doc", "smp_doc")
    run_cmd(f'rm -rf {build_path}')
    if not os.path.isdir(docs_path):
        return
    for soc in os.listdir(docs_path):
        soc_dir = os.path.join(docs_path, soc)
        if not os.path.isdir(soc_dir):
            continue
        run_cmd(f'rm -rf {soc_dir}/build')
        run_cmd(f'rm -rf {soc_dir}/__pycache__')
        for lan in ("en", "zh_CN"):
            lan_dir = os.path.join(soc_dir, lan)
            if not os.path.isdir(lan_dir):
                continue
            run_cmd(f'rm -rf {lan_dir}/_build')
            run_cmd(f'rm -rf {lan_dir}/xml')
            run_cmd(f'rm -rf {lan_dir}/xml_in')
            run_cmd(f'rm -rf {lan_dir}/man')
            run_cmd(f'rm -rf {lan_dir}/__pycache__')
            run_cmd(f'rm -rf {lan_dir}/examples/projects')
            run_cmd(f'rm -rf {lan_dir}/projects')
            run_cmd(f'rm -rf {lan_dir}/source')
            run_cmd(f'rm -f {lan_dir}/sphinx-warning-log.txt')
            run_cmd(f'rm -f {lan_dir}/sphinx-warning-log-sanitized.txt')
            run_cmd(f'rm -f {lan_dir}/doxygen-warning-log.txt')

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument('--clean', type=bool, default=False)
    parser.add_argument('--target', type=str, default="all")
    parser.add_argument('--type', type=str, default="armino_doc", choices=["all", "html", "pdf", "armino_doc"])
    parser.add_argument('--version', type=str, default="latest")
    parser.add_argument("--verbose", action="store_true")
    args = parser.parse_args()

    root_path = os.getcwd()
    soc_name = os.environ.get('ARMINO_SOC', 'bk7259')
    os.environ.setdefault('ARMINO_SOC', soc_name)
    build_path = os.path.join(root_path, "build", "doc", "smp_doc")
    if os.path.exists(build_path) == False:
        os.makedirs(build_path, exist_ok=True)

    if args.clean and args.target == "all":
        clean_docs()
        return

    global VERBOSE
    VERBOSE = args.verbose
    global BUILD_TYPE
    BUILD_TYPE = args.type

    root_version_path = os.path.join(root_path, "docs", "version.json")
    build_version_path = os.path.join(build_path, "version.json")
    run_cmd(f'cp {root_version_path} {build_version_path}')

    if args.clean == False and args.target == "all":
        build_all(os.path.join(root_path, "docs"), build_path, args.version)
        return

    target_path = os.path.join(root_path, "docs", args.target)
    if os.path.exists(target_path):
        build_doc(args.target, target_path, os.path.join(build_path, args.target), args.version)
    else:
        print("Error not found target: %s" % (os.path.join(root_path, "docs", args.target)))

if __name__ == "__main__":
    main(sys.argv)
