# Read command line arguments and generate include files for resources.

import os
import sys
import argparse

def main():
    parser = argparse.ArgumentParser(description='Generateinclude files for resources.')
    parser.add_argument('-i', '--input', help='Input file', required=True, type=str, dest='input')
    parser.add_argument('-o', '--output', help='Output file', required=False, type=str, dest='output')
    parser.add_argument('-yes', '--yes', help='Answer yes to all questions', required=False, action='store_true', dest='yes')

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print('Input file does not exist.')
        sys.exit(1)

    # Check if output file is specified. If not, use input file name with .inl extension.
    if args.output is None:
        args.output = args.input + '.inl'
    
    # Check if output file already exists. If so, ask user if he wants to overwrite it.
    if os.path.exists(args.output) and not args.yes:
        print('Output file already exists. Do you want to overwrite it? (y/n)')
        answer = input()
        if answer != 'y':
            sys.exit(0)
    
    # Ok, let's go.
    data = open(args.input, 'rb').read()

    # Get file name without extension.
    file_name = os.path.splitext(os.path.basename(args.input))[0]

    output_prefix = '#pragma once\n\n'
    output_prefix += '#include <cstdint>\n\n'
    output_prefix += 'namespace { \n     uint8_t res_%s[] = {' % file_name

    ostream = open(args.output, 'w')

    # Write output prefix.
    ostream.write(output_prefix)

    # Write data.
    for i in range(0, len(data)):
        if i % 16 == 0:
            ostream.write('\n        ')
        ostream.write('0x%02x' % data[i])

        if i != len(data) - 1:
            ostream.write(', ')

    ostream.write('\n    };\n}')    

    ostream.close()
    


if __name__ == '__main__':
    main()
    sys.exit(0)