CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = log/*.cpp \
       pool/*.cpp \
       buffer/*.cpp \
       test.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET) -pthread -lmysqlclient

clean:
	rm -rf bin/$(OBJS) $(TARGET)