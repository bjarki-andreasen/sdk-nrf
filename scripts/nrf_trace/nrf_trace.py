# Usage python3 <path to elf> <path to text file with dumped addresses>
#
# Notes
#
# - if functions appear in strange files, its liley because of static inline.
#   the compiler can choose to inline, or it can create it "somewhere".

import sys
import os.path

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

if len(sys.argv) == 3:
    elf_path = sys.argv[1]
    elf_path = sys.argv[2]
else:
    elf_path = './build/hello_world/zephyr/zephyr.elf'
    trace_path = './trace.txt'

assert os.path.exists(elf_path)

elf = ELFFile(open(elf_path, "rb"))

sym_lut = []
for sect in elf.iter_sections():
    if not isinstance(sect, SymbolTableSection):
        continue

    if sect.name == '':
        continue

    for sym in sect.iter_symbols():
        if 'type' not in sym.entry.st_info:
            continue

        if sym.entry.st_info['type'] != 'STT_FUNC':
            continue

        if sym.name == '':
            continue

        sym_addr = sym.entry.st_value

        sym_lut.append({
            'name': sym.name,
            'addr': sym_addr,
        })

assert elf.has_dwarf_info()

dwarf_info = elf.get_dwarf_info()

dies = []
for cu in dwarf_info.iter_CUs():
    for die in cu.iter_DIEs():
        dies.append(die)

subprogram_dies = [die for die in dies if die.tag == 'DW_TAG_subprogram' or die.tag == 'DW_TAG_inlined_subroutine']

die_lut = []
for die in subprogram_dies:
    info = {}

    top_die = die.cu.get_top_DIE()
    if top_die is None:
        continue

    if 'DW_AT_name' not in top_die.attributes:
        continue

    info['file'] = top_die.get_full_path()

    if 'DW_AT_name' not in die.attributes:
        continue

    info['name'] = die.attributes['DW_AT_name'].value.decode('utf8')

    if 'DW_AT_decl_line' not in die.attributes:
        continue

    info['line'] = die.attributes['DW_AT_decl_line'].value

    if 'DW_AT_inline' not in die.attributes:
        if 'DW_AT_low_pc' not in die.attributes:
            continue

        info['addr'] = die.attributes['DW_AT_low_pc'].value

    die_lut.append(info)

with open(trace_path, 'r') as fp:
    addresses = [int(l.split(': ')[1][:-1], 16) for l in fp.readlines()]

used = []

for addr in addresses:
    matching_syms = [sym for sym in sym_lut if addr == sym['addr']]
    assert len(matching_syms) == 1
    sym = matching_syms[0]
    matching_dies = [die for die in die_lut if die['name'] == sym['name']]
    matching_dies = [die for die in matching_dies if 'addr' in die]
    if len(matching_dies) < 1:
        # inline function
        continue
    matching_dies = [die for die in matching_dies if die['addr'] & ~1 == sym['addr'] & ~1]
    assert len(matching_dies) == 1
    die = matching_dies[0]

    used.append({
        'name': sym['name'],
        'file': die['file'],
        'line': die['line']
    })

print('used:')
for line in used:
    print('')
    print('    ' + line['name'])
    print('    ' + line['file'])
    print('    ' + str(line['line']))
