.PHONY: all release debug install
.PHONY: run run-debug
.PHONY: clean clean-debug clean-release clean-gprof
.PHONY: gprof gprof-view perf-view
.PHONY: memcheck-valgrind
CMAKE_FLAGS=
RUN_PARAMS=

ifeq ($(OS),Windows_NT)
  ifeq ($(TERM),xterm)
    CMAKE_FLAGS= -G"MSYS Makefiles"
  endif
endif

TARGET=varLisp
TARGET_D = $(TARGET)D
TARGET_G = $(TARGET)G

# usage: make -verbose=1
ifeq ($(verbose),1)
	CMAKE_FLAGS+=-DCMAKE_VERBOSE_MAKEFILE=on
else
	CMAKE_FLAGS+=-DCMAKE_VERBOSE_MAKEFILE=off
endif

all: release
release:
	@mkdir -p Release
	cd Release && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/bin .. && make

debug:
	@mkdir -p Debug
	@cd Debug && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug .. && make

gprof:
	@mkdir -p Gprof
	@cd Gprof && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Gprof .. && make

run: release
	@./$(TARGET)

run-debug: debug
	@gdb ./$(TARGET_D)

gprof-view: gprof
	@if [ ! -f gmon.out ]; then ./$(TARGET_G) $(RUN_PARAMS); fi
	@gprof -b --demangle ./$(TARGET_G) gmon.out | gprof2dot -s | xdot &

# NOTE: perf.data need execute bin-pg with parameters!
perf-view: gprof
	@if [ ! -f perf.data ]; then perf record -g -- ./$(TARGET_G) $(RUN_PARAMS); fi
	@perf script | c++filt | gprof2dot -s -f perf | xdot &

memcheck-valgrind: debug
	@valgrind --tool=memcheck --leak-check=full --error-limit=no --show-leak-kinds=all ./$(TARGET_D) $(RUN_PARAMS)

#valgrind --tool=memcheck --log-file=/home/trunk/valgrind_log_all --leak-check=full --error-limit=no --show-leak-kinds=all /opt/lim/bin/limserver
#valgrind --leak-check=yes --trace-children=yes --show-reachable=yes --log-file=log program args
#说明：
#valgrind是一个调试程序的工具集，可以检测Memcheck, Addrcheck, Cachegrind等。
#--leak-check=<no|summary|yes|full> [default: summary]
#	指的是泄露检测信息输出方式；full:完全检查内存泄漏
#   选择summary,只有统计概要输出
#--show-reachable=<yes|no> [default: no]
# 	是否显示内存泄漏的地点
# 	如果这个禁用，那只检测那些已经丢失指针指向的那些内存块
#--trace-children=<yes|no>
#	是否跟入子进程。当程序正常退出的时候valgrind自然会输出内存泄漏的信息。对于多线程可加可不加
#--log-file=log
#	输出检测的日志文件，不加会直接显示在shell中，valgrind会在log名后加.pid
#--num-callers=N
#	指定报告中调用栈的层数，这在定位和跟踪错误的时候会比较有用
#! http://www.cnblogs.com/cobbliu/p/4423775.html

install:
	@cd Release && make install

clean: clean-debug clean-release clean-gprof
clean-release:
	@if [ -f Release/Makefile ]; then cd Release && make clean; fi
	@if [ -d Release ]; then rm -rf Release; fi

clean-debug:
	@if [ -f Debug/Makefile ]; then cd Debug && make clean; fi
	@if [ -d Debug ]; then rm -rf Debug; fi

clean-gprof:
	@if [ -f gmon.out ]; then rm gmon.out ; fi
	@if [ -f perf.data ]; then rm perf.data* ; fi
	@if [ -f Gprof/Makefile ]; then cd Gprof && make clean; fi
	@if [ -d Gprof ]; then rm -rf Gprof; fi
