#FROM centos:7.4.1708
FROM pwartifactory.parallelwireless.net/devops_dockerhub/base-docker-images/devops-docker-prod-centos-7.4.1708-base-v2:latest

#RUN yum update
RUN yum install -y wget
RUN yum install -y git
RUN yum install -y python3 python3-pip
RUN pip3 install gdown
RUN yum install -y rpm-build redhat-rpm-config
#RUN yum group install -y "Development Tools"
RUN yum install -y numactl-devel expat-devel
RUN yum install -y gcc gcc-c++ make
RUN yum install -y docker-ce docker-ce-cli

RUN mkdir -p /opt/swtools/bin/depot_tools/jfrog_CLI/ && \
    wget https://pwartifactory.parallelwireless.net:443/artifactory/cws-toolschains-remote-cache/jfrog -P /opt/swtools/bin/depot_tools/jfrog_CLI/

RUN cd /tmp && gdown https://drive.google.com/uc?id=1LtqOrPmxLV1KCNG5H5pU_zzqFPlK69W3
RUN cd /tmp && gdown https://drive.google.com/uc?id=1LsHbeX0M4qLBXPqp9JTF6UwmqmBizJXW
RUN cd /tmp && gdown https://drive.google.com/uc?id=1ZtRmUM9nZg594LIMMbUL9fFBCLj4OnmL

RUN cd /tmp && \
    tar zxf libIPSec_MB.0.54.0.tar.gz && \
    ls && \
    cp -f libIPSec_MB* /usr/lib

#creating user
ARG UNAME=parallel
ARG UID=1000
ARG GID=1000

RUN echo $UNAME $UID $GID

RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME

RUN getent group
RUN groupmod -g 995 input
RUN groupmod -g 999 docker

RUN usermod -aG docker $UNAME
RUN usermod -aG root $UNAME

USER $UNAME
