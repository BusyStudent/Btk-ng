import sys
import os

sdl_key = open('/usr/include/SDL2/SDL_keycode.h', 'r').read()

lines = sdl_key.split('\n')

for line in lines:
    l = line.strip()
    if l.startswith('SDLK_'):
        ret = l.split('=')
        name = ret[0].strip()
        name = name[5:].title()

        if name.isnumeric():
            name = '_' + name

        print('    {},'.format(name))

print('\n')

print('    switch (c) {')
for line in lines:
    l = line.strip()
    if l.startswith('SDLK_'):
        ret = l.split('=')
        sname = ret[0].strip()
        name = sname[5:].title()

        if name.isnumeric():
            name = '_' + name

        print('        case {} : return Keycode::{};'.format(sname, name))
print('        default : return Keycode::Unknown;')
print('    }')