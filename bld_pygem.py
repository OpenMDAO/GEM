
import os
import sys
import subprocess
from os.path import join, dirname, basename, abspath, expanduser, isdir, isfile, expandvars
import platform
import shutil
import StringIO
import fnmatch
import struct

def copy(src, dst):
    """copy symlinks if present"""
    if (not sys.platform.startswith('win')) and os.path.islink(src):
        linkto = os.readlink(src)
        os.symlink(linkto, dst)
    else:
        shutil.copy(src, dst)

def expand_path(path):
    if path is not None:
        return abspath(expandvars(expanduser(path)))

def _get_dlibpath(egads_lib, cas_lib):
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
    """Get the architecture string (DARWIN, DARWIN64, LINUX, LINUX64, WIN32,
    WIN64)
    """
    arch_dct = {
        'darwin': 'DARWIN',
        'linux2': 'LINUX',
        'win32': 'WIN',
        }
    bits = struct.calcsize("P") * 8
    if bits == 32:
        if sys.platform == 'win32':
            return 'WIN32'
        else:
            return arch_dct[sys.platform]
    else: # assume 64 bit
        return arch_dct[sys.platform]+'64'
    
    
def _get_cas_rev(cas_root):
    for path, dirlist, filelist in os.walk(cas_root):
        for name in filelist:
            if name == 'Standard_Version.hxx':
                with open(join(path, name), 'r') as f:
                    for line in f:
                        parts = line.split()
                        if len(parts)>1 and parts[0] == '#define' and parts[1] == 'OCC_VERSION':
                            return parts[2]

def _get_occ_libs(libpath):
    if sys.platform.startswith('linux'):
        libs = fnmatch.filter(os.listdir(libpath), "*.so")
        libs.extend(fnmatch.filter(os.listdir(libpath), "*.so.*"))
    elif sys.platform == 'darwin':
        libs = fnmatch.filter(os.listdir(libpath), "*.dylib")
    elif sys.platform.startswith("win"):
        libs = fnmatch.filter(os.listdir(libpath), "*.dll")
    return [join(libpath, lib) for lib in libs]

if __name__ == '__main__':
    from argparse import ArgumentParser
    parser = ArgumentParser()
    parser.add_argument("--esp", action="store", type=str,
                        dest='esp_dir', help="Engineering Sketchpad directory")
    parser.add_argument("--casroot", action="store", type=str,
                        dest='casroot', help="OpenCASCADE root directory")
    parser.add_argument("--casrev", action="store", type=str,
                        dest='casrev', help="OpenCASCADE revision number")
    parser.add_argument("-c", "--clean", action="store_true", dest="clean",
                        help="do a make clean before building")
    parser.add_argument("--bdist_egg", action="store_true", dest="bdist_egg",
                        help="build a binary egg for pygem")
    parser.add_argument("--sdist", action="store_true", dest="sdist",
                        help="build a source distribution for pygem")
    parser.add_argument("--gv", action="store_true", dest="gv",
                        help="link in the gv libraries")
    parser.add_argument("--gemtype", action="store", type=str,
                        dest='gem_type', help="GEM type (diamond or quartz)")
  
    options = parser.parse_args()
    
    cas_rev = options.casrev
    cas_root = expand_path(options.casroot)
    esp_dir = expand_path(options.esp_dir)
    
    if cas_root is None:
        print "OpenCASCADE directory must be supplied\n"
        parser.print_help()
        sys.exit(-1)
    elif not isdir(cas_root):
        print "OpenCASCADE directory %s doesn't exist\n" % cas_root
        sys.exit(-1)
    if esp_dir is None:
        print "Engineering Sketchpad directory must be supplied\n"
        parser.print_help()
        sys.exit(-1)
    elif not isdir(esp_dir):
        print "Engineering Sketchpad directory %s doesn't exist\n" % esp_dir
        sys.exit(-1)
    elif not options.gem_type:
        print 'You must specify a GEM type (diamond or quartz)'
        sys.exit(-1)
        
    cas_lib = join(cas_root, 'lib')
    egads_lib = join(esp_dir, 'lib')
    if cas_rev is None:
        cas_rev = _get_cas_rev(cas_root)
        
    if cas_rev is None:
        print "Can't determine OpenCASCADE revision\n"
        sys.exit(-1)

    lib_path_tup = _get_dlibpath(egads_lib, cas_lib)
    arch = _get_arch()
    
    env = {
        'GEM_ARCH': arch,
        'GEM_TYPE': options.gem_type,
        'GEM_BLOC': dirname(abspath(__file__)),
        'GEM_TYPE': options.gem_type,
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
        
    if options.gv:
        env['GEM_GRAPHICS'] = 'gv'
       
    ## We could generate bash and csh files here to set the env variables
    ## for building, but since we can build directly from this script, we
    ## don't really need to.  All we need to use the pygem distribution later
    ## is to make sure that LD_LIBRARY_PATH (or equivalent) is set, and that
    ## is taken care of for us if the 'plugin install' command is used.
    # shfile = open('genEnv.sh', 'w')
    # cshfile = open('genEnv.csh', 'w')
    # try:
    #     for name, val in env.items():
    #         shfile.write('export %s=%s\n' % (name, val))
    #         cshfile.write('setenv %s %s\n' % (name, val))
    # finally:
    #     shfile.close()
    #     cshfile.close()
        
    # update the current environment
    os.environ.update(env)
    
    esp_src = join(esp_dir,'src')
    srcdirs = [esp_src, 
               join(env['GEM_BLOC'], 'src'),
               join(env['GEM_BLOC'], options.gem_type)]
    gem_dirs = []
 
    if options.clean:
        for srcdir in srcdirs:
            ret = subprocess.call('make clean', shell=True, env=os.environ, 
                                  cwd=srcdir)
    for srcdir in srcdirs:
        ret = subprocess.call('make', shell=True, env=os.environ, 
                              cwd=srcdir)
   
    pkg_name = 'pygem_'+options.gem_type
    pygem_libdir = join(dirname(abspath(__file__)), pkg_name, pkg_name, 'lib')
    if isdir(pygem_libdir):
        shutil.rmtree(pygem_libdir)
    os.mkdir(pygem_libdir)
    
    esp_libs = join(esp_dir, 'lib')
    # collect egads, opencsm libs
    for name in os.listdir(esp_libs):
        copy(join(esp_libs, name), join(pygem_libdir, name))
    
    # collect OCC libs
    for libpath in _get_occ_libs(cas_lib):
        copy(libpath, join(pygem_libdir, basename(libpath)))
    
    # run setup.py
    if options.bdist_egg:
        ret = subprocess.call("python setup.py bdist_egg", 
                              shell=True, env=os.environ, 
                              cwd=dirname(dirname(pygem_libdir)))
    if options.sdist:
        ret = subprocess.call("python setup.py sdist", 
                              shell=True, env=os.environ, 
                              cwd=dirname(dirname(pygem_libdir)))

  
