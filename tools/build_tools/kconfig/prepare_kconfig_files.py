#!/usr/bin/env python
#
# SPDX-FileCopyrightText: 2019-2021 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

from __future__ import print_function, unicode_literals

import argparse
import json
import sys
import os
from io import open
import re


def _prepare_source_files(env_dict, list_separator):
    """
    Prepares source files which are sourced from the main Kconfig because upstream kconfiglib doesn't support sourcing
    a file list. The inputs are the same environment variables which are used by kconfiglib:
        - COMPONENT_KCONFIGS,
        - COMPONENT_KCONFIGS_SOURCE_FILE,
        - COMPONENT_KCONFIGS_PROJBUILD,
        - COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE.

    The outputs are written into files pointed by the value of
        - COMPONENT_KCONFIGS_SOURCE_FILE,
        - COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE,

    After running this function, COMPONENT_KCONFIGS_SOURCE_FILE and COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE will
    contain a list of source statements based on the content of COMPONENT_KCONFIGS and COMPONENT_KCONFIGS_PROJBUILD,
    respectively. For example, if COMPONENT_KCONFIGS="var1;var2;var3" and
    COMPONENT_KCONFIGS_SOURCE_FILE="/path/file.txt" then the content of file /path/file.txt will be:
        source "var1"
        source "var2"
        source "var3"

    The character used to delimit paths in COMPONENT_KCONFIGS* variables is set using --list-separator option.
    Space separated lists are currently only used by the documentation build system (esp-docs).
    """

    def _dequote(var):
        return var[1:-1] if len(var) > 0 and (var[0], var[-1]) == ('"',) * 2 else var

    def _write_source_file(config_var, config_file):
        dequoted_var = _dequote(config_var)
        if dequoted_var:
            new_content = '\n'.join(['source "{}"'.format(path) for path in dequoted_var.split(list_separator)])
        else:
            new_content = ''

        try:
            with open(config_file, 'r', encoding='utf-8') as f:
                old_content = f.read()
        except Exception:
            # File doesn't exist or other issue
            old_content = None
            # "None" ensures that it won't be equal to new_content when it is empty string because files need to be
            # created for empty environment variables as well

        if new_content != old_content:
            # write or rewrite file only if it is necessary
            with open(config_file, 'w', encoding='utf-8') as f:
                f.write(new_content)

    def _parse_kconfig_group(kconfig_path):
        """
        Parse KCONFIG_GROUP marker from Kconfig file.
        Supports both '# KCONFIG_GROUP:' and '#KCONFIG_GROUP:' formats.
        Returns group name if found, None otherwise.
        """
        try:
            with open(kconfig_path, 'r', encoding='utf-8') as f:
                # Read first 10 lines to find the marker
                for i, line in enumerate(f):
                    if i > 10:  # Only check first 10 lines
                        break
                    line = line.strip()
                    # Support both '# KCONFIG_GROUP:' and '#KCONFIG_GROUP:' formats
                    if line.startswith('#KCONFIG_GROUP:') or line.startswith('# KCONFIG_GROUP:'):
                        # Extract group name - handle both formats
                        if line.startswith('#KCONFIG_GROUP:'):
                            group = line.replace('#KCONFIG_GROUP:', '').strip()
                        else:
                            group = line.replace('# KCONFIG_GROUP:', '').strip()
                        return group
        except Exception:
            pass
        return None

    def _group_components_by_marker(kconfig_paths, list_separator):
        """
        Group Kconfig files by their KCONFIG_GROUP marker.
        Returns dict: {group_name: [kconfig_paths]}, list of ungrouped paths
        """
        groups = {}
        ungrouped = []
        
        for kconfig_path in kconfig_paths.split(list_separator):
            kconfig_path = kconfig_path.strip().strip('"')
            if not kconfig_path:
                continue
            
            group = _parse_kconfig_group(kconfig_path)
            if group:
                if group not in groups:
                    groups[group] = []
                groups[group].append(kconfig_path)
            else:
                ungrouped.append(kconfig_path)
        
        return groups, ungrouped

    def _collect_sourced_kconfig_paths(kconfig_paths, armino_path):
        """Return Kconfig files directly sourced by the given grouped Kconfigs."""
        sourced_paths = set()
        source_re = re.compile(r'^\s*source\s+"([^"]+)"')
        for kconfig_path in kconfig_paths:
            try:
                with open(kconfig_path, 'r', encoding='utf-8') as f:
                    for line in f:
                        match = source_re.search(line)
                        if not match:
                            continue
                        source_path = match.group(1)
                        if armino_path:
                            source_path = source_path.replace('${ARMINO_PATH}', armino_path)
                        sourced_paths.add(os.path.normpath(source_path))
            except Exception:
                continue
        return sourced_paths

    def _menu_sort_key(menu_name):
        preferred = {
            # Device Drivers: keep board/input/sensor/USB ahead of monitor-only
            # helpers, instead of pure alphabetical order.
            'Board Devices': 10,
            'Keys & Buttons': 20,
            'Touch': 30,
            'Sensors': 40,
            'USB': 50,
            'Battery Monitor': 90,
            # System Services: show boot/system-facing services before
            # application helpers and utility libraries.
            'Startup & Initialization': 110,
            'Console & Event Services': 120,
            'Identity': 130,
            'Application Services': 140,
            'Compatibility Layer': 150,
            'Utilities & Libraries': 160,
        }
        return (0, preferred[menu_name], menu_name) if menu_name in preferred else (1, menu_name)

    def _generate_nested_menu_structure(content_lines, menu_tree, armino_subsys_dir, indent_level=0):
        """
        Generate nested menu structure from menu tree.
        menu_tree structure: {
            'components': [list of kconfig paths],  # Components at this level
            'children': {                           # Child menus
                'MenuName': {menu_tree},
                ...
            }
        }
        """
        indent = '    ' * indent_level
        
        # First, add components at current level
        if menu_tree.get('components'):
            for kconfig_path in sorted(menu_tree['components']):
                # Convert absolute path to relative path using the active
                # subsystem root. ARMINO_PATH points at ap/ or cp/.
                if armino_subsys_dir and kconfig_path.startswith(armino_subsys_dir):
                    rel_path = kconfig_path.replace(armino_subsys_dir, '${ARMINO_PATH}', 1)
                else:
                    rel_path = kconfig_path
                content_lines.append('{}source "{}"'.format(indent, rel_path))
        
        # Then, add child menus
        if menu_tree.get('children'):
            for menu_name in sorted(menu_tree['children'].keys(), key=_menu_sort_key):
                child_tree = menu_tree['children'][menu_name]
                content_lines.append('{}menu "{}"'.format(indent, menu_name))
                content_lines.append('')
                _generate_nested_menu_structure(content_lines, child_tree, armino_subsys_dir, indent_level + 1)
                content_lines.append('')
                content_lines.append('{}endmenu'.format(indent))

    def _generate_group_kconfig_files(groups, armino_subsys_dir, group_kconfigs_dir, special_group_outputs=None):
        """
        Generate group Kconfig files automatically.
        Supports multi-level grouping using '::' separator (e.g., "Demos::Peripheral::Touch").
        Returns list of generated file paths.
        """
        os.makedirs(group_kconfigs_dir, exist_ok=True)
        
        generated_files = []
        special_group_outputs = special_group_outputs or {}
        
        # Separate flat groups and nested groups
        flat_groups = {}
        nested_groups = {}
        
        for group_name, kconfig_paths in groups.items():
            if '::' in group_name:
                # Multi-level group
                menu_path = [part.strip() for part in group_name.split('::')]
                # Use the full path as key to avoid conflicts
                path_key = '::'.join(menu_path)
                if path_key not in nested_groups:
                    nested_groups[path_key] = {'path': menu_path, 'components': []}
                nested_groups[path_key]['components'].extend(kconfig_paths)
            else:
                # Flat group
                flat_groups[group_name] = kconfig_paths
        
        # Merge flat groups and nested groups by top-level menu name
        # This ensures that groups with the same top-level name (e.g., "Demos" and "Demos::Net")
        # are merged into a single file
        merged_top_level_groups = {}
        
        # Process flat groups - treat them as top-level groups
        for group_name in sorted(flat_groups.keys()):
            if group_name not in merged_top_level_groups:
                merged_top_level_groups[group_name] = {'components': [], 'children': {}}
            # Add flat group components directly to top level
            merged_top_level_groups[group_name]['components'].extend(flat_groups[group_name])
        
        # Process nested groups - merge by top-level menu
        for path_key, group_info in nested_groups.items():
            top_level = group_info['path'][0]
            if top_level not in merged_top_level_groups:
                merged_top_level_groups[top_level] = {'components': [], 'children': {}}
            # Build menu tree structure
            current = merged_top_level_groups[top_level]
            menu_path = group_info['path'][1:]  # Skip top level
            for menu_name in menu_path:
                if 'children' not in current:
                    current['children'] = {}
                if menu_name not in current['children']:
                    current['children'][menu_name] = {'components': [], 'children': {}}
                current = current['children'][menu_name]
            # Add components to leaf node
            if 'components' not in current:
                current['components'] = []
            current['components'].extend(group_info['components'])
        
        # Generate one file per top-level group (merged). Some top-level groups
        # are embedded manually by build_main_ap.kconfig so they are emitted as
        # menu bodies instead of standalone top-level menus.
        for group_name, output_file in special_group_outputs.items():
            os.makedirs(os.path.dirname(output_file), exist_ok=True)
            if group_name not in merged_top_level_groups:
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write('# No {} grouped components\n'.format(group_name))

        for top_level in sorted(merged_top_level_groups.keys()):
            if top_level in special_group_outputs:
                output_file = special_group_outputs[top_level]
                content_lines = []
                _generate_nested_menu_structure(
                    content_lines,
                    merged_top_level_groups[top_level],
                    armino_subsys_dir,
                    0
                )
                with open(output_file, 'w', encoding='utf-8') as f:
                    f.write('\n'.join(content_lines))
                continue

            group_file = os.path.join(group_kconfigs_dir, '{}_group.kconfig'.format(top_level.lower().replace(' ', '_').replace('::', '_')))
            content_lines = []
            
            # Start with top-level menu
            content_lines.append('menu "{}"'.format(top_level))
            content_lines.append('')
            
            # Generate nested menu structure from merged tree
            menu_tree = merged_top_level_groups[top_level]
            _generate_nested_menu_structure(content_lines, menu_tree, armino_subsys_dir, 1)
            
            content_lines.append('')
            content_lines.append('endmenu')
            
            with open(group_file, 'w', encoding='utf-8') as f:
                f.write('\n'.join(content_lines))
            
            generated_files.append(group_file)
        
        return generated_files

    def _generate_group_index_file(group_kconfigs_dir, group_files):
        """
        Generate index file that sources all group Kconfig files.
        Returns path to index file, or None if no groups.
        """
        os.makedirs(group_kconfigs_dir, exist_ok=True)
        index_file = os.path.join(group_kconfigs_dir, 'group_index.kconfig')
        
        if not group_files:
            # Create empty file if no groups
            with open(index_file, 'w', encoding='utf-8') as f:
                f.write('# No grouped components\n')
            return index_file
        
        content_lines = []
        def _group_order(path):
            name = os.path.basename(path)
            preferred = [
                'multimedia_group.kconfig',
                'storage_group.kconfig',
                'power_management_group.kconfig',
                'security_&_cloud_group.kconfig',
            ]
            try:
                return (0, preferred.index(name), name)
            except ValueError:
                return (1, len(preferred), name)

        # Ensure group_kconfigs_dir is absolute
        group_kconfigs_dir_abs = os.path.abspath(os.path.normpath(group_kconfigs_dir))
        # Use set to deduplicate file paths
        seen_files = set()
        for group_file in sorted(group_files, key=_group_order):
            # Use absolute path for source statement
            # Ensure path is absolute and normalized
            if os.path.isabs(group_file):
                abs_path = os.path.normpath(group_file)
            else:
                abs_path = os.path.abspath(os.path.normpath(os.path.join(group_kconfigs_dir_abs, group_file)))
            # Deduplicate: only add if not seen before
            if abs_path not in seen_files:
                seen_files.add(abs_path)
                content_lines.append('source "{}"'.format(abs_path))
        
        with open(index_file, 'w', encoding='utf-8') as f:
            f.write('\n'.join(content_lines))
        
        return index_file

    def _write_specific_source_file(config_file_in, config_file_out, pattern, exclude_paths=None):
        """
        Write source file with pattern matching and optional path exclusion.
        
        Args:
            config_file_in: Input file path
            config_file_out: Output file path
            pattern: Regex pattern to match lines
            exclude_paths: Optional set of paths to exclude
        """
        print(config_file_in)
        print(config_file_out)
        try:
            with open(config_file_in, 'r', encoding='utf-8') as f:
                content_lines = f.readlines()
                # Filter by pattern
                content_lines = [content_line for content_line in content_lines 
                               if re.search(pattern, content_line)]
                # Apply exclude filter if provided
                if exclude_paths:
                    filtered_lines = []
                    for line in content_lines:
                        # Extract path from source statement: source "path"
                        match = re.search(r'source\s+"([^"]+)"', line)
                        if match:
                            path = match.group(1)
                            if path not in exclude_paths:
                                filtered_lines.append(line)
                        else:
                            filtered_lines.append(line)
                    content_lines = filtered_lines
        except Exception:
            content_lines = []
        with open(config_file_out, 'w', encoding='utf-8') as f:
            for content_line in content_lines:
                f.write(content_line)
    
    try:
        # First, write the base source files
        _write_source_file(env_dict['COMPONENT_KCONFIGS'], env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'])
        _write_source_file(env_dict['COMPONENT_KCONFIGS_PROJBUILD'], env_dict['COMPONENT_KCONFIGS_PROJBUILD_SOURCE_FILE'])
        
        # Parse component groups from KCONFIG_GROUP markers
        armino_path = env_dict.get('ARMINO_PATH', '')
        armino_subsys_dir = os.path.normpath(armino_path) if armino_path else ''
        
        # Get build directory from components_kconfigs_path
        components_kconfigs_path = env_dict.get('COMPONENTS_KCONFIGS_SOURCE_FILE', '')
        if components_kconfigs_path:
            build_dir = os.path.dirname(components_kconfigs_path)
            group_kconfigs_dir = os.path.join(build_dir, 'group_kconfigs')
        else:
            group_kconfigs_dir = os.path.join(os.getcwd(), 'group_kconfigs')
        
        # Group components by KCONFIG_GROUP marker
        component_kconfigs = env_dict.get('COMPONENT_KCONFIGS', '')
        grouped_paths = set()
        group_files = []
        special_group_outputs = {
            'Debug': env_dict.get('DEBUG_KCONFIGS_SOURCE_FILE', ''),
            'Board & SoC': env_dict.get('BOARD_SOC_KCONFIGS_SOURCE_FILE', ''),
            'Device Drivers': env_dict.get('DEVICE_DRIVERS_KCONFIGS_SOURCE_FILE', ''),
            'Wireless Connectivity': env_dict.get('WIRELESS_CONNECTIVITY_KCONFIGS_SOURCE_FILE', ''),
            'Network Services': env_dict.get('NETWORK_SERVICES_KCONFIGS_SOURCE_FILE', ''),
            'RTOS & Kernel': env_dict.get('OPERATING_SYSTEM_KCONFIGS_SOURCE_FILE', ''),
            'System Services': env_dict.get('SYSTEM_SERVICES_KCONFIGS_SOURCE_FILE', ''),
            'Demos': env_dict.get('DEMOS_KCONFIGS_SOURCE_FILE', ''),
            'Third Party': env_dict.get('THIRD_PARTY_KCONFIGS_SOURCE_FILE', ''),
        }
        special_group_outputs = dict(
            (name, path) for name, path in special_group_outputs.items() if path
        )
        
        if component_kconfigs:
            groups, ungrouped = _group_components_by_marker(component_kconfigs, list_separator)

            # Generate group Kconfig files if there are any groups
            if groups:
                group_files = _generate_group_kconfig_files(
                    groups,
                    armino_subsys_dir,
                    group_kconfigs_dir,
                    special_group_outputs
                )
                # Collect all grouped paths for exclusion
                for group_paths in groups.values():
                    grouped_paths.update(group_paths)
                    grouped_paths.update(_collect_sourced_kconfig_paths(group_paths, armino_path))
            else:
                _generate_group_kconfig_files({}, armino_subsys_dir, group_kconfigs_dir, special_group_outputs)
        else:
            _generate_group_kconfig_files({}, armino_subsys_dir, group_kconfigs_dir, special_group_outputs)
        
        # Always generate index file (even if empty)
        index_file = _generate_group_index_file(group_kconfigs_dir, group_files)
        # Store index file path in environment for later use
        env_dict['GROUP_KCONFIGS_INDEX_FILE'] = index_file
        
        # Write components_kconfigs.in with grouped components excluded
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'], 
            env_dict['COMPONENTS_KCONFIGS_SOURCE_FILE'], 
            r'.*/components/.*',
            exclude_paths=grouped_paths if grouped_paths else None
        )
        
        # Write other source files (middleware, projects, properties, extra).
        # Core SoC/arch/driver menus are placed explicitly in the AP/CP app
        # top-level Kconfig, so exclude them from that raw middleware fallback
        # to avoid duplicate symbol definitions. Properties-lib builds
        # still rely on this fallback to source their arch/driver/soc Kconfig
        # files.
        middleware_pattern = r'.*/middleware/.*'
        is_app_config = (
            'properties_libs' not in os.path.normpath(group_kconfigs_dir).split(os.sep)
        )
        if is_app_config:
            middleware_pattern = r'.*/middleware/(?!arch/|driver/|soc/|compal/).*'
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'],
            env_dict['MIDDLEWARE_KCONFIGS_SOURCE_FILE'],
            middleware_pattern,
            exclude_paths=grouped_paths if grouped_paths else None
        )
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'],
            env_dict['PROJECTS_KCONFIGS_SOURCE_FILE'],
            r'.*/projects/.*',
            exclude_paths=grouped_paths if grouped_paths else None
        )
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'],
            env_dict['PROPERTIES_KCONFIGS_SOURCE_FILE'],
            r'.*/properties/(?!modules/bk_private/).*' if is_app_config else r'.*/properties/.*',
            exclude_paths=grouped_paths if grouped_paths else None
        )
        _write_specific_source_file(
            env_dict['COMPONENT_KCONFIGS_SOURCE_FILE'],
            env_dict['EXTRA_KCONFIGS_SOURCE_FILE'],
            r'^(?!.*\/(?:components|middleware|projects|properties)\/).*',
            exclude_paths=grouped_paths if grouped_paths else None
        )
    except KeyError as e:
        print('Error:', e, 'is not defined!')
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(description='Kconfig Source File Generator')

    parser.add_argument('--env', action='append', default=[],
                        help='Environment value', metavar='NAME=VAL')

    parser.add_argument('--env-file', type=argparse.FileType('r'),
                        help='Optional file to load environment variables from. Contents '
                             'should be a JSON object where each key/value pair is a variable.')

    parser.add_argument('--list-separator', choices=['space', 'semicolon'],
                        default='space',
                        help='Separator used in environment list variables (COMPONENT_KCONFIGS, COMPONENT_KCONFIGS_PROJBUILD)')

    args = parser.parse_args()

    try:
        env = dict([(name, value) for (name, value) in (e.split('=', 1) for e in args.env)])
    except ValueError:
        print('--env arguments must each contain =.')
        sys.exit(1)

    if args.env_file is not None:
        env.update(json.load(args.env_file))

    list_separator = ';' if args.list_separator == 'semicolon' else ' '

    _prepare_source_files(env, list_separator)


if __name__ == '__main__':
    main()
