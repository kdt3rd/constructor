Constructor
===========

Constructor is meant to be a "makefile" generator that allows one to
generate build scripts for [GNU
make](http://www.gnu.org/software/make/),
[ninja](http://martine.github.com/ninja/), or others.

Relation to CMake
-----------------

[CMake](http://www.cmake.org) is certainly a great choice for
many projects, and currently provides one of the better solutions for
managing different compilation platforms. However, after using cmake
for years, it has a number of weaknesses that warranted another way of
thinking about build file generation. Here are the weaknesses that
constructor attempts to address:

- *Syntax*: If all you are doing is building a simple program, with no
  configuration or dependencies beyond the c library from the
  compiler, then it's hard to improve on add_executable from
  cmake. But once you start using libraries, some of which may be
  built as part of your project, some which may be imported, and some
  may be in pkg-config, some may not have that, cmake starts to become
  cumbersome and overly verbose to find and define those libraries and
  all their include and link paths. As a matter of style, the cmake
  syntax / language is very verbose, and why implement your own file
  parser when there are other, pre-existing languages that are very
  easy to extend, and probably are more comfortable to the programmers
  who are creating the build files?
- *Toolchains*: It is possible to define different toolchains for cmake
  to support using different compilers, but that puts cmake into a
  cross-compiling mode. However, it may not be that you are actually
  cross-compiling, but rather just switching between gcc and clang. If
  you override the compiler variables in cmake, this works, but then
  you start having a sea of conditional checks to find out which
  platform you are compiling on, and managing compiler options becomes
  more and more conditionals, so the "important" part of your build
  file where you are describing how to produce your build artifacts
  and make them available to others starts being lost amongst all the
  conditionals.
- *Dependencies*: This is more relative to larger projects that want to
  build multiple libraries and executables, potentially on their own,
  or as part of a larger build, but it is sometimes confusing to
  manage dependencies in cmake, especially when there is compile-time
  generated code. Even if you get individual library dependencies
  correct, which isn't that hard, it is very easy to end up with a
  system where build artifacts clobber each other because another
  artifact that depends on the same library causes something to be
  re-compiled because it had different flags set globally.
- *Code Generation*: add_custom_command works fine in cmake, but you do
  have to manage dependencies manually. Additionally, if you plan on
  building your code in a cross-platform environment, and there are
  differences in how the command needs to be run, you end up with
  conditionals and duplicating all the dependency definition in
  addition to how to run the command. Worse, in a cross-compiling
  environment, managing this code generation is very confusing as you
  often have to have cross-build dependencies against a native build
  of the tree. Not impossible, just very difficult to get correct.
- *Package Searches*: cmake has an extensive set of "find_*" functions
  that allow one to see if your system has some external library you
  may want to use, but depending on how that find function is written,
  it is inconsistent in how the resulting output variables are named,
  and it is also difficult to target individual compile options
  (i.e. include paths), resulting in them often being set
  globally. This is often fine, but perhaps there is an easier way to
  conditionally build and set preprocessor "HAVE_FOO" flags by having
  these packages be objects in the system instead of just a bunch of
  variables.

What is Constructor
-------------------

In a successful build system there are the following concepts:

- *artifacts*: these are produced by the build and exist on some output
  filesystem. Some of these may be transient, or intermediate,
  artifacts that don't need to be kept around once another artifact
  that depends on the intermediate artifact is finished being
  produced. But the majority will exist in some build tree "cache"
  such that subsequent iterative builds are faster. An artifact may
  not always be built.
- *target*: This may be an artifact, but may also be a "pseudo" target
  in that it is a short name to make the developer's dispatch
  simpler. This could be just a short name for a chain of an actual
  artifact, or some process to apply to the build tree (i.e. make
  clean or make install).
- *default* target: when nothing else is requested of the build
  system, what will be compiled. This may not include all available
  targets, so a target
- *dependencies*: The ninja build system has a very simple, but complete
  categorization of dependencies:
   - *explicit* dependencies: files or artifacts that are needed as
     input to the build command, as in on the command line (or
     response file)
   - *implicit* dependencies: files or artifacts that a target depends
     on, but aren't additional inputs on the command line. This might
     be a script or program that is part of the rule to build (if that
     script changes, the artifact should be re-generated). The rule
     itself is an implicit dependency, in that if the command line
     changes, the artifact will be re-built. Files included by the
     code being compiled (i.e. header files) are also implicit
     dependencies, although they are dynamic in nature, so a separate
     dependency file mechanism is used (as it is in GNU make, although
     GNU make does not have a formal implicit dependency definition,
     instead rules have to be constructed carefully to include only
     relevant explicity dependencies)
   - *order-only* dependencies: These allow one to specify that a
     particular target, when out of date, will not be generated until
     another target has been generated. This is most commonly used when
     generating header files that will then be included by the code
     being compiled.
- *rules*: These are the system command lines to be executed that
  generate at least one artifact in response to a target being
  considered out of date.
- *variables*: Used by rules such that an artifact can have a custom
  set of flags to the command line, or that a group of targets can
  have a consistent set of flags applied to them.

The goal of constructor is to enable an easy mechanism for the
developer to specify the above. To accomplish this, the core of
constructor includes the following:

- *item*: every input file or target is an item. An item may be
  referred to by name before it is explicitly defined as a target,
  such that files can be parsed in any order, and as long as a
  consistent set of targets is defined, the dependencies should
  resolve. An explicit file item, referring to a file in the build
  tree should ensure the file exists to prevent typos, but should
  perform relative path (to the current directory being processed) to
  absolute path mapping such that out-of-tree builds behave as
  expected.
- *dependency*: every item can depend on other items, with the
  following types of dependency:
   - *explicit*: as discussed above
   - *implicit*: as discussed above
   - *order*: as discussed above
   - *chain*: An explicit dependency, but any use of this item should
     also include the item as an input to the rule. This is most often
     used for building static libraries, where when you define a
     library, you may specify it depends on another library. When a
     downstream user of this library is going to link in the library,
     they don't have to additionally specify any dependencies you may
     have added.
  Dependencies must be a directed acyclic graph - circular
  dependencies of any type should be an error condition. Specifying
  the same item as multiple types of dependency should promote the
  dependency to the "highest" level of dependency specified, where the
  ordering is as such: 1. chain 2. explicit 3. implicit 4. order
- *hierarchy*: basically, the directory structure representing the
  source tree. This is a first class object to enable defining rules
  or variables that apply to all objects defined in directories
  processed recursively. This allows the top level file to define
  general build rules, although a particular sub-directory may add to
  that rule or variable, for that sub-directory (and it's
  sub-directories) only, but other directories outside that wouldn't
  see those variables. Also used to know how and when to regenerate
  the build files by tracking dependencies on the parsed files.
- *grouping*: A high level concept, not directly needed for building,
  but useful for generating developer-friendly grouping of files if
  using a system like Visual Studio or Xcode.
- *module*: enables utility functions to make working with that type
  of build target easier, and mappings that define default
  rules and variables that can be customized, but are designed to be
  overlaid with toolsets to allow a generic mechanism to switch
  compilers or other build tools.
- *toolset*: used by modules to define particular commands and command
  line flags. These may have defaults, but probably have to be defined
  by the user to inform the modules how to trigger something. For
  example, for c++, you may have an "EnableCPP11" method, but
  depending on the compiler, that may be accomplished via different
  command line option, or not be supported at all and cause an
  error. Another example might be differences in how warnings are
  enabled in C/C++.
  This must include the definition of both a current, native, toolset,
  and what the target toolset is, such that generator commands can be
  created using the native toolset if necessary when cross-compiling.
- *rule*: also an item, but an abstract definition of how to match up
  variables with items and toolsets to produce a target. It is an item
  such that only rules that are actually used (depended on) can be
  emitted, making a more terse (faster) build file.
- *variable*: items can have variables attached to them that customize
  how those items or sub-items are produced and or used. They would be
  referred to by the rules using whatever build file syntax necessary.
- *external*: mechanism to find and define external dependencies in a
  cross-platform manner. Assuming this is usually a library, the item
  created would define all the variables attached to it necessary to
  directly hook into the normal chain dependency system without having
  to manage extra variables. This is a replacement for variables
  storing the output of pkg-config.
- *generator*: Once the parsing of build files is complete, and a
  graph of items is created, a generator is applied to emit a
  particular style of build file. Initially this will just be ninja,
  but could be extended to add support for GNU make or Visual Studio
  solution files.
