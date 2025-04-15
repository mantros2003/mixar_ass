CC = clang++
VER = 17
OPENCVINCLUDEPATH = /opt/homebrew/opt/opencv/include/opencv4
OPENCVLIBPATH = /opt/homebrew/opt/opencv/lib
OPENCVLIBS = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
EXEC = app

$(EXEC):
	$(CC) \
  src/main.cpp imgui/*.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp \
  -Iimgui -Iimgui/backends -I/opt/homebrew/include -I$(OPENCVINCLUDEPATH) -L/opt/homebrew/lib -L$(OPENCVLIBPATH) \
  $(OPENCVLIBS) \
  -lglfw -framework OpenGL \
  -std=c++$(VER) -o $(EXEC)
