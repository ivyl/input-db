CXX := x86_64-w64-mingw32-g++
LIBS := -ldinput8 -lgdi32 -lhid -static-libgcc -static-libstdc++ -static

%.exe: %.cpp
	$(CXX) $< -o $@ $(LIBS)


all: dinput-dumper.exe
