#!/usr/bin/env python3
"""Compress primary_all for secureboot overwrite OTA.

Layout (must match BL2 decompress_bl2.c):
  [uint32 block_num]
  [uint16 block_list[block_num + 2]]
  [lzma blocks...]

block_list[0..block_num-1] = compressed size of each full 64KB block.
block_list[block_num]      = compressed size of the final partial block (0 if none).
block_list[block_num + 1]  = uncompressed size of the final partial block (0 if none).
"""

import logging
import os
import struct
import tempfile

from .common import *

# MUST match COMPRESS_BLOCK_SIZE in BL2 decompress_bl2.c (64 * 1024).
COMPRESS_BLOCK_SZ = 0x10000
_UINT16_MAX = 0xFFFF


def _lzma_compress_chunk(compress_tool, chunk):
    """Compress one chunk via the external lzma tool using private temp files.

    Each call uses fresh files so a short last block cannot leave stale bytes
    for the next lzma process (the ow17 flush/truncate failure mode).
    """
    fd_in, path_in = tempfile.mkstemp(prefix='bk_cmp_in_')
    fd_out, path_out = tempfile.mkstemp(prefix='bk_cmp_out_')
    os.close(fd_out)
    try:
        with os.fdopen(fd_in, 'wb') as f:
            f.write(chunk)
        run_cmd(f'{compress_tool} e {path_in} {path_out}')
        with open(path_out, 'rb') as f:
            data = f.read()
        if len(data) > _UINT16_MAX:
            raise ValueError(
                f'lzma output {len(data)} bytes exceeds uint16 max ({_UINT16_MAX}); '
                f'input chunk was {len(chunk)} bytes')
        return data
    finally:
        for path in (path_in, path_out):
            try:
                os.unlink(path)
            except OSError:
                pass


def compress_bin(infile, outfile):
    file_size = os.path.getsize(infile)
    if file_size == 0:
        raise ValueError(f'compress_bin: empty input {infile}')

    block_num = file_size // COMPRESS_BLOCK_SZ
    remainder = file_size % COMPRESS_BLOCK_SZ
    # BL2 rejects block_num==0 (see decompress_bl2.c); primary_all is always multi-MB.
    if block_num == 0:
        raise ValueError(
            f'compress_bin: input {infile} size 0x{file_size:x} < one block '
            f'(0x{COMPRESS_BLOCK_SZ:x}); BL2 cannot install this image')

    script_dir = get_script_dir()
    compress_tool = os.path.normpath(
        os.path.join(script_dir, '..', 'tools', 'packager_tools', 'lzma'))
    if not os.path.isfile(compress_tool):
        raise FileNotFoundError(f'lzma tool not found: {compress_tool}')

    logging.debug(f'compress {infile}: size=0x{file_size:x} full_blocks={block_num} rem={remainder}')

    size_entries = []   # raw bytes for each block_list entry (2 or 4 bytes)
    compressed_blobs = []

    with open(infile, 'rb') as src:
        for idx in range(block_num):
            chunk = src.read(COMPRESS_BLOCK_SZ)
            if len(chunk) != COMPRESS_BLOCK_SZ:
                raise RuntimeError(f'short read on full block {idx}: got {len(chunk)}')
            cdata = _lzma_compress_chunk(compress_tool, chunk)
            size_entries.append(struct.pack('<H', len(cdata)))
            compressed_blobs.append(cdata)
            logging.debug(f'  full block {idx}: compressed={len(cdata)}')

        if remainder:
            chunk = src.read(remainder)
            if len(chunk) != remainder:
                raise RuntimeError(f'short read on tail: got {len(chunk)}, expect {remainder}')
            cdata = _lzma_compress_chunk(compress_tool, chunk)
            # after_size (compressed), before_size (plain)
            size_entries.append(struct.pack('<HH', len(cdata), remainder))
            compressed_blobs.append(cdata)
            logging.debug(f'  tail block: compressed={len(cdata)} plain={remainder}')
        else:
            # Exact multiple of 64KB: BL2 still reads block_list[n] / [n+1].
            size_entries.append(struct.pack('<HH', 0, 0))
            logging.debug('  no tail block: write after/before = 0/0')

    header = struct.pack('<I', block_num) + b''.join(size_entries)
    expect_hdr = 4 + 2 * (block_num + 2)
    if len(header) != expect_hdr:
        raise RuntimeError(f'header length {len(header)} != expected {expect_hdr}')

    with open(outfile, 'wb') as dst:
        dst.write(header)
        for blob in compressed_blobs:
            dst.write(blob)

    logging.info(
        f'compress_bin: {infile} -> {outfile} '
        f'(blocks={block_num}, rem={remainder}, out={os.path.getsize(outfile)})')
