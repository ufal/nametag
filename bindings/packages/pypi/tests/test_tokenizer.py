#!/usr/bin/python
# vim:fileencoding=utf8
from __future__ import unicode_literals

import unittest

class TestTokenizer(unittest.TestCase):
    def test_tokenizer(self):
        import ufal.nametag

        tokenizer = ufal.nametag.Tokenizer.newVerticalTokenizer()
        forms = ufal.nametag.Forms()

        sentences = [
                "Prezidentem Československa v letech 1918 - 1935 byl prof . T . G . Masaryk .".split(),
                "Zemřel 14 . září 1937 ve věku 87 let .".split()
        ]

        tokenizer.setText("\n\n".join("\n".join(sentence) for sentence in sentences))
        for sentence in sentences:
            self.assertTrue(tokenizer.nextSentence(forms, None))
            self.assertEqual(list(forms), sentence)
        self.assertFalse(tokenizer.nextSentence(forms, None))

if __name__ == '__main__':
    unittest.main()
