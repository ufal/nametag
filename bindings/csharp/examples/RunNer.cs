// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

using Ufal.NameTag;
using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

class RunNer {
    private static int NamedEntitiesComparator(NamedEntity a, NamedEntity b) {
        if (a.start < b.start) return -1;
        if (a.start > b.start) return 1;
        if (a.length > b.length) return -1;
        if (a.length < b.length) return 1;
        return 0;
    }
    private static void SortEntities(NamedEntities entities, List<NamedEntity> sortedEntities) {
        sortedEntities.Clear();
        foreach (NamedEntity entity in entities)
            sortedEntities.Add(entity);
        sortedEntities.Sort(NamedEntitiesComparator);
    }

    public static int Main(string[] args) {
        if (args.Length < 1) {
            Console.Error.WriteLine("Usage: RunMorphoCli ner_file");
            return 1;
        }

        Console.Error.Write("Loading ner: ");
        Ner ner = Ner.load(args[0]);
        if (ner == null) {
            Console.Error.WriteLine("Cannot load ner from file '{0}'", args[0]);
            return 1;
        }
        Console.Error.WriteLine("done");

        Forms forms = new Forms();
        TokenRanges tokens = new TokenRanges();
        NamedEntities entities = new NamedEntities();
        List<NamedEntity> sortedEntities = new List<NamedEntity>();
        Stack<int> openEntities = new Stack<int>();
        Tokenizer tokenizer = ner.newTokenizer();
        if (tokenizer == null) {
            Console.Error.WriteLine("No tokenizer is defined for the supplied model!");
            return 1;
        }

        XmlTextWriter xmlOut = new XmlTextWriter(Console.Out);
        for (bool not_eof = true; not_eof; ) {
            string line;
            StringBuilder textBuilder = new StringBuilder();

            // Read block
            while ((not_eof = (line = Console.In.ReadLine()) != null) && line.Length > 0) {
                textBuilder.Append(line).Append('\n');
            }
            if (not_eof) textBuilder.Append('\n');

            // Tokenize and tag
            string text = textBuilder.ToString();
            tokenizer.setText(text);
            int t = 0;
            while (tokenizer.nextSentence(forms, tokens)) {
                ner.recognize(forms, entities);
                SortEntities(entities, sortedEntities);

                for (int i = 0, e = 0; i < tokens.Count; i++) {
                    int token_start = (int)tokens[i].start, token_length = (int)tokens[i].length;
                    xmlOut.WriteString(text.Substring(t, token_start - t));
                    if (i == 0) xmlOut.WriteStartElement("sentence");

                    for (; e < sortedEntities.Count && sortedEntities[e].start == i; e++) {
                        xmlOut.WriteStartElement("ne");
                        xmlOut.WriteAttributeString("type", sortedEntities[e].type);
                        openEntities.Push((int)sortedEntities[e].start + (int)sortedEntities[e].length - 1);
                    }

                    xmlOut.WriteStartElement("token");
                    xmlOut.WriteString(text.Substring(token_start, token_length));
                    xmlOut.WriteEndElement();

                    for (; openEntities.Count > 0 && openEntities.Peek() == i; openEntities.Pop())
                        xmlOut.WriteEndElement();
                    if (i + 1 == tokens.Count) xmlOut.WriteEndElement();
                    t = token_start + token_length;
                }
            }
            xmlOut.WriteString(text.Substring(t));
        }
        return 0;
    }
}
