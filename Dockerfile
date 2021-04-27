from tensorflow/tensorflow:1.12.3-py3
WORKDIR /srv/nametag
COPY requirements.txt requirements.txt
RUN pip install -r requirements.txt
RUN curl --remote-name-all https://lindat.mff.cuni.cz/repository/xmlui/bitstream/handle/11234/1-3443/nametag2-models-200831.zip
RUN unzip nametag2-models-200831.zip
RUN rm nametag2-models-200831.zip
COPY . .
ENTRYPOINT ["python", "nametag2.py"]
