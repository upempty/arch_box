
CXX := g++
TARGET := streamout
SRC := streamout.cpp

INC_DIR := /userdata/stream/myusr/include
LIB_DIR := /userdata/stream/myusr/lib

CXXFLAGS := -I$(INC_DIR) -fpermissive -Wall -Wextra
LDFLAGS := -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR)
LIBS := -lavformat -lavfilter -lavcodec -lavutil -lavdevice -lswscale -lavfilter -lpthread

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
#g++ streamout.cpp -o streamout -I /userdata/stream/myusr/include -L/userdata/stream/myusr/lib \ 
#-lavformat -lavfilter -lavcodec -lavutil -lavdevice -lswscale -lavfilter  -lpthread -fpermissive \
#-Wl,-rpath,/userdata/stream/myusr/lib
