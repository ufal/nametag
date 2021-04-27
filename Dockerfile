from tensorflow/tensorflow:1.12.3-py3
WORKDIR /srv/nametag
COPY requirements.txt requirements.txt
RUN pip install -r requirements.txt
COPY . .
ENTRYPOINT ["python", "nametag2.py"]
