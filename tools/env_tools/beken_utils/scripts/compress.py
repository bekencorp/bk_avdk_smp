#!/usr/bin/env python3
import logging
import math
import struct

from .common import *

# MUST match device-side COMPRESS_BLOCK_SIZE in BL2 decompress_bl2.c (64*1024).
COMPRESS_BLOCK_SZ = 0x10000

def compress_bin(infile, outfile):
    compress_size_list = []
    compress_temp_in = 'temp_before_compress'
    compress_temp_out = 'temp_after_compress'
    file_size = os.path.getsize(infile)
    with open(infile,'rb') as src,open(outfile,'w+b') as dst:
        # Number of full 64KB blocks. BL2 reads this as a leading little-endian
        # uint32 (see decompress_bl2.c) to size block_list, instead of
        # re-deriving it from partition geometry.
        block_num = math.floor(file_size/COMPRESS_BLOCK_SZ)
        # Layout: [uint32 block_num][uint16 block_list[block_num+2]]. The last two
        # uint16 entries hold the final partial block's after/before sizes.
        offset = 4 + 2 * (block_num + 2)
        logging.debug(f'block num = {block_num}')
        dst.seek(offset)
        sum = 0
        file_in = open(compress_temp_in,"wb+")
        file_out = open(compress_temp_out,"wb+")
        src.seek(0)
        idx = 0
        while True:
            uncompress_block_size = bytes()
            chunk = bytes() # clear chunk
            chunk = src.read(COMPRESS_BLOCK_SZ)
            if(len(chunk) != COMPRESS_BLOCK_SZ):
                uncompress_block_size = struct.pack("H",len(chunk))
            if not chunk:
                break
            file_in.seek(0)
            file_in.write(chunk)
            script_dir = get_script_dir()
            compress_tool = f'{script_dir}/../tools/packager_tools/lzma'
            cmd =f'{compress_tool} e {compress_temp_in} {compress_temp_out}'
            run_cmd(cmd)
            chunk = bytes() # clear chunk
            file_out.seek(0)
            chunk = file_out.read()
            compress_chunk_size = len(chunk)
            logging.debug(f'block after size:{compress_chunk_size}')
            compress_chunk_size = struct.pack("H",compress_chunk_size)
            compress_size = compress_chunk_size + uncompress_block_size
            compress_size_list.append(compress_size)
            dst.write(chunk)
            sum += len(chunk)
        file_in.close()
        file_out.close()
        dst.seek(0)
        dst.write(struct.pack("<I", block_num))  # leading block count read by BL2
        for num in compress_size_list:
            dst.write(num)
