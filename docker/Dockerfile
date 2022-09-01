# Download base image ubuntu 20.04
FROM ubuntu:20.04

# LABEL about the custom image
LABEL maintainer="BTCU developers team"
LABEL version="0.1"
LABEL description="This is Docker Image for BTCU node."

# Disable Prompt During Packages Installation
ARG DEBIAN_FRONTEND=noninteractive

# Update Ubuntu Software repository
RUN apt update

# Copy install_ubuntu.sh script and define default command for the container
COPY ../install_ubuntu.sh /install_ubuntu.sh
CMD ["./install_ubuntu.sh"]

# Expose Port for the Application
EXPOSE 3666 5666
