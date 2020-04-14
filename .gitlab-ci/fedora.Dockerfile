FROM fedora:31

RUN dnf -y install \
    abattis-cantarell-fonts \
    cairo-devel \
    cairo-gobject-devel \
    ccache \
    clang \
    clang-analyzer \
    desktop-file-utils \
    diffutils \
    fribidi-devel \
    gcc \
    gcc-c++ \
    gettext \
    git \
    glib2-devel \
    glib2-static \
    glibc-devel \
    glibc-headers \
    glibc-langpack-en \
    gobject-introspection-devel \
    google-droid-sans-fonts \
    gtk-doc \
    harfbuzz-devel \
    hicolor-icon-theme \
    itstool \
    lcov \
    libthai-devel \
    ninja-build \
    python3 \
    python3-jinja2 \
    python3-pip \
    python3-pygments \
    python3-wheel \
    redhat-rpm-config \
    thai-scalable-waree-fonts \
 && dnf clean all

RUN pip3 install meson==0.53.1

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8
