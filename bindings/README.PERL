Perl NameTag Bindings
========================

Installation
============

The Perl bindings are available as Ufal-NameTag package on CPAN.

To compile Perl bindings manually, run make in the bindings/perl directory,
optionally with the options descriged in NameTag Installation. Perl 5.10 and
later is supported.

Path to the include headers of the required Perl version must be specified in
the PERL_INCLUDE variable using
  make PERL_INCLUDE=path_to_Perl_includes


Perl Bindings API
=================

NameTag library bindings is available in the Ufal::NameTag
(http://search.cpan.org/~straka/Ufal-NameTag/) package. The classes can be
imported into the current namespace using the :all export tag.

The bindings is a straightforward conversion of the C++ bindings API. Vectors do
not have native Perl interface, see Ufal::NameTag::Forms for reference. Static
methods and enumerations are available only through the module, not through
object instance.


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

    virtual Tokenizer* newTokenizer() const;
  };
