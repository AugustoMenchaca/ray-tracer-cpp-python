CXX      = g++
CXXFLAGS = -std=c++17 -O2 -shared
SRC      = src/raytracer.cpp
PYTHON   = python

ifeq ($(OS),Windows_NT)
    LIB    = raytracer.dll
    LFLAGS =
else
    LIB    = raytracer.so
    LFLAGS = -lm
    CXXFLAGS += -fPIC
endif

dll:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(LIB) $(LFLAGS)

run: dll
	$(PYTHON) ui/main.py

study: dll
	$(PYTHON) -c "\
import ctypes, os; \
from PIL import Image; \
lib = ctypes.CDLL('./$(LIB)'); \
lib.render.restype = None; \
lib.render.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_uint8)]; \
w, h, s = 480, 270, 8; \
buf = (ctypes.c_uint8 * (w*h*3))(); \
lib.render(w, h, s, buf); \
img = Image.frombytes('RGB', (w, h), bytes(buf)); \
os.makedirs('output', exist_ok=True); \
img.save('output/study.png'); \
print('>> Caso de estudo salvo em output/study.png') \
"

clean:
	rm -f raytracer.dll raytracer.so libraytracer.so
	rm -f output/*.png

.PHONY: dll run study clean
