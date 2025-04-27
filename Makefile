# 获取当前目录下的所有cpp文件
SRC = $(wildcard *.cpp) 
# 将src中的所有.cpp文件替换为.o文件
OBJS = $(patsubst %.cpp,%.o,$(SRC))
# 编译器
CC = g++
CFLAG =
# 头文件包含路径
INCLUDE = -I.
# 库文件
LIBS = -lpthread -lsamplerate
# 库文件
LIBS_PATH =
# 目标执行文件
TARGET = test

$(TARGET): $(OBJS)
	$(CC) $(CFLAG) -o $(TARGET) $(OBJS) $(LIBS_PATH) $(LIBS)
	rm -f $(OBJS)

$(OBJS): $(SRC)	
	$(CC) $(CFLAG) -c $(SRC) $(INCLUDE) $(LIBS_PATH) $(LIBS)

.PHONY: clean
clean:
	rm -f *.o $(TARGET)


