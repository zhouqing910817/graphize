CC := g++
CFLAGS := -c -g -pipe -pthread -fpermissive -std=c++11 -fno-omit-frame-pointer -IM
LDFLAGS := -pthread -ldl -lz -lm -L/usr/local/Cellar/openssl\@1.1/1.1.1j/lib/ -lssl -lcrypto -lprotobuf -lleveldb -lgflags -lsnappy  -lcpp-hocon -lboost_system -lboost_filesystem -lboost_locale-mt -lboost_thread-mt -lboost_regex  -lleatherman_util -lleatherman_locale -L../thirdparty/brpc/lib64 -lbrpc -L../thirdparty/shmcache/lib64 -lshmcache -L../thirdparty/fastcommon/lib64 -lfastcommon
INCLUDES := -I./ -I/usr/local/include/ -I./src -Isrc/pb_include -I../thirdparty/brpc/include -I../thirdparty/fastcommon/include -I../thirdparty/shmcache/include -I../thirdparty/gtest/include -I/usr/local/Cellar/openssl\@1.1/1.1.1j/include/ -I./proto

SOURCE_cpp=$(wildcard ./*.cpp)\
           $(wildcard frame/client/*/*.cpp) \
           $(wildcard frame/*.cpp) \
           $(wildcard common/*.cpp) \
           $(wildcard service/*.cpp) \
           $(wildcard service/base/*.cpp) \
           $(wildcard server/*.cpp)
SOURCE_cc= $(wildcard proto/*.cc)
SOURCE=$(SOURCE_cpp) $(SOURCE_cc)
OBJECTS=$(SOURCE_cpp:.cpp=.o) $(SOURCE_cc:.cc=.o)
HEAD_DEP=$(OBJECTS:.h=.o)
TARGET = graphize
.PHONY: all clean run
all: $(TARGET)
$(TARGET): $(OBJECTS) $(HEAD_DEP)
	$(CC) $(LDFLAGS) $^ -o $@
#-include $(HEAD_DEP)
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^
.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

run:
	./$(TARGET)

clean:
	\rm -rf $(OBJECTS) $(TARGET)
