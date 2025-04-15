CC = clang++
VER = 17
OPENCVINCLUDEPATH = /opt/homebrew/opt/opencv/include/opencv4
OPENCVLIBPATH = /opt/homebrew/opt/opencv/lib
OPENCVLIBS = -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
EXEC = app

$(EXEC):
	$(CC) \
  src/main.cpp src/ImageProcessor.cpp external/imnodes/imnodes.cpp external/imgui/*.cpp external/imgui/backends/imgui_impl_glfw.cpp external/imgui/backends/imgui_impl_opengl3.cpp \
  -Iexternal/imgui -Iexternal/imgui/backends -I/opt/homebrew/include -I$(OPENCVINCLUDEPATH) -Iexternal/imnodes -L/opt/homebrew/lib -L$(OPENCVLIBPATH) \
  $(OPENCVLIBS) \
  -lglfw -framework OpenGL \
  -std=c++$(VER) -o $(EXEC)
