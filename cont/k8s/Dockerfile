#FROM centos:7.4.1708
FROM pwartifactory.parallelwireless.net/devops_dockerhub/base-docker-images/devops-docker-prod-centos-7.4.1708-base-v2:latest

RUN mkdir -p /home/ciph_app

# dpdk-crypto-app, ciph_app.xml
COPY ciph_app/bin/* /home/ciph_app/
RUN chmod 0755 /home/ciph_app/dpdk-crypto-app

COPY ciph_app/lib/* /usr/lib64/

COPY ciph_app/svc/ciph_app.service /etc/systemd/system

COPY ciph_app/svc/ciph_app.sh /home/ciph_app
RUN chmod 0755 /home/ciph_app/ciph_app.sh

# enable
RUN ln -s -r /etc/systemd/system/ciph_app.service /etc/systemd/system/multi-user.target.wants/ciph_app.service
RUN ls -lrt /etc/systemd/system/multi-user.target.wants/ciph_app.service
