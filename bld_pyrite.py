
import os
import sys
import subprocess
from os.path import join, dirname, abspath, expanduser
import platform

def expand_path(path):
    if path is not None:
        return abspath(expanduser(path))

def _get_dlibpath():
    _lib_path_dct = {
        'darwin': 'DYLD_LIBRARY_PATH',
        'win32': 'PATH',
    }
    pname = _lib_path_dct.get(sys.platform, 'LD_LIBRARY_PATH')
    path = os.environ.get(pname)
    if path is None:
        path = ''
    parts = path.split(os.pathsep)
    if egads_lib not in parts:
        parts = [egads_lib]+parts
    if cas_lib not in parts:
        parts = [cas_lib]+parts
    return (pname, os.pathsep.join(parts))

def _get_arch():
    """Get the architecture string (DARWIN, DARWIN64, LINUX, LINUX64, WIN32, WIN64)"""
    arch_dct = {
        'darwin': 'DARWIN',
        'linux2': 'LINUX',
        'win32': 'WIN32',
        }
    return arch_dct[sys.platform]
    
    
def _get_cas_rev(cas_root):
    for path, dirlist, filelist in os.walk(cas_root):
        for name in filelist:
            if name == 'Standard_Version.hxx':
                with open(join(path, name), 'r') as f:
                    for line in f:
                        parts = line.split()
                        if len(parts)>1 and parts[0] == '#define' and parts[1] == 'OCC_VERSION':
                            return parts[2]


if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("--esp", action="store", type=str,
                        dest='esp_dir', help="Engineering Sketchpad directory")
    parser.add_argument("--casroot", action="store", type=str,
                        dest='casroot', help="OpenCASCADE root directory")
    parser.add_argument("--casrev", action="store", type=str,
                        dest='casrev', help="OpenCASCADE revision number")
   
    options = parser.parse_args()
    
    cas_rev = options.casrev
    cas_root = expand_path(options.casroot)
    esp_dir = expand_path(options.esp_dir)
    
    if cas_root is None:
        print "OpenCASCADE directory must be supplied"
        parser.print_help()
        sys.exit(-1)
    elif not os.path.isdir(cas_root):
        print "OpenCASCADE directory doesn't exist"
        sys.exit(-1)
    if esp_dir is None:
        print "Engineering Sketchpad directory must be supplied"
        parser.print_help()
        sys.exit(-1)
    elif not os.path.isdir(esp_dir):
        print "Engineering Sketchpad directory doesn't exist"
        sys.exit(-1)
        
    cas_lib = join(cas_root, 'lib')
    egads_lib = join(esp_dir, 'lib')
    if cas_rev is None:
        cas_rev = _get_cas_rev(cas_root)
        
    if cas_rev is None:
        print "Can't determine OpenCASCADE revision"
        sys.exit(-1)

    lib_path_tup = _get_dlibpath()
    arch = _get_arch()
    
    env = {
        'GEM_ARCH': arch,
        'GEM_TYPE': 'diamond',
        'GEM_BLOC': dirname(abspath(__file__)),
        'OCSM_SRC': join(esp_dir, 'src', 'OpenCSM'),
        'EGADSINC': join(esp_dir, 'src', 'EGADS', 'include'),
        'EGADSLIB': egads_lib,
        'CASROOT': cas_root,
        'CASREV': cas_rev,
        'CASARCH': arch[0]+arch[1:].lower(),
        'GEM_ROOT': esp_dir,
        lib_path_tup[0]: lib_path_tup[1],
        }
    
    if sys.platform == 'darwin':
        env['MACOSX'] = '.'.join(platform.mac_ver()[0].split('.')[0:2])
        

    import pprint
    pprint.pprint(env)
    