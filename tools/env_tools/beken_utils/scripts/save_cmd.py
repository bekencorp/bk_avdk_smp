#!/usr/bin/env python3

CMD_LIST_FILE = 'pack_cmd_list.txt'


def reset_cmd_list():
    """Truncate pack_cmd_list.txt so every build starts from a clean record."""
    with open(CMD_LIST_FILE, 'w'):
        pass


def save_cmd(description, cmd):
    """Append one 'description + command' record to pack_cmd_list.txt.

    Unlike bksecure (which appends forever), the build calls reset_cmd_list()
    once at the start of pack_all() so the file describes this build only.
    """
    with open(CMD_LIST_FILE, 'a') as f:
        f.write('\r\n')
        f.write(description)
        f.write('\r\n')
        split_line = len(description) * '-'
        f.write(split_line)
        f.write('\r\n')
        f.write(cmd)
        f.write('\r\n')
