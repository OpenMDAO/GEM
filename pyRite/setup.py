from distutils.core import setup, Extension
import os

# GEM_ARCH must be one of "DARWIN", "DARWIN64", LINUX64", "WIN32", or "WIN64"
gem_arch = os.environ.get('GEM_ARCH')

# GEM_TYPE can be set to "diamond" (the default) or "quartz"
gem_type = os.environ.get('GEM_TYPE')

# These environment variables are usually set for GEM builds:
if gem_arch == 'WIN32':
    gemlib = os.environ.get('GEM_BLOC') + '\lib'
elif gem_arch == 'WIN64':
    gemlib = os.environ.get('GEM_BLOC') + '\lib'
else:
    gemlib = os.environ.get('GEM_BLOC') + '/lib'

egadsinc = os.environ.get('EGADSINC')
egadslib = os.environ.get('EGADSLIB')
caprilib = os.environ.get('CAPRILIB')

if gem_arch == 'DARWIN':
    os.environ['ARCHFLAGS'] = '-arch i386'
    if gem_type == 'quartz':
        print '\nMaking "gem.so" for "quartz" (on DARWIN)\n'

        gem_include_dirs       = ['../include',
                                  '/System/Library/Frameworks/Python.framework/Versions/Current/Extras/lib/python/numpy/core/include']
        gem_extra_compile_args = []
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
        print '\nMaking "gem.so" for "diamond" (on DARWIN)\n'

        gem_include_dirs       = ['../include',
                                   '/System/Library/Frameworks/Python.framework/Versions/Current/Extras/lib/python/numpy/core/include']
        gem_extra_compile_args = []
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
elif gem_arch == 'DARWIN64':
    os.environ['ARCHFLAGS'] = '-arch x86_64'
    if gem_type == 'quartz':
        print '\nMaking "gem.so" for "quartz" (on DARWIN64)\n'

        gem_include_dirs       = ['../include',
                                  '/System/Library/Frameworks/Python.framework/Versions/Current/Extras/lib/python/numpy/core/include']
        gem_extra_compile_args = []
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
        print '\nMaking "gem.so" for "diamond" (on DARWIN64)\n'

        gem_include_dirs       = ['../include',
                                  '/System/Library/Frameworks/Python.framework/Versions/Current/Extras/lib/python/numpy/core/include']
        gem_extra_compile_args = []
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
    if gem_type == 'quartz':
        print '\nMaking "gem.so" for "quartz" (on LINUX64)\n'

        gem_include_dirs       = ['../include']
        gem_extra_compile_args = []
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
        print '\nMaking "gem.so" for "diamond" (on LINUX64)\n'

        gem_include_dirs       = ['../include']
        gem_extra_compile_args = []
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
    if gem_type == 'quartz':
        print '\nMaking "gem.so" for "quartz" (on WIN32)\n'

        gem_include_dirs       = ['..\include',
				  'D:\Python27\Lib\site-packages\\numpy\core\include']
        gem_extra_compile_args = []
        gem_library_dirs       = [gemlib,
                                  caprilib]
        gem_libraries          = ['gem',
                                  'quartz',
                                  'capriDyn',
                                  'dcapri']
        gem_extra_link_args    = []
    else:
        print '\nMaking "gem.so" for "diamond" (on WIN32)\n'

        gem_include_dirs       = ['..\include',
				  'D:\Python27\Lib\site-packages\\numpy\core\include']
        gem_extra_compile_args = []
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
        if gem_type == 'quartz':
           gem_libraries.append('gvIMD')
        else:
           gem_libraries.append('gv')
        gem_libraries.append('GLU32')
        gem_libraries.append('OpenGL32')
        gem_libraries.append('User32')
        gem_libraries.append('GDI32')
elif gem_arch == 'WIN64':
    if gem_type == 'quartz':
        print '\nMaking "gem.so" for "quartz" (on WIN64)\n'

        gem_include_dirs       = ['..\include',
				  'D:\Python27\Lib\site-packages\\numpy\core\include']
        gem_extra_compile_args = ['-DLONGLONG']
        gem_library_dirs       = [gemlib,
                                  caprilib]
        gem_libraries          = ['gem',
                                  'quartz',
                                  'capriDyn',
                                  'dcapri']
        gem_extra_link_args    = []
    else:
        print '\nMaking "gem.so" for "diamond" (on WIN64)\n'

        gem_include_dirs       = ['..\include',
				  'D:\Python27\Lib\site-packages\\numpy\core\include']
        gem_extra_compile_args = ['-DLONGLONG']
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
        if gem_type == 'quartz':
           gem_libraries.append('gvIMD')
        else:
           gem_libraries.append('gv')
        gem_libraries.append('GLU32')
        gem_libraries.append('OpenGL32')
        gem_libraries.append('User32')
        gem_libraries.append('GDI32')
else:
    assert False, 'GEM_ARCH is not (correctly) set'

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
                    sources            = ['pyRite.c'])

# Legal keyword arguments for the setup() function
#    'distclass', 'script_name', 'script_args', 'options',
#    'name', 'version', 'author', 'author_email',
#    'maintainer', 'maintainer_email', 'url', 'license',
#    'description', 'long_description', 'keywords',
#    'platforms', 'classifiers', 'download_url',
#    'requires', 'provides', 'obsoletes'

setup (name = 'Gem',
       version = '0.90',
       description = 'Python interface to GEM',
       ext_modules = [module1])
