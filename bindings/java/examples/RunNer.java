// This file is part of NameTag <http://github.com/ufal/nametag/>.
//
// Copyright 2016 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

import cz.cuni.mff.ufal.nametag.*;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Scanner;
import java.util.Stack;

class RunNer {
  public static String encodeEntities(String text) {
    return text.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll("\"", "&quot;");
  }

  public static void sortEntities(NamedEntities entities, ArrayList<NamedEntity> sortedEntities) {
    class NamedEntitiesComparator implements Comparator<NamedEntity> {
      public int compare(NamedEntity a, NamedEntity b) {
        if (a.getStart() < b.getStart()) return -1;
        if (a.getStart() > b.getStart()) return 1;
        if (a.getLength() > b.getLength()) return -1;
        if (a.getLength() < b.getLength()) return 1;
        return 0;
      }
    }
    NamedEntitiesComparator comparator = new NamedEntitiesComparator();

    sortedEntities.clear();
    for (int i = 0; i < entities.size(); i++)
      sortedEntities.add(entities.get(i));
    Collections.sort(sortedEntities, comparator);
  }

  public static void main(String[] args) {
    if (args.length == 0) {
      System.err.println("Usage: RunNer recognizer_model");
      System.exit(1);
    }

    System.err.print("Loading ner: ");
    Ner ner = Ner.load(args[0]);
    if (ner == null) {
      System.err.println("Cannot load recognizer from file '" + args[0] + "'");
      System.exit(1);
    }
    System.err.println("done");

    Forms forms = new Forms();
    TokenRanges tokens = new TokenRanges();
    NamedEntities entities = new NamedEntities();
    ArrayList<NamedEntity> sortedEntities = new ArrayList<NamedEntity>();
    Scanner reader = new Scanner(System.in);
    Stack<Integer> openEntities = new Stack<Integer>();
    Tokenizer tokenizer = ner.newTokenizer();
    if (tokenizer == null) {
      System.err.println("No tokenizer is defined for the supplied model!");
      System.exit(1);
    }

    boolean not_eof = true;
    while (not_eof) {
      StringBuilder textBuilder = new StringBuilder();
      String line;

      // Read block
      while ((not_eof = reader.hasNextLine()) && !(line = reader.nextLine()).isEmpty()) {
        textBuilder.append(line);
        textBuilder.append('\n');
      }
      if (not_eof) textBuilder.append('\n');

      // Tokenize and recognize
      String text = textBuilder.toString();
      tokenizer.setText(text);
      int unprinted = 0;
      while (tokenizer.nextSentence(forms, tokens)) {
        ner.recognize(forms, entities);
        sortEntities(entities, sortedEntities);

        for (int i = 0, e = 0; i < tokens.size(); i++) {
          TokenRange token = tokens.get(i);
          int token_start = (int)token.getStart();
          int token_end = (int)token.getStart() + (int)token.getLength();

          if (unprinted < token_start) System.out.print(encodeEntities(text.substring(unprinted, token_start)));
          if (i == 0) System.out.print("<sentence>");

          // Open entities starting at current token
          for (; e < sortedEntities.size() && sortedEntities.get(e).getStart() == i; e++) {
            System.out.printf("<ne type=\"%s\">", sortedEntities.get(e).getType());
            openEntities.push((int)sortedEntities.get(e).getStart() + (int)sortedEntities.get(e).getLength() - 1);
          }

          // The token itself
          System.out.printf("<token>%s</token>", encodeEntities(text.substring(token_start, token_end)));

          // Close entities ending after current token
          while (!openEntities.empty() && openEntities.peek() == i) {
            System.out.printf("</ne>");
            openEntities.pop();
          }
          if (i + 1 == tokens.size()) System.out.printf("</sentence>");
          unprinted = token_end;
        }
      }
      // Write rest of the text (should be just spaces)
      if (unprinted < text.length()) System.out.print(encodeEntities(text.substring(unprinted)));
    }
  }
}
