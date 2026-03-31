#pragma once

void avdk_hex_dump(const void *data, size_t len, uint32_t addr_offset);

bool cmd_contain(int argc, char **argv, char *string);