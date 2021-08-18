FROM pwartifactory.parallelwireless.net/pw-docker-images/ciph-app/imgc/ciph_app_imgc:v88

#creating user
ARG UNAME=parallel
ARG UID=1000
ARG GID=1000
ARG DOCKER_GID=994

ARG BUILD_NUMBER=0
ENV BUILD_NUMBER=$BUILD_NUMBER
ARG BRANCH=none
ENV BRANCH=$BRANCH

RUN echo $UNAME $UID $GID $DOCKER_GID

RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME

# map id of docker group on build servers
RUN groupmod -g $DOCKER_GID docker

RUN usermod -aG docker $UNAME
RUN usermod -aG root $UNAME

USER $UNAME
