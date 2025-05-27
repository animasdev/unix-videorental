FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    sqlite3 \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY ./server ./server
COPY ./common ./common

RUN gcc -Wno-format-truncation ./server/server.c ./server/db/*.c common/*.c -o rental-server -lsqlite3 -I./server/db

CMD ["bash"]
