#! usr/bin/env python3
import sys
import os
import logging
import shutil
import struct
import re
import  argparse

GLOBAL_HDR_LEN = 32
IMG_HDR_LEN = 32
IMAGE_NUM_VAL = 2
IMAGE_VERSION = 1
MAGIC_VAL = 'QSPI_DL.'
IMAGE_OFFSET = 0x0
PARTIITON_SIZE = 0x400000

#check the input vaule(decimal or hex) and final convert to decimal.
def convert_to_decimal(input_value):
        if re.match(r"^0x[0-9a-fA-F]+$", input_value):
            return int(input_value, 16)
        elif re.match(r"^[0-9]+$", input_value):
            return int(input_value)
        else:
            raise ValueError(f'The input value is neither a valid decimal number nor a valid hex number.')

#auto convert the input value to decimal
def auto_int(input_value):
    return int(input_value, 0)

class Gen_all_final_image:
    def __init__(self,file1,file2):
        self.img1_name = file1
        self.img2_name = file2
        self.img1_len  = os.path.getsize(self.img1_name)
        self.img2_len  = os.path.getsize(self.img2_name)
        self.partition_start = IMAGE_OFFSET
        self.partition_size  = PARTIITON_SIZE
        self.partition_buf   = bytearray()
        logging.debug(f' self.img1_name {self.img1_name} self.img2_name {self.img2_name}self.img1_len {self.img1_len} self.img2_len {self.img2_len}')

    def get_input_file(self):
        return self.partition_buf

    def init_crc32_table(self):
        self.crc32_table = []

        for i in range(0,256):
            c = i
            for j in range(0,8):
                if c&1:
                    c = 0xEDB88320 ^ (c >> 1)
                else:
                    c = c >> 1
            self.crc32_table.append(c)

    def cal_crc32(self, crc, buf):
        for c in buf:
            crc = (crc>>8) ^ self.crc32_table[(crc^c)&0xff]
        return crc

    def gen_sub_hdr(self, image_name, dl_pos_addr, image_offset, img_len, version=0, type=0):
        partition_offset = struct.pack('>I', self.partition_start)
        partition_size = struct.pack('>I', self.partition_size)
        flash_start_addr = struct.pack('>I', dl_pos_addr)       #flash dl addr
        image_offset = struct.pack('>I', image_offset)          #image offset 0 #big endian
        image_len = struct.pack('>I', img_len)

        with open(image_name, 'rb') as f:
            self.partition_buf += f.read()
        #logging.debug(f'partition_offset {partition_offset}, partition_size {partition_size} , flash_start_addr {flash_start_addr},image_offset {image_offset} image_len {image_len} ')
        self.init_crc32_table()

        #logging.debug(f' self.partition_buf {self.partition_buf}')
        checksum = self.cal_crc32(0xffffffff,self.partition_buf)
        checksum = struct.pack('>I', checksum)

        version = struct.pack('>I', version)
        type = struct.pack('>H', type)
        reserved = 0
        reserved = struct.pack('>H', reserved)

        hdr = partition_offset + partition_size + flash_start_addr + image_offset + image_len  + checksum + version + type + reserved
        logging.debug(f' hdr {hdr}')
        return hdr

    def gen_global_hdr(self, img_num, img_hdr_list, version, magic_val):
        magic = magic_val.encode()
        magic = struct.pack('8s', magic)
        version = struct.pack('>I', version)
        hdr_len = struct.pack('>H', GLOBAL_HDR_LEN)
        img_num = struct.pack('>H', img_num)
        flags = struct.pack('>I', 0)
        reserved1 = struct.pack('>I', 0)
        reserved2 = struct.pack('>I', 0)
        global_crc_content = version + hdr_len + img_num + flags + reserved1 + reserved2
        for img_hdr in img_hdr_list:
            global_crc_content += img_hdr

        self.init_crc32_table()
        global_crc = self.cal_crc32(0xffffffff, global_crc_content)
        global_crc = struct.pack('>I', global_crc)
        all_app_global_hdr = magic + global_crc + version + hdr_len + img_num + flags + reserved1 + reserved2
        logging.debug(f'add download global hdr: magic={magic}, img_num={img_num}, version={version}, flags={flags}, crc={global_crc}')

        return all_app_global_hdr

#methods python3 gen_all_final_image.py all_app.bin sub.bin pos1_addr pos2_addr all_final.bin
if __name__ == '__main__':
    logging.basicConfig()
    logging.getLogger().setLevel(logging.INFO)
    logging.info(f'Gen_all_final_image start')
    file_default_len = 0
    img_hdr_list = []
    offset = (IMG_HDR_LEN * IMAGE_NUM_VAL) + GLOBAL_HDR_LEN

    #create ArgumentParser object
    parser = argparse.ArgumentParser(description =f'gen final image')
    parser.add_argument('input1_image', type=str, nargs='?',default='all_app.bin', help='the first input image(default:all_app.bin)')
    parser.add_argument('input2_image', type=str, nargs='?',default='sub.bin',help='the second input image(default:sub.bin)')   
    parser.add_argument('dl_pos1_addr', type=auto_int, nargs='?',default=0, help="the first file's doswnloading flash addr and can be in decimal or hex format.(default:0)") 
    parser.add_argument('dl_pos2_addr', type=auto_int, nargs='?',default=0, help="the second file's downloading flash addr and can be in decimal or hex format(default:0)")
    parser.add_argument('output_image', type=str, nargs='?',default='all_final.bin',help='the output image file(default:all_final.bin)')  

    #parse cmd args
    try:
        args = parser.parse_args()
    except:
        print(f'unknown command')
        sys.exit(0)

    input_file1  = args.input1_image     #sys.argv[1]
    input_file2  = args.input2_image     
    logging.debug(f'Gen_all_final_image input_file1={input_file1}, input_file2={input_file2}, global hdr: offset={offset},args.dl_pos1_addr {args.dl_pos1_addr} args.dl_pos2_addr {args.dl_pos2_addr} IMAGE_NUM_VAL={IMAGE_NUM_VAL}')
    dl_pos1_addr = args.dl_pos1_addr 
    dl_pos2_addr = args.dl_pos2_addr
    output_file  = args.output_image
    logging.debug(f'Gen_all_final_image dl_pos1={dl_pos1_addr} dl_pos2={dl_pos2_addr} output_file={output_file}')

    obj = Gen_all_final_image(input_file1, input_file2)
    sub_image1_hdr = obj.gen_sub_hdr(obj.img1_name, dl_pos1_addr, image_offset=offset, img_len= obj.img1_len, version=0, type=0)
    img_hdr_list.append(sub_image1_hdr)

    sub_image2_hdr = obj.gen_sub_hdr(obj.img2_name, dl_pos2_addr, image_offset=(offset +obj.img1_len), img_len=obj.img2_len, version=0, type=0)
    img_hdr_list.append(sub_image2_hdr)
    #raise RuntimeError(f'stop')
    global_image_hdr = obj.gen_global_hdr(img_num=IMAGE_NUM_VAL, img_hdr_list=img_hdr_list, version=IMAGE_VERSION, magic_val=MAGIC_VAL)
    all_image_hdr = global_image_hdr + sub_image1_hdr + sub_image2_hdr
    original_combine_image = obj.get_input_file()
    all_final_image = all_image_hdr + original_combine_image
    with open(output_file, 'wb+') as final_file:
        final_file.write(all_final_image)
    logging.info(f'Gen_all_final_image success')
