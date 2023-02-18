use Test::More tests => 5;
use utf8;

use Ufal::NameTag;

my $tokenizer = Ufal::NameTag::Tokenizer::newVerticalTokenizer();
my $forms = Ufal::NameTag::Forms->new();

my @sentences = (
  [qw(Prezidentem Československa v letech 1918 - 1935 byl prof . T . G . Masaryk .)],
  [qw(Zemřel 14 . září 1937 ve věku 87 let .)]
);

$tokenizer->setText(join("\n\n", map {join("\n", @{$_})} @sentences));
foreach my $sentence (@sentences) {
  ok($tokenizer->nextSentence($forms, undef));
  is_deeply([map { $forms->get($_) } (0 .. $forms->size() - 1)], $sentence);
}
ok(!$tokenizer->nextSentence($forms, undef));
