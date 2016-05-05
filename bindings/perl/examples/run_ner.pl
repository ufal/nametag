# This file is part of NameTag <http://github.com/ufal/nametag/>.
#
# Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

use warnings;
use strict;
use open qw(:std :utf8);

use Ufal::NameTag;

sub encode_entities($) {
  my ($text) = @_;
  $text =~ s/[&<>"]/$& eq "&" ? "&amp;" : $& eq "<" ? "&lt;" : $& eq ">" ? "&gt;" : "&quot;"/ge;
  return $text;
}

sub sort_entities($) {
  my ($entities) = @_;
  my @entities = ();
  for (my ($i, $size) = (0, $entities->size()); $i < $size; $i++) {
    push @entities, $entities->get($i);
  }
  return sort { $a->{start} <=> $b->{start} || $b->{length} <=> $a->{length} } @entities;
}

@ARGV >= 1 or die "Usage: $0 recognizer_model\n";

print STDERR "Loading ner: ";
my $ner = Ufal::NameTag::Ner::load($ARGV[0]);
$ner or die "Cannot load recognizer from file '$ARGV[0]'\n";
print STDERR "done\n";
shift @ARGV;

my $forms = Ufal::NameTag::Forms->new();
my $tokens = Ufal::NameTag::TokenRanges->new();
my $entities = Ufal::NameTag::NamedEntities->new();
my @sorted_entities;
my @open_entities;
my $tokenizer = $ner->newTokenizer();
$tokenizer or die "No tokenizer is defined for the supplied model!";

for (my $not_eof = 1; $not_eof; ) {
  my $text = '';

  # Read block
  while (1) {
    my $line = <>;
    last unless ($not_eof = defined $line);
    $text .= $line;
    chomp($line);
    last unless length $line;
  }

  # Tokenize and recognize
  $tokenizer->setText($text);
  my $t = 0;
  while ($tokenizer->nextSentence($forms, $tokens)) {
    $ner->recognize($forms, $entities);
    @sorted_entities = sort_entities($entities);

    # Write entities
    for (my ($i, $size, $e) = (0, $tokens->size(), 0); $i < $size; $i++) {
      my $token = $tokens->get($i);
      my ($token_start, $token_length) = ($token->{start}, $token->{length});

      print encode_entities(substr $text, $t, $token_start - $t);
      print '<sentence>' if $i == 0;

      # Open entities starting at current token
      for (; $e < @sorted_entities && $sorted_entities[$e]->{start} == $i; $e++) {
        printf '<ne type="%s">', encode_entities($sorted_entities[$e]->{type});
        push @open_entities, $sorted_entities[$e]->{start} + $sorted_entities[$e]->{length} - 1;
      }

      # The token itself
      printf '<token>%s</token>', encode_entities(substr $text, $token_start, $token_length);

      # Close entities ending after current token
      while (@open_entities && $open_entities[-1] == $i) {
        print '</ne>';
        pop @open_entities;
      }
      print '</sentence>' if $i + 1 == $size;
      $t = $token_start + $token_length;
    }
  }
  # Write rest of the text
  print encode_entities(substr $text, $t);
}
