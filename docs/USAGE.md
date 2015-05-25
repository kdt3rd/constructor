Usage
=====

The usage of constructor is meant to be expressive and flexible, but
simple most of the time. It is designed to be used to create makefiles
to build files and produce an output folder of things, usually
compiled and linked, and so it has built-in support for compilation
rules. Other mechanisms can be added via extension modules.

Basic Overview
--------------

The idea is that in each source folder, you define a *construct*
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

To make a quick executable

app = Executable( "myapp", "main.cpp" );

Of course, making a Makefile for this is also pretty trivial.
