#!/usr/bin/env python

import sys
import os
import argparse
import subprocess
from py_lightship.TemplateProcessor import TemplateProcessor


def parse_args():

    # set up argument parser
    parser = argparse.ArgumentParser(description='Assists in cross compiling')
    parser.add_argument('--target', help='The name of the platform to compile for. This ends up being written to CMAKE_SYSTEM_NAME', type=str)
    parser.add_argument('--triplet', help='Target triplet to cross compile for, e.g. "x86_64-pc-linux-gnu"', type=str)
    parser.add_argument('--set-version', help='Sets the version string, e.g. "0.5.2-beta"', type=str)
    parser.add_argument('--c-compiler', help='Full path to the C compiler to use for the last target', type=str)
    parser.add_argument('--rc-compiler', help='Full path to the Windows resource compiler if compiling for Windows', type=str)
    parser.add_argument('--compiler-root', help='The root path of the compiler', type=str)
    parser.add_argument('--cmake', help='Additional options to pass to CMake', type=str, action='append')
    parser.add_argument('--make', help='The command to use to "make" the target (Windows will use nmake, e.g.', type=str)
    parser.add_argument('--install', help='The command to use to install the target', type=str)
    parser.add_argument('--compress', help='The command to use to compress the target', type=str)
    args = parser.parse_args()
    
    # verify targets
    if args.target is None:
        print('Please specify at least one platform')
        sys.exit(1)
    if args.triplet is None:
        print('Please specifiy a target triplet')
        sys.exit(1)
    if args.c_compiler is None:
        print('Please specify a C compiler')
        sys.exit(1)
    if args.compiler_root is None:
        print('Please specify the compiler root')
        sys.exit(1)
    if args.make is None:
        print('Please specify a command to make')
        sys.exit(1)
    if args.install is None:
        print('Please specify a command to install')
        sys.exit(1)

    # build dictionary
    return {
        'target': args.target,
        'triplet': args.triplet,
        'version': args.set_version,
        'c_compiler': args.c_compiler,
        'compiler_root': args.compiler_root,
        'rc_compiler': args.rc_compiler,
        'additional_cmake_args': args.cmake,
        'make': args.make,
        'install': args.install,
        'compress': args.compress
    }


def generate_target(target):

    t = TemplateProcessor()

    target['dest_folder_name'] = 'lightship-' + target['version'] + '-' + target['triplet']
    target['binary_path'] = os.path.abspath(os.path.join('cross-build/build', target['dest_folder_name']))
    target['install_prefix'] = os.path.abspath(os.path.join('cross-build/dist', target['dest_folder_name']))
    t.ensure_path(target['binary_path'])
    t.ensure_path(target['install_prefix'])

    # destination path is the source directory so we have access to every folder
    # we are sourcing our template files from scripts/cmake
    t.set_destination_path('.')
    t.set_source_path('scripts/cmake')

    # There are two template files to parse. The toolchains file is placed in
    # scripts/cmake/toolchains and is .gitignored
    # The install-arch script needs to land in the binary directory of the target
    target['toolchain_file'] = os.path.abspath(os.path.join('scripts/cmake/toolchains', 'toolchian-' + target['triplet'] + '.cmake'))
    t.add_template('install-arch.cmake.in', os.path.join(target['binary_path'], 'install-arch.cmake'))
    t.add_template('toolchain-arch.cmake.in', target['toolchain_file'])

    t.add_substitution('ARCH_NAME', target['target'])
    t.add_substitution('BUILD_DIR', target['binary_path'])
    t.add_substitution('C_COMPILER', target['c_compiler'])
    t.add_substitution('RC_COMPILER', target['rc_compiler'])
    t.add_substitution('ROOT_PATH', target['compiler_root'])
    t.add_substitution(target['triplet'] + '_INSTALL_PREFIX', target['install_prefix'])

    t.process_all()


def configure(target):
    cmake_call = list()
    cmake_call.append('cmake')
    cmake_call.append('-DCMAKE_TOOLCHAIN_FILE=' + target['toolchain_file'])
    cmake_call.append('-DCMAKE_BUILD_TYPE=Release')
    cmake_call.append('-DCMAKE_INSTALL_PREFIX=' + target['install_prefix'])
    if not target['additional_cmake_args'] is None:
        cmake_call.append(target['additional_cmake_args'].split(' '))

    # source CMakeLists.txt from project's source directory
    cmake_call.append(os.path.abspath('.'))

    # working directory in build dir
    process = subprocess.Popen(cmake_call, cwd=target['binary_path'])
    process.wait()
    if not process.returncode == 0:
        raise Exception('Configure phase failed')


def make(target):
    process = subprocess.Popen(target['make'].split(' '), cwd=target['binary_path'])
    process.wait()
    if not process.returncode == 0:
        raise Exception('Compile phase failed')


def install(target):
    process = subprocess.Popen(target['install'].split(' '), cwd=target['binary_path'])
    process.wait()
    if not process.returncode == 0:
        raise Exception('Install failed')


def compress(target):
    call = list()
    call.append('tar')
    call.append('--xz')
    call.append('-cf')
    call.append(target['dest_folder_name'] + '.tar.xz')
    call.append(target['dest_folder_name'])
    print(call)
    print(os.path.abspath(os.path.join(target['install_prefix'], '..')))
    process = subprocess.Popen(call, cwd=os.path.abspath(os.path.join(target['install_prefix'], '..')))
    process.wait()
    if not process.returncode == 0:
        raise Exception('Compress phase failed')

if __name__ == '__main__':

    if not os.path.isdir('scripts/cmake'):
        print('Please execute this script from the source directory')
        sys.exit(1)

    target = parse_args()
    generate_target(target)
    configure(target)
    make(target)
    install(target)
    compress(target)
    sys.exit(0)
