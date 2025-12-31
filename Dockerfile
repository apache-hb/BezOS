FROM amazonlinux:2023

RUN yum install -y git ninja-build python3-pip g++ python3-devel pkg-config && \
    pip3 install meson

RUN yum install -y libcurl-devel openssl-devel libarchive-devel libxml2-devel

COPY data/ /opt/bezos/data/
COPY packages/ /opt/bezos/packages/
COPY repo/ /opt/bezos/repo/
COPY sources/ /opt/bezos/sources/
COPY tool/ /opt/bezos/tool/

COPY workspace.xml /opt/bezos/workspace.xml

WORKDIR /opt/bezos/

RUN mkdir -p build && \
    mkdir -p install && \
    meson setup tool build/tool --prefix /opt/bezos/install/tool

RUN meson install -C build/tool
