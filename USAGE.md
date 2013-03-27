Usage
=====

The usage of constructor is meant to be expressive and flexible, but
simple most of the time. It is designed to be used to create makefiles
to build files and produce an output folder of things, usually
compiled and linked, and so it has built-in support for compilation
rules. Other mechanisms can be added via extensions.

Basic Overview
--------------

The idea is that in each source folder, you define a *build.py* file. At
the top level, you can define a *config.py* if you want to set up
globals or want to make other architecture-specific decisions at the
top level that can then be used in all the included build files. It
would be in this config file that you include or define any
extensions.

It is all just python code, with some special functions. you can treat
all files as having already done an "import os" and an "import sys",
so you can just access those immediately without an
import. Additionally, there is a set of "built-in" functions added to the
namespace by constructor automatically. Any namespace global functions
or variables defined in config.py are also made available in all
build.py files included.

Built-In Command Line Options
-----------------------------

--build <name> Chooses the particular name as the build target
               (defined via BuildConfig), otherwise defaults to
               the one defined as the default

--build-dir <path> Specifies the output directory for the root of
                   build configurations (the build output location
                   specified to BuildConfig is concatenated). Defaults
                   to the current directory.

--source-dir <path> Where to find the top level of the source files,
                    defaults to the current directory

--generator <type> The generator to use when emitting build files.
                   The default is for ninja build files, and is the
                   only one currently supported, but others are
                   possible and may be added in the future.

Core Features
-------------

FindOptionalExecutable( exe )
FindExecutable( exe )

AddExternalResolver( f )
FindExternalLibrary( name, version=None )
HaveExternal( name )

Info( msg )
Warning( msg )
Error( msg )

DefineFeature( name, typename, help=None, default=None )
Feature( name )

SubDir( name )

BuildConfig( tag, outloc, default = False )
Building( b )

GetCurrentSourceDir()
GetCurrentSourceRelDir()
GetBuildRootDir()
GetCurrentBinaryDir()

C/C++ Compilation Features
--------------------------

Unit Testing Features
---------------------

