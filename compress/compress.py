import os, bz2


# Get absolute path from relative path to ensure running in batch
def open_file(filename, mode='r'):
    path, fl = os.path.split(os.path.realpath(__file__))
    full_path = os.path.join(path, filename)
    return open(full_path, mode)


binary = open_file("../build/TPT-cn.exe", 'rb').read()
binlen = binary.__len__()
len1 = binlen & 255
len2 = binlen >> 8 & 255
len3 = binlen >> 16 & 255
len4 = binlen >> 24 & 255
compressed = bz2.compress(binary)
compressed = "BuTT" + chr(len1) + chr(len2) + chr(len3) + chr(len4) + compressed
open_file("../build/TPT-cn.ptu", 'wb').write(compressed)
