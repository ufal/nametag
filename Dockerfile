FROM tensorflow/tensorflow:1.15.5-py3
WORKDIR /srv/nametag
COPY requirements.txt requirements.txt
RUN pip install -r requirements.txt
RUN apt-get install -y wget unzip
RUN wget https://lindat.mff.cuni.cz/repository/xmlui/bitstream/handle/11234/1-3773/nametag2-models-210916.zip
RUN unzip nametag2-models-210916.zip
RUN rm nametag2-models-210916.zip
COPY . .
ENTRYPOINT ["/usr/bin/python3", "nametag2_server.py"]

EXPOSE 8001
CMD ["8001", "wembedding_server_addr:port", "czech", \
  "czech-cnec2.0-200831:cs:ces:cze", "nametag2-models-210916/czech-cnec2.0-200831", \
    "http://ufal.mff.cuni.cz/nametag/2/models#czech-cnec2_acknowledgements", \
  "dutch-conll-200831:nl:nld:dut", "nametag2-models-210916/dutch-conll-200831", \
    "http://ufal.mff.cuni.cz/nametag/2/models#dutch-conll_acknowledgements", \
  "english-conll-200831:en:eng", "nametag2-models-210916/english-conll-200831", \
    "http://ufal.mff.cuni.cz/nametag/2/models#english-conll_acknowledgements", \
  "german-conll-200831:de:deu:ger", "nametag2-models-210916/german-conll-200831", \
    "http://ufal.mff.cuni.cz/nametag/2/models#german-conll_acknowledgements", \
  "german-germeval-210916", "nametag2-models-210916/german-germeval-210916", \
    "http://ufal.mff.cuni.cz/nametag/2/models#german-germeval_acknowledgements", \
  "spanish-conll-200831:es:spa", "nametag2-models-210916/spanish-conll-200831", \
    "http://ufal.mff.cuni.cz/nametag/2/models#spanish-conll_acknowledgements" ]
