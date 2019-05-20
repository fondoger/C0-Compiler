# g++ 头文件查找方式
# 对于#incldue "xxx.h"这类头文件, 若未在编译命令中指明头文件,
# 则g++会自动在当前目录中查找头文件

executable = test

build_dir = build
src_dir = src

sources = $(wildcard $(src_dir)/*.cpp)
objects = $(patsubst $(src_dir)/%.cpp, $(build_dir)/%.o, $(sources))
gxxflags = -std=c++11 -Wall -Wextra

# -Wall 将warning当作error
# -Wextra 显示额外的信息(比如空循环)

$(executable): $(objects)
	@# 下面的$@等价于$(target)
	@# 下面的$^等价于$(objects)
	g++ $(gxxflags) -o $@ $^
	
	# Use `./test hello_world.txt` to run
	# Or use `make run` as a shortcut

debug: $(objects)
	g++ $(gxxflags) -g -o $@ $^


# %.o 匹配所有以.o结尾的目标名
#%.o: %.cpp
$(objects): $(build_dir)/%.o : $(src_dir)/%.cpp
	@mkdir -p build
	@# 下面的$@代表当前语句的目标, 相当于%.o
	@# $< 表示第一个依赖的目标, 相当于%.cpp
	g++ $(gxxflags) -c $< -o $@ 

run:
	./test hello_world.txt


clean:
	rm -f $(target) $(objects)







