#!/bin/sh

git clone https://github.com/open-source-parsers/jsoncpp.git
#git clone https://github.com/sirikata/liboauthcpp.git
#git clone https://github.com/thom311/libnl.git

patch liboauthcpp/include/liboauthcpp/liboauthcpp.h liboauthcpp_liboauthcpp.h.patch
patch liboauthcpp/build/CMakeLists.txt liboauthcpp_CMakeLists.txt.patch
patch jsoncpp/CMakeLists.txt jsoncpp_CMakeLists.txt.patch
