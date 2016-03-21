Usage
=====

The usage of constructor is meant to be expressive and flexible, but
simple most of the time. It is designed to be used to create makefiles
to build files and produce an output folder of things, usually
compiled and linked, and so it has built-in support for compilation
rules. Other mechanisms can be added via extension modules.

Basic Overview
--------------

The idea is that in each source folder, you define a `construct`
file. At the top level, in the first construct file, you define build
configurations.  A build configuration is associated with a name, and
a toolset to build with, and what configuration for that toolset, so
this is used to define a debug build, or a release build, or a
cross-platform compile build.

As a user, you can define new tools to be applied, and create your own
toolsets. A toolset is a collection of tools. When defining a tool,
the idea is to centralize the platform- or software- specific nature
of command line options or programs to use, and so concepts like
warning flags and such are hidden behind a name-to-list encapsulation
so you specify "optimize" in the main construct file, and that
translates to -O3 for gcc, or whatever you choose. Similarly for
warning flags and other common options.

So the entire idea of constructor is to make it so the specific
decisions about compiling for a particular configuration are contained
within that configuration definition.




For simple things, construct is pretty ... simple.

To make a quick command line executable

```
executable "myapp"
  source "main.cpp"
```

Of course, making a Makefile for this is also pretty trivial, so
perhaps this isn't very interesting.


Configurations
--------------

The basic precept of constructor is to manage build files. One of the
common things to do is to define multiple build configurations, such
as an optimized build, or a debug build. Each of these represents a
full re-compiling of the tree with different options, and there is
usually a desire to keep both an optimized and debug build around, so
you can run normally, but quickly debug if some software issue
arises. The `configuration` directive is used to declare a new
configuration.

Once the configuration is defined, any options specified, or variables
set, such as defines declared, apply only to that configuration, as if
they were in a sub-scope like another file. Another configuration
directive resets this, such that the new configuration receives a new
set of options and variables. Finally, a `default_configuration`
directive declares that the configuration definition is finished, and
any future options or variable set apply to the current scope.

For example, the following defines 2 configurations, each with their
own options:

```
configuration "release"
  optimize "opt"
  warnings "most"
  defines "NDEBUG"
  
configuration "debug"
  optimize "debug"
  warnings "most"

default_configuration "release"
```

*NB:* Do note that the indentation is not necessary, it is just convenient
for displaying the grouping.

Toolsets
--------

define grouping of tools using a toolset. Enable globally, or per
configuration.

External Libraries and Platforms
--------------------------------

The compiler brings in libc, libstdc++, libm, threads, etc. for you,
but what about all those cool libraries you want to use?

Something like `pkg-config` under linux/BSD unices is
desired. `pkg-config` is great, but is very native-system
specific. One can have multiple `pkg-config` repositories, for 32-bit
vs. 64-bit support, or for cross-compiling. But the issue still
remains of how to specify which to use. Previous tools such as cmake
require one to handle this by massive propagation of if statements all
through the project specification files. For constructor to be a
simpler, more obvious system, this mechanism is critical.

```
toolset "mingw-x86_64"
  platform "Windows"
  add_tool {}

configuration "win64-cross"
  toolset "mingw-x86_64"
  
executable "myapp"
  source "main.cpp"
  external { "Windows", "ntdll", "shlwapi" }
```

The `external` directive applies to executables as well as libraries,
and as always, if a library specifies a library, any executables that
use that library will also link against those external libraries if
the library produced is static. The first argument in the list is the
platform that the list targets. So under linux, the windows section
would only be used when compiling the "win64-cross" configuration.

For libraries that are required across all platforms, or are meant to
be conditionally enabled, another mechanism can be used:

```
foo = external_library "foo"
if foo then
   <use foo>
end

bar = external_library{ "bar", ">=1.2.0" }
if bar then
   <use bar>
else
   error "Library 'bar' minimum version 1.2.0 not found"
end
```


