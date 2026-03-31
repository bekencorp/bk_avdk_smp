#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <common/bk_assert.h>
#include <avdk_check.h>

/**
 * Hex dump function - prints memory content in hexadecimal format
 * 
 * @param data Pointer to the data buffer
 * @param len  Length of data to dump
 * @param addr_offset Base address offset (usually 0, or actual address)
 */
void avdk_hex_dump(const void *data, size_t len, uint32_t addr_offset)
{
    const uint8_t *buf = (const uint8_t *)data;
    size_t i, j;
    char line_buf[128];  // Buffer for one complete line
    char hex_str[49] = {0};  // 16 bytes * 3 chars = 48, plus 1 for middle space
    char ascii[17] = {0};
    int hex_pos = 0;

    if (data == NULL || len == 0)
    {
        bk_printf("hex_dump: invalid parameters (data=%p, len=%zu)\n", data, len);
        return;
    }

    bk_printf("Hex dump (len=%zu, offset=0x%08X):\n", len, addr_offset);
    bk_printf("Address   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
    bk_printf("--------  -----------------------------------------------  ----------------\n");

    for (i = 0; i < len; i += 16)
    {
        hex_pos = 0;

        // Build hex bytes string (8 bytes, space, 8 bytes)
        for (j = 0; j < 16; j++)
        {
            if (i + j < len)
            {
                uint8_t byte = buf[i + j];
                hex_pos += os_snprintf(hex_str + hex_pos, sizeof(hex_str) - hex_pos, "%02X ", byte);

                // Build ASCII representation
                ascii[j] = (byte >= 0x20 && byte <= 0x7E) ? byte : '.';
            }
            else
            {
                hex_pos += os_snprintf(hex_str + hex_pos, sizeof(hex_str) - hex_pos, "   ");  // Padding
                ascii[j] = ' ';
            }

            // Add extra space after 8 bytes
            if (j == 7)
            {
                hex_pos += os_snprintf(hex_str + hex_pos, sizeof(hex_str) - hex_pos, " ");
            }
        }

        // Build complete line: address + hex + ascii
        ascii[16] = '\0';
        os_snprintf(line_buf, sizeof(line_buf), "%08X  %-48s  %s\n", 
                    (uint32_t)(addr_offset + i), hex_str, ascii);

        // Print the complete line in one call
        bk_printf("%s", line_buf);
    }
}

bool cmd_contain(int argc, char **argv, char *string)
{
    bool ret = false;

    for (int i = 0; i < argc; i++)
    {
        if (os_strcmp(argv[i], string) == 0)
        {
            ret = true;
        }
    }

    return ret;
}