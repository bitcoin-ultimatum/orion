# Download base image ubuntu 18.04
FROM ubuntu:18.04

# LABEL about the custom image
LABEL maintainer="BTCU developers team"
LABEL version="0.1"
LABEL description="This is Docker Image for BTCU node."

# Disable Prompt During Packages Installation
ARG DEBIAN_FRONTEND=noninteractive

# Update Ubuntu Software repository
RUN apt update
RUN apt-get update && apt-get install -y wget

# Copy install_ubuntu.sh script and define default command for the container
COPY install_ubuntu_docker.sh /install_ubuntu_docker.sh
RUN bash -c "/install_ubuntu_docker.sh"

#RUN wget https://btcu.io/releases/chainstate_orion.zip
#RUN apt install unzip
#RUN mkdir /home/.btcu
#RUN unzip -d /home/.btcu chainstate_orion.zip
#ENTRYPOINT ["./orion/bin/btcud", "-daemon"]

# Expose Port for the Application
EXPOSE 3666 5666
