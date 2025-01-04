# Copyright (C) 2022 Davidson Francis <davidsondfgl@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

CC      ?= g++
WSDIR    = $(CURDIR)/../../
INCLUDE  = -I $(WSDIR)/include
CFLAGS   =  -Wall -Wextra -O2
CFLAGS  +=  $(INCLUDE) -std=c99 -pthread -pedantic
LDLIBS  +=  $(WSDIR)/libws.a
TARGET   =  data_server
TARGET_MN   =  miner_info
TARGET_MI   =  market_info
TARGET_WD   =  miner_wd
TARGET_RP   =  miner_report

.PHONY: all clean

# Build objects rule
%.o: %.cpp
	$(CC) $< $(CFLAGS) $(INCLUDE) -c -o $@

all: $(TARGET)

# data_server
$(TARGET): $(TARGET).o $(LDLIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lstdc++
	
# miner_info
$(TARGET_MN): $(TARGET_MN).o $(LDLIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lcurl -ljson -lstdc++ -lhiredis -lxlsxwriter
	
# market_info
$(TARGET_MI): $(TARGET_MI).o $(LDLIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lcurl -ljson -lstdc++ -lhiredis -lxlsxwriter
	
# miner_wd
$(TARGET_WD): $(TARGET_WD).o $(LDLIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lstdc++

# miner_report
$(TARGET_RP): $(TARGET_RP).o $(LDLIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS) -lcurl -ljson -lstdc++ -lhiredis -lxlsxwriter

# Clean
clean:
	@rm -f $(TARGET) $(TARGET_MN) $(TARGET_MI) $(TARGET_WD) $(TARGET_RP)
	@rm -f *.o
