%module "Ufal::NameTag"

%runtime %{
#ifdef seed
  #undef seed
#endif
%}

%include "../common/nametag.i"

%perlcode %{
@EXPORT_OK = qw(*Forms:: *TokenRange:: *TokenRanges:: *NamedEntity::
                *NamedEntities:: *Version:: *Tokenizer:: *Ner::);
%EXPORT_TAGS = (all => [@EXPORT_OK]);
%}
