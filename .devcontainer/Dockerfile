FROM ubuntu:16.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y wget bash git gcc g++ default-jre make automake autoconf libtool libboost-dev libboost-thread-dev libgmp-dev

COPY docker-entrypoint.sh /
RUN chmod +x /docker-entrypoint.sh

ENV DEBIAN_FRONTEND=

# ENTRYPOINT [ "/docker-entrypoint.sh" ]
