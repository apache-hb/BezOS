#!/bin/sh

# pkgtoold requires CAP_SYS_ADMIN to create and destroy overlay filesystems
setcap CAP_SYS_ADMIN+ep ${MESON_INSTALL_PREFIX}/bin/pkgtoold

# Install and enable the systemd service
cp ${MESON_INSTALL_PREFIX}/lib/systemd/system/pkgtoold.service /etc/systemd/system/pkgtoold.service
systemctl enable pkgtoold.service
systemctl restart pkgtoold.service
