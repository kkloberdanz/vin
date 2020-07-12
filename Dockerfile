FROM centos:7

RUN yum install -y centos-release-scl

RUN yum install -y make git zip unzip ncurses-devel devtoolset-8-gcc

WORKDIR /work

RUN mkdir /dist

COPY . /work
