FROM ubuntu:16.04

RUN apt-get update && apt-get install -y wget bash git gcc g++ default-jre make automake autoconf libtool libboost-dev libboost-thread-dev libgmp-dev

ADD entrypoint.sh entrypoint.sh

ENTRYPOINT ["entrypoint.sh"]
