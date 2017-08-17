Java NameTag Bindings
========================

Installation
============

Binary Java bindings are available in NameTag binary packages.

To compile Java bindings manually, run make in the bindings/java directory,
optionally with the options descriged in NameTag Installation. Java 6 and newer
is supported.

The Java installation specified in the environment variable JAVA_HOME is used.
If the environment variable does not exist, the JAVA_HOME can be specified using
  make JAVA_HOME=path_to_Java_installation


Java Bindings API
=================

NameTag library bindings is available in the cz.cuni.mff.ufal.nametag package.

The bindings is a straightforward conversion of the C++ bindings API. Vectors do
not have native Java interface, see cz.cuni.mff.ufal.nametag.Forms class for
reference. Also, class members are accessible and modifiable using using
getField and setField wrappers.

The bindings require native C++ library libnametag_java (called nametag_java on
Windows). If the library is found in the current directory, it is used,
otherwise standard library search process is used. The path to the C++ library
can also be specified using static nametag_java.setLibraryPath(String path) call
(before the first call inside the C++ library, of course).


C++ Bindings API
================


Helper Structures
-----------------

  typedef vector<string> Forms;

  struct TokenRange {
    size_t start;
    size_t length;
  };
  typedef vector<TokenRange> TokenRanges;

  struct NamedEntity {
    size_t start;
    size_t length;
    string type;

    NamedEntity();
    NamedEntity(size_t start, size_t length, const string& type);
  };
  typedef vector<NamedEntity> NamedEntities;


Main Classes
------------

  class Version {
   public:
    unsigned major;
    unsigned minor;
    unsigned patch;
    string prerelease;

    static Version current();
  };

  class Tokenizer {
   public:
    virtual void setText(const char* text);
    virtual bool nextSentence(Forms* forms, TokenRanges* tokens);

    static Tokenizer* newVerticalTokenizer();
  };

  class Ner {
    static ner* load(const char* fname);

    virtual void recognize(Forms& forms, NamedEntities& entities) const;

    virtual void entityTypes(Forms& types) const;
    virtual void gazetteers(Forms& gazetteers, Ints& gazetteer_types) const;

    virtual Tokenizer* newTokenizer() const;
  };
