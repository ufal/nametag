use Test::More tests => 9;
use utf8;

use Ufal::NameTag;

ok(my $ner = Ufal::NameTag::Ner::load('t/data/test.ner'));

sub recognize {
  my ($sentence, @results) = @_;

  ok(my $tokenizer = $ner->newTokenizer());

  my $forms = Ufal::NameTag::Forms->new();
  my $entities = Ufal::NameTag::NamedEntities->new();

  $tokenizer->setText($sentence);
  ok($tokenizer->nextSentence($forms, undef));

  $ner->recognize($forms, $entities);
  is_deeply([sort {$a->{start} <=> $b->{start} || $a->{length} <=> $b->{length}}
                  map { {start=>$entities->get($_)->{start},
                         length=>$entities->get($_)->{length},
                         type=>$entities->get($_)->{type}} }
                      (0 .. $entities->size()-1)],
            [@results]);

  ok(not $tokenizer->nextSentence($forms, undef));
}

recognize("Vidím kočku.", {start=>1, length=>1, type=>'animal'});
recognize("Kočka vidí kočky.", {start=>0, length=>1, type=>'animal'}, {start=>2, length=>1, type=>'animal'});
