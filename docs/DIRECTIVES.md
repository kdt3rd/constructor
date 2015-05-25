Built-in Functions
==================

Below is documentation for the built-in functions and classes provided
by constructor. These are provided via a c++ <-> lua bridge.

SourceDir

RelativeSourceDir

SourceFile

SubDir

SubProject

CodeFilter

CreateFile

GenerateSourceDataFile

AddTool

AddToolSet

AddToolOption

SetToolModulePath

AddToolModulePath

LoadToolModule

EnableLanguages

SetOption

Compile

Executable

Library

UseLibraries

Include

AddDefines

GlobFiles

BuildConfiguration

DefaultConfiguration

ExternalLibraryExists

ExternalLibrary

RequiredExternalLibrary


Classes
-------

Item

  Item.name
  Item.addDependency
  Item.depends
  Item.variables
  Item.clearVariable
  Item.setVariable
  Item.addToVariable
  Item.inheritVariable
  Item.forceTool
  Item.overrideToolSetting
  Item.addIncludes
  Item.addDefines
  Item.includeArtifactDir
  Item.setTopLevel
  Item.setDefaultTarget
  Item.setPseudoTarget

Utility Collections
-------------------

file

  file.basename

  file.extension

  file.replace_extension

  file.compare

  file.diff

  file.exists

  file.find

  file.find_exe

  file.set_exe_path

  path

    path.join

    path.setp


sys

  sys.is64bit

  sys.machine

  sys.release

  sys.version

  sys.system

  sys.node

