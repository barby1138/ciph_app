FROM centos:7.4.1708

#RUN yum update 
#RUN yum install -y wget
RUN yum group install -y "Development Tools"
RUN yum install -y numactl-devel expat-devel

#RUN mkdir /project && chmod 777 /project
RUN mkdir /work && chmod 777 /work
COPY 3rdparty_artifactory/cmake-3.6.0-Linux-x86_64.tar.gz /work
RUN cd /work && tar zxf cmake-3.6.0-Linux-x86_64.tar.gz && cd cmake-3.6.0-Linux-x86_64
RUN cd /work/cmake-3.6.0-Linux-x86_64 && cp -r bin /usr && cp -r share /usr

#creating user
ARG UNAME=parallel
#ARG UID=1000
#ARG GID=1000
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
RUN usermod -aG root $UNAME
USER $UNAME


