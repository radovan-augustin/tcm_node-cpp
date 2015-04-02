#
# Copyright (c) 2015 Radovan Augustin, rado_augustin@yahoo.com
# All rights reserved.
#
# This file is part of tcm_node-cpp, licensed under BSD 3-Clause License.
#

CPP=g++

SRCS_TCM=_py.cpp \
         tcm_modules.cpp \
         tcm_iblock.cpp \
         tcm_node.cpp

OBJS_TCM=$(SRCS_TCM:.cpp=.o)

LIBS=-luuid
PROGNAME_TCM=tcm_node

all: $(PROGNAME_TCM)

%.o: %.cpp
	$(CPP) -Wno-write-strings -s -fno-rtti -c $< -o $@

$(PROGNAME_TCM): $(OBJS_TCM)
	$(CPP) $(OBJS_TCM) $(LIBS) -o $@

clean:
	rm -f *.o
	rm -f $(PROGNAME_TCM)
