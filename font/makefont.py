#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, os, re
from PIL import Image

reload(sys)
sys.setdefaultencoding('UTF-8')


# Get absolute path from relative path to ensure running in batch
def open_file(filename, mode='r'):
    path, fl = os.path.split(os.path.realpath(__file__))
    full_path = os.path.join(path, filename)
    return open(full_path, mode)


def bit_reduce(x):
    if x == 255: return 0
    if x == 170: return 1
    if x == 85: return 2
    if x == 0: return 3


# Remain only Chinese characters with regex
lang_str = open_file('../data/Lang.h', 'r').read().decode("UTF-8")
lang_str = re.sub(ur"[^\u4e00-\u9fa5]", "", lang_str)

# Deduplication
lang_str = "".join(set(lang_str))

# Sorting
char_list = list(lang_str)
char_list.sort()

# Exporting character figure with imagemagick
charindex = 0
ptrcount = 256
ptrindex = 4615
lastchar = 255
ptrline = ""
fontfile = open_file('fontsample.h', 'r')
contents = fontfile.readlines()
fontfile.close()

for char in char_list:
    charindex += 1

    # Original converting method, no anti-aliasing
    '''
    cmd = "convert -size 10x10 xc:none +antialias -gravity center -pointsize 10 "
    cmd += "-font \"C:\\\\Windows\\\\Fonts\\\\ZpixEX2_EX.ttf\" label:\""
    cmd += char
    cmd += "\" result.xbm"
    print cmd
    if os.system(cmd.decode('UTF-8').encode('cp936')):
        print('IM process failed!')
    else:
        print('IM process success.')

    # xbm file to TPT font encoding
    charmap = open('result-1.xbm', 'r').read()
    bit = []
    i = 0
    for hexstr in re.finditer('0x', charmap):
        i += 1
        byte = int(charmap[hexstr.start():hexstr.start() + 4], 16)
        if i % 2:
            rowcount = 8
        else:
            rowcount = 2
        for k in range(rowcount):
            bit.append(byte >> k & 1)
    '''

    # New converting method with anti-aliasing
    cmd = "convert -size 10x10 -gravity center -pointsize 10 -depth 2 "
    cmd += "-font \"C:\\\\Windows\\\\Fonts\\\\ZpixEX2_EX.ttf\" label:\""
    cmd += char
    cmd += "\" result.png"
    print cmd
    if os.system(cmd.decode('UTF-8').encode('cp936')):
        print('IM process failed!')
    else:
        print('IM process success.')

    # png file to TPT font encoding
    charimg = Image.open('result.png')
    bit = list(charimg.getdata())
    bit = map(bit_reduce, bit)
    print bit

    # Insert font data into Font.h
    i = 0
    line = "    0x0A, "
    while i < 100:
        bitslice = bit[i:i + 4]
        byte = 0
        for k in range(3, -1, -1):
            byte = byte << 2 | bitslice[k]
        line += "0x" + format(byte, '02X') + ", "
        i += 4
    contents.insert(charindex + 260, line + "\n")  # Insert font pointer data into Font.h, 0x0000 for unused characters
    currchar = int(repr(char)[4:8], 16)
    for i in range(lastchar + 1, currchar):
        if not ptrcount % 8:
            ptrline = "    "
        ptrline += "0x0000, "
        if ptrcount % 8 == 7:
            contents.insert(charindex + 262 + ((ptrcount + 1) / 8), ptrline + "\n")
        ptrcount += 1
    lastchar = currchar

    ptrindex += 26
    if not ptrcount % 8:
        ptrline = "    "
    ptrline += "0x" + format(ptrindex, '0002X') + ", "
    if ptrcount % 8 == 7:
        contents.insert(charindex + 262 + ((ptrcount + 1) / 8), ptrline + "\n")
    ptrcount += 1

fontfile = open_file('../data/font.h', 'w')
fontfile.writelines(contents)
fontfile.close()

# Delete useless files
'''
os.remove('result-0.xbm')
os.remove('result-1.xbm')
'''
os.remove('result.png')
