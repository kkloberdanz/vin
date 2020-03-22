FROM centos:7

RUN yum install -y make gcc git zip unzip ncurses-devel

WORKDIR /work

RUN mkdir /dist

COPY . /work
