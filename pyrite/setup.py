from setuptools import setup, Extension
import os
import sys

try:
    import numpy
    numpy_include = os.path.join(os.path.dirname(numpy.__file__), 
                                 'core', 'include')
except ImportError:
    print 'numpy was not found.  Aborting build'
    sys.exit(-1)

# GEM_ARCH must be one of "DARWIN", "DARWIN64", LINUX64", "WIN32", or "WIN64"
gem_arch = os.environ['GEM_ARCH']

# GEM_TYPE can be set to "diamond" (the default) or "quartz"
gem_type = os.environ.get('GEM_TYPE', 'diamond')

# These environment variables are usually set for GEM builds:
gemlib = os.path.join(os.environ['GEM_BLOC'], 'lib')

if gem_type == 'diamond':
    egadsinc = os.environ['EGADSINC']
    egadslib = os.environ['EGADSLIB']
elif gem_type == 'quartz':
    caprilib = os.environ['CAPRILIB']

print '\nMaking "gem.so" for "%s" (on %s)\n' % (gem_type, gem_arch)

gem_include_dirs       = ['../include', numpy_include]

if gem_arch.startswith('DARWIN'):
    lib_ext = ".dylib"
    if gem_arch == "DARWIN64":
        os.environ['ARCHFLAGS'] = '-arch x86_64'
    else:
        os.environ['ARCHFLAGS'] = '-arch i386'
    gem_extra_compile_args = []
    if gem_type == 'quartz':
        gem_library_dirs       = [gemlib,
                                  caprilib,
                                  '/usr/X11/lib']
        gem_libraries          = ['gem',
                                  'quartz',
                                  'gem',
                                  'quartz',
                                  'capriDyn',
                                   'dcapri',
                                  'X11']
        gem_extra_link_args    = ['-u _gixCADLoad -u _gibFillCoord -u _gibFillDNodes -u _gibFillQMesh -u _gibFillQuads -u _gibFillSpecial -u _gibFillTris -u _giiFillAttach -u _giuDefineApp -u _giuProgress -u _giuRegisterApp -u _giuSetEdgeTs -u _giuWriteApp -framework CoreFoundation -framework IOKit']
    else:
        gem_library_dirs       = [gemlib,
                                  egadslib,
                                  '/usr/X11/lib']
        gem_libraries          = ['gem',
                                  'diamond',
                                  'egads']
        gem_extra_link_args    = []

    if (os.environ.get('GEM_GRAPHICS') == "gv"):
        print "...gv graphics is enabled\n"

        gem_include_dirs.append(egadsinc)
        gem_extra_compile_args.append('-DGEM_GRAPHICS=gv')
        gem_libraries.append('gv')
        gem_libraries.append('GLU')
        gem_libraries.append('GL')
        gem_libraries.append('X11')
        gem_libraries.append('Xext')
        gem_libraries.append('pthread')
        gem_extra_link_args.append('-framework IOKit -framework CoreFoundation')
elif gem_arch == 'LINUX64':
    lib_ext = ".so"
    gem_extra_compile_args = []
    if gem_type == 'quartz':
        gem_library_dirs       = [gemlib,
                                  caprilib,
                                  '/usr/X11R6/lib']
        gem_libraries          = ['gem',
                                  'quartz',
                                  'gem',
                                  'quartz',
                                  'capriDyn',
                                  'dcapri',
                                  'X11']
        gem_extra_link_args    = ['-rdynamic']
    else:
        gem_library_dirs       = [gemlib,
                                  egadslib]
        gem_libraries          = ['gem',
                                  'diamond',
                                  'egads']
        gem_extra_link_args    = []

    if (os.environ.get('GEM_GRAPHICS') == "gv"):
        print "...gv graphics is enabled\n"

        gem_include_dirs.append(egadsinc)
        gem_extra_compile_args.append('-DGEM_GRAPHICS=gv')
        gem_libraries.append('gv')
        gem_libraries.append('GLU')
        gem_libraries.append('GL')
        gem_libraries.append('X11')
        gem_libraries.append('Xext')
        gem_libraries.append('pthread')
elif gem_arch == 'WIN32':
    lib_ext = ".dll"
    gem_extra_compile_args = []
    gem_extra_link_args    = []
    if gem_type == 'quartz':
        gem_library_dirs       = [gemlib,
                                  caprilib]
        gem_libraries          = ['gem',
                                  'quartz',
                                  'capriDyn',
                                  'dcapri']
    else:
        gem_library_dirs       = [gemlib,
                                  egadslib]
        gem_libraries          = ['gem',
                                  'diamond',
                                  'egads']
    if (os.environ.get('GEM_GRAPHICS') == "gv"):
        print "...gv graphics is enabled\n"

        gem_include_dirs.append(egadsinc)
        gem_extra_compile_args.append('-DGEM_GRAPHICS=gv')
        if gem_type == 'quartz':
            gem_libraries.append('gvIMD')
        else:
            gem_libraries.append('gv')
        gem_libraries.append('GLU32')
        gem_libraries.append('OpenGL32')
        gem_libraries.append('User32')
        gem_libraries.append('GDI32')
elif gem_arch == 'WIN64':
    lib_ext = ".dll"
    gem_extra_compile_args = ['-DLONGLONG']
    gem_extra_link_args    = []
    if gem_type == 'quartz':
        gem_library_dirs       = [gemlib,
                                  caprilib]
        gem_libraries          = ['gem',
                                  'quartz',
                                  'capriDyn',
                                  'dcapri']
    else:
        gem_library_dirs       = [gemlib,
                                  egadslib]
        gem_libraries          = ['gem',
                                  'diamond',
                                  'egads']
    if (os.environ.get('GEM_GRAPHICS') == "gv"):
        print "...gv graphics is enabled\n"

        gem_include_dirs.append(egadsinc)
        gem_extra_compile_args.append('-DGEM_GRAPHICS=gv')
        if gem_type == 'quartz':
            gem_libraries.append('gvIMD')
        else:
            gem_libraries.append('gv')
        gem_libraries.append('GLU32')
        gem_libraries.append('OpenGL32')
        gem_libraries.append('User32')
        gem_libraries.append('GDI32')

# Legal keyword arguments for the Extension constructor
#    'name', 'sources', 'include_dirs',
#    'define_macros', 'undef_macros',
#    'library_dirs', 'libraries', 'runtime_library_dirs',
#    'extra_objects', 'extra_compile_args', 'extra_link_args',
#    'swig_opts', 'export_symbols', 'depends', 'language'

module1 = Extension('gem',
                    include_dirs       = gem_include_dirs,
                    extra_compile_args = gem_extra_compile_args,
                    library_dirs       = gem_library_dirs,
                    libraries          = gem_libraries,
                    extra_link_args    = gem_extra_link_args,
                    language           = 'c',
                    sources            = ['pyrite/pyrite.c'])

# Legal keyword arguments for the setup() function
#    'distclass', 'script_name', 'script_args', 'options',
#    'name', 'version', 'author', 'author_email',
#    'maintainer', 'maintainer_email', 'url', 'license',
#    'description', 'long_description', 'keywords',
#    'platforms', 'classifiers', 'download_url',
#    'requires', 'provides', 'obsoletes'

setup (name = 'pyrite',
       version = '0.90',
       description = 'Python interface to GEM',
       zip_safe = False,
       ext_modules = [module1],
       packages = ['pyrite'],
       package_data = { 'pyrite': ['test/*.py', 'lib/*%s' % lib_ext]},
      ) 




