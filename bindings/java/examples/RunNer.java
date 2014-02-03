// This file is part of NameTag.
//
// Copyright 2013 by Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// NameTag is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// NameTag is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NameTag.  If not, see <http://www.gnu.org/licenses/>.

import cz.cuni.mff.ufal.nametag.*;
import java.util.Scanner;
import java.util.Stack;

class RunNer {
  public static void recognizeVertical(Ner ner) {
    Forms forms = new Forms();
    NamedEntities entities = new NamedEntities();
    Scanner reader = new Scanner(System.in);

    int lines = 0;
    boolean not_eof = true;
    while (not_eof) {
      String line;

      forms.clear();
      while ((not_eof = reader.hasNextLine()) && !(line = reader.nextLine()).isEmpty())
        forms.add(line);

      if (!forms.isEmpty()) {
        ner.recognize(forms, entities);

        for (int i = 0; i < entities.size(); i++) {
          NamedEntity entity = entities.get(i);

          StringBuilder entity_ids = new StringBuilder(), entity_text = new StringBuilder();
          for (int j = (int) entity.getStart(); j < entity.getStart() + entity.getLength(); j++) {
            if (j > entity.getStart()) {
              entity_ids.append(',');
              entity_ids.append(' ');
            }
            entity_ids.append(lines + j + 1);
            entity_text.append(forms.get(j));
          }
          System.out.printf("%s\t%s\t%s\n", entity_ids.toString(), entity.getType(), entity_text.toString());
        }
      }
      lines += forms.size() + 1;
    }
  }

  public static String encodeEntities(String text) {
    return text.replaceAll("&", "&amp;").replaceAll("<", "&lt;").replaceAll(">", "&gt;").replaceAll("\"", "&quot;");
  }

  public static void recognizeUntokenized(Ner ner) {
    Forms forms = new Forms();
    TokenRanges tokens = new TokenRanges();
    NamedEntities entities = new NamedEntities();
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

        for (int i = 0; i < entities.size(); i++) {
          NamedEntity entity = entities.get(i);
          int entity_start = (int) tokens.get((int) entity.getStart()).getStart();
          int entity_end = (int) (tokens.get((int) (entity.getStart() + entity.getLength() - 1)).getStart() + tokens.get((int) (entity.getStart() + entity.getLength() - 1)).getLength());

          // Close entities that end sooned than current entity
          while (!openEntities.empty() && openEntities.peek() < entity_start) {
            if (unprinted < openEntities.peek()) System.out.print(encodeEntities(text.substring(unprinted, openEntities.peek())));
            unprinted = openEntities.pop();
            System.out.print("</ne>");
          }

          // Print text just before the entity, open it and add end to the stack
          if (unprinted < entity_start) System.out.print(encodeEntities(text.substring(unprinted, entity_start)));
          unprinted = entity_start;
          System.out.printf("<ne type=\"%s\">", entity.getType());
          openEntities.push(entity_end);
        }

        // Close unclosed entities
        while (!openEntities.empty()) {
          if (unprinted < openEntities.peek()) System.out.print(encodeEntities(text.substring(unprinted, openEntities.peek())));
          unprinted = openEntities.pop();
          System.out.print("</ne>");
        }
      }
      // Write rest of the text (should be just spaces)
      if (unprinted < text.length()) System.out.print(encodeEntities(text.substring(unprinted)));
    }
  }

  public static void main(String[] args) {
    int argi = 0;
    if (argi < args.length && args[argi].equals("-v")) argi++;
    boolean use_vertical = argi > 0;

    if (!(argi < args.length)) {
      System.err.println("Usage: RunNer ner_model");
      System.exit(1);
    }

    System.err.print("Loading ner: ");
    Ner ner = Ner.load(args[argi]);
    if (ner == null) {
      System.err.println("Cannot load recognizer from file '" + args[argi] + "'");
      System.exit(1);
    }
    System.err.println("done");

    if (use_vertical) recognizeVertical(ner);
    else recognizeUntokenized(ner);
  }
}
