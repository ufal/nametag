#!/usr/bin/python
# vim:fileencoding=utf8
from __future__ import unicode_literals
from operator import itemgetter

import os
import unittest

class TestNer(unittest.TestCase):
    def test_ner(self):
        import ufal.nametag

        ner = ufal.nametag.Ner.load(os.path.join(os.path.dirname(__file__),  "data", "test.ner"))
        self.assertTrue(ner)

        def recognize(self, sentence, results):
            tokenizer = ner.newTokenizer()
            self.assertTrue(tokenizer)

            forms = ufal.nametag.Forms()
            entities = ufal.nametag.NamedEntities()

            tokenizer.setText(sentence);
            self.assertTrue(tokenizer.nextSentence(forms, None))

            ner.recognize(forms, entities)
            self.assertEqual(sorted([{'start':entity.start,
                                      'length':entity.length,
                                      'type':entity.type} for entity in entities],
                                    key=itemgetter('start','length')),
                             results)
            self.assertFalse(tokenizer.nextSentence(forms, None))

        recognize(self, "Vidím kočku.", [{'start':1, 'length':1, 'type':'animal'}])
        recognize(self, "Kočka vidí kočky.", [{'start':0, 'length':1, 'type':'animal'}, {'start':2, 'length':1, 'type':'animal'}])

if __name__ == '__main__':
    unittest.main()
