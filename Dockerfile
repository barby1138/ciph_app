FROM centos:7.4.1708

#RUN yum update 
RUN yum install -y wget
RUN yum install -y python3 python3-pip
RUN pip3 install gdown
RUN yum install -y rpm-build redhat-rpm-config
#RUN yum group install -y "Development Tools"
RUN yum install -y numactl-devel expat-devel
RUN yum install -y gcc gcc-c++ make

RUN mkdir /work && chmod 777 /work

RUN cd /work && gdown https://drive.google.com/uc?id=1LtqOrPmxLV1KCNG5H5pU_zzqFPlK69W3
RUN cd /work && gdown https://drive.google.com/uc?id=1LsHbeX0M4qLBXPqp9JTF6UwmqmBizJXW
RUN cd /work && gdown https://drive.google.com/uc?id=1ZtRmUM9nZg594LIMMbUL9fFBCLj4OnmL

#creating user
ARG UNAME=parallel
#ARG UID=1000
#ARG GID=1000
#RUN groupadd -g $GID -o $UNAME
#RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
#RUN usermod -aG root $UNAME
#USER $UNAME
