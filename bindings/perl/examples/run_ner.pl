# This file is part of NameTag.
#
# Copyright 2013 by Institute of Formal and Applied Linguistics, Faculty of
# Mathematics and Physics, Charles University in Prague, Czech Republic.
#
# NameTag is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# NameTag is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with NameTag.  If not, see <http://www.gnu.org/licenses/>.

use warnings;
use strict;
use open qw(:std :utf8);

use Ufal::NameTag qw(:all);

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
my $ner = Ner::load($ARGV[0]);
$ner or die "Cannot load recognizer from file '$ARGV[0]'\n";
print STDERR "done\n";
shift @ARGV;

my $forms = Forms->new();
my $tokens = TokenRanges->new();
my $entities = NamedEntities->new();
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
