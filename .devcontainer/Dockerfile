FROM ubuntu:16.04 as builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y wget bash git gcc g++ default-jre make automake autoconf libtool libboost-dev libboost-thread-dev libgmp-dev

COPY . /app/antlr
RUN /app/antlr/contrib/get-antlr-3.4 && cp -r /app/antlr/antlr-3.4 /app && rm -rf /app/antlr && mkdir /app/antlr && mv /app/antlr-3.4 /app/antlr/antlr-3.4

COPY .devcontainer/docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

ENV DEBIAN_FRONTEND