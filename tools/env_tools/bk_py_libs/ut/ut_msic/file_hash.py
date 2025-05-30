import hashlib

def get_file_md5sum(file:str):
    md5_hash = hashlib.md5()
    with open(file, 'rb') as f:
        bin_content = f.read()
    
    md5_hash.update(bin_content)
    return md5_hash.hexdigest()