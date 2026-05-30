#!/usr/bin/env python3
"""Convert embedded gcov serial hexdump to .gcda files and optionally generate HTML report."""
import argparse
import re
import os
import subprocess
import sys


def parse_gcov_dump(dump_file):
    with open(dump_file, 'r') as f:
        lines = f.readlines()

    files = []
    current_path = None
    current_hex = []

    for line in lines:
        line = line.rstrip()

        m = re.match(r'Emitting \d+ bytes for (.+\.gcda)', line)
        if m:
            if current_path and current_hex:
                files.append((current_path, current_hex))
            current_path = m.group(1)
            current_hex = []
            continue

        m = re.match(r'[0-9a-f]{8}: ((?:[0-9a-f]{2}\s*)+)', line)
        if m and current_path:
            hex_bytes = m.group(1).strip().split()
            current_hex.extend(hex_bytes)
            continue

    if current_path and current_hex:
        files.append((current_path, current_hex))

    return files


def write_gcda_files(files, path_replace=None):
    obj_dirs = set()
    for path, hex_bytes in files:
        if path_replace:
            old, new = path_replace
            path = path.replace(old, new)
        binary_data = bytes(int(b, 16) for b in hex_bytes)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as f:
            f.write(binary_data)
        obj_dirs.add(os.path.dirname(path))
        print(f"  Written {len(binary_data)} bytes -> {path}", file=sys.stderr)
    return obj_dirs


def run_lcov(obj_dirs, gcov_tool, output_info=None):
    dir_args = []
    for d in obj_dirs:
        dir_args += ['--directory', d]

    cmd = ['lcov', '--gcov-tool', gcov_tool, '--capture'] + dir_args
    if output_info:
        cmd += ['-o', output_info]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error: lcov failed:\n{result.stderr}", file=sys.stderr)
            sys.exit(1)
        print(f"  Generated: {output_info}", file=sys.stderr)
    else:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error: lcov failed:\n{result.stderr}", file=sys.stderr)
            sys.exit(1)
        sys.stdout.write(result.stdout)

    return output_info or result.stdout


def run_genhtml(info_file, output_dir):
    cmd = ['genhtml', info_file, '-o', output_dir, '--show-details', '--legend']
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Error: genhtml failed:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)
    print(f"  HTML report: {output_dir}/index.html", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description='Convert embedded gcov serial hexdump to coverage report.',
        epilog='''Examples:
  Generate .gcda files only (default):
    %(prog)s gcov_dump.txt

  Generate .gcda and lcov .info file:
    %(prog)s gcov_dump.txt --lcov -o coverage.info

  One-liner to HTML report:
    %(prog)s gcov_dump.txt --html coverage_html

  With path replacement:
    %(prog)s gcov_dump.txt --html coverage_html \\
        --replace-path "bk_solution_ai_dev=bk_avdk_smp_dev"
''',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument('dump_file', help='Path to serial log file containing gcov hexdump')
    parser.add_argument('--gcov-tool',
                        default='/opt/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gcov',
                        help='Path to gcov tool matching your cross-compiler '
                             '(default: arm-none-eabi-gcov from toolchain 14.3)')
    parser.add_argument('--lcov', action='store_true',
                        help='Run lcov after generating .gcda (output .info to stdout or -o FILE)')
    parser.add_argument('-o', '--output', metavar='FILE',
                        help='Write lcov .info to FILE instead of stdout (implies --lcov)')
    parser.add_argument('--html', metavar='DIR',
                        help='Generate HTML report in DIR (implies --lcov, runs genhtml)')
    parser.add_argument('--replace-path', metavar='OLD=NEW',
                        help='Replace OLD prefix with NEW in .gcda output paths')

    args = parser.parse_args()

    if not os.path.isfile(args.dump_file):
        parser.error(f"file not found: {args.dump_file}")

    path_replace = None
    if args.replace_path:
        if '=' not in args.replace_path:
            parser.error("--replace-path must be in OLD=NEW format")
        path_replace = args.replace_path.split('=', 1)

    # Step 1: Parse dump
    files = parse_gcov_dump(args.dump_file)
    if not files:
        print("No gcda data found in dump file.", file=sys.stderr)
        sys.exit(1)
    print(f"Found {len(files)} gcda file(s)", file=sys.stderr)

    # Step 2: Write .gcda files
    obj_dirs = write_gcda_files(files, path_replace)

    # If no lcov/html requested, stop here
    need_lcov = args.lcov or args.output or args.html
    if not need_lcov:
        print("\nDone. .gcda files written.", file=sys.stderr)
        return

    # Step 3: Run lcov
    info_file = args.output
    if args.html and not info_file:
        info_file = os.path.join(args.html, 'coverage.info')
        os.makedirs(args.html, exist_ok=True)

    print(f"\nRunning lcov (gcov-tool: {args.gcov_tool})...", file=sys.stderr)
    run_lcov(obj_dirs, args.gcov_tool, output_info=info_file)

    # Step 4: Run genhtml if requested
    if args.html:
        if not info_file:
            info_file = os.path.join(args.html, 'coverage.info')
        print(f"Running genhtml...", file=sys.stderr)
        run_genhtml(info_file, args.html)
        print(f"\nDone! Open {args.html}/index.html to view coverage report.", file=sys.stderr)
    elif info_file:
        print(f"\nDone! Run: genhtml {info_file} -o coverage_html", file=sys.stderr)
    else:
        print("\nDone! lcov info written to stdout.", file=sys.stderr)


if __name__ == '__main__':
    main()
