%module "Ufal::NameTag"

%runtime %{
#ifdef seed
  #undef seed
#endif
%}

%include "../common/nametag.i"
