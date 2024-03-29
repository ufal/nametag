Version 1.2.2-dev
-----------------


Version 1.2.1 [15 Feb 23]
-------------------------
- Fix warnings on Clang 15 by qualifying `std::move`.
- Update MorphoDiTa to 1.11.2.


Version 1.2.0 [18 Feb 23]
-------------------------
- Add `FormCapitalization` feature template.
- Add `GazetteersEnhanced` feature template, much improving gazetteers
  handling, including better matching, possibility of hard gazetteers
  and support for loading additional gazetteers to an existing model.
- Export recognizable named entities from the recognizer.
- Export gazetteers from the recognizer (only `GazetteersEnhanced`).
- Add several `nametag_server` options.
- Update MorphoDita to 1.11.1.
- On Windows, the file paths are now UTF-8 encoded, instead of ANSI.
  This change affects the API, binary arguments, and program outputs.
- Add ARM64 macOS build.
- The Windows binaries are now compiled with VS 2019, older systems
  than Windows 7 are no longer supported.
- The Python wheels are provided for Pythons 3.6-3.11.


Version 1.1.2 [01 Jul 17]
-------------------------
- Allow specifying custom path to C++ library in Java.
- Fix bug causing a memory leak on g++.
- Add `--log` option to the REST server.


Version 1.1.1 [05 May 16]
-------------------------
- Remove forgotten `dllimport` attribute (fixes compilation on Windows).


Version 1.1.0 [05 May 16]
-------------------------
- Change license from LGPL to MPL 2.0.
- Fix problem in external tagger, which was unusable.
- Add support for OS X 10.7 and later.
- Support compilation on Visual Studio 2015.
- Add C# bindings.
- Add REST server using MicroRestD http://github.com/ufal/microrestd.
- Start using Semantic Versioning http://semver.org/.
- Remove support for shared library build.
- Use C++ Builtem http://github.com/ufal/cpp_builtem as build system.
- Use embedded version of MorphoDiTa instead of using GIT submodule.
- Use upstream version of C++ Utils http://github.com/ufal/cpp_utils.
- Use upstream version of Unilib http://github.com/ufal/unilib.
- Use PyTypeObject in Python bindings instead of proxy classes.
- Use C++ iostreams instead of C stdio.
- Use t2t_docsys http://github.com/ufal/t2t_docsys as documentation system.


Version 1.0.0 [11 Apr 14]
-------------------------
- Initial public release.
