#!/bin/sh

#
# airhockey - SFML Key script
# Written 2011 by David Herrmann <dh.herrmann@googlemail.com>
# Dedicated to the Public Domain
#

#
# SFML uses an enum for all keys it reports. We simply map these keys to our
# enums so we do not need to perform conversions.
# This greps the SFML header for all sfKeyXY enums and echos out the enum that
# we use in airhockey and we just need to copy it into engine3d.h.
#

grep sfKey /usr/include/SFML/Window/Keyboard.h | head -n -3 | sed -e 's/.*sfKey\([^,]*\).*/\tE3D_KEY_\U\1\E = sfKey\1,/'
