#!/bin/sh

# Prepare API documentation and examples
make -C ../../../doc nametag_bindings_api.txt >/dev/null

cat <<EOF
=encoding utf-8

=head1 NAME

Ufal::NameTag - bindings to NameTag library L<http://ufal.mff.cuni.cz/nametag>.

=head1 SYNOPSIS

  use Ufal::NameTag;

  my $ner_file = 'czech-cnec2.0-140304.ner';
  my $ner = Ufal::NameTag::Ner::load($ner_file) or die "Cannot load NER from file '$ner_file'\n";
  my $forms = Ufal::NameTag::Forms->new(); $forms->push($_) for qw(Jan Hus bydlel v Praze .);
  my $entities = Ufal::NameTag::NamedEntities->new();

  $ner->recognize($forms, $entities);

  for my $i (0 .. $entities->size()-1) {
    my $entity = $entities->get($i);
    printf "entity of type %s: '%s'\n", $entity->{type},
      join(' ', map {$forms->get($_)} ($entity->{start} .. $entity->{start}+$entity->{length}-1));
  }

=head1 REQUIREMENTS

To compile the module, C++11 compiler is needed, either C<g++> 4.7 or newer,
C<clang> 3.2 or newer or C<Visual Studio 2015>.

=head1 DESCRIPTION

C<Ufal::NameTag> is a Perl binding to NameTag library L<http://ufal.mff.cuni.cz/nametag>.

All classes can be imported into the current namespace using the C<all> export tag.

The bindings is a straightforward conversion of the C<C++> bindings API.
Vectors do not have native Perl interface, see L<Ufal::NameTag::Forms>
source for reference. Static methods and enumerations are available only
through the module, not through object instance.

=head2 Wrapped C++ API

The C++ API being wrapped follows. For a API reference of the original
C++ API, see L\<http://ufal.mff.cuni.cz/nametag/api-reference\>.

EOF
tail -n+4 ../../../doc/nametag_bindings_api.txt | sed 's/^/  /'
cat <<EOF

=head1 Example

=head2 run_ner

Simple example performing named entity recognition.

EOF
sed '1,/^$/d' ../../../bindings/perl/examples/run_ner.pl | sed 's/^/  /'
cat <<EOF

=head1 AUTHORS

Milan Straka <straka@ufal.mff.cuni.cz>

Jana Straková <strakova@ufal.mff.cuni.cz>

=head1 COPYRIGHT AND LICENCE

Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
Mathematics and Physics, Charles University in Prague, Czech Republic.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

=cut
EOF
