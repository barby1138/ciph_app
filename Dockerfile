FROM centos:7.4.1708

#RUN yum update 
RUN yum install -y wget
RUN yum group install -y "Development Tools"
RUN yum install -y numactl-devel expat-devel

RUN mkdir /work && chmod 777 /work
RUN cd /work && wget https://drive.google.com/file/d/1LsHbeX0M4qLBXPqp9JTF6UwmqmBizJXW/view?usp=sharing
RUN cd /work && wget https://drive.google.com/file/d/1LtqOrPmxLV1KCNG5H5pU_zzqFPlK69W3/view?usp=sharing

#creating user
ARG UNAME=parallel
#ARG UID=1000
#ARG GID=1000
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME
RUN usermod -aG root $UNAME
USER $UNAME
