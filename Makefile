CXX      = g++
CXXFLAGS = -std=c++17 -O2 -shared
SRC      = src/raytracer.cpp
PYTHON   = python

ifeq ($(OS),Windows_NT)
    # Se o MSYS2 estiver instalado no local padrão, adiciona ao PATH para garantir o g++ de 64-bit e utilitários Unix
    ifneq ($(wildcard C:/msys64/mingw64/bin),)
        export PATH := C:\msys64\mingw64\bin;C:\msys64\usr\bin;$(PATH)
    endif
    LIB    = raytracer.dll
    LFLAGS =
else
    LIB    = raytracer.so
    LFLAGS = -lm
    CXXFLAGS += -fPIC
    PYTHON   = python3
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

setup:
ifeq ($(OS),Windows_NT)
	$(PYTHON) -m pip install -r requirements.txt
else
	@echo "Instalando dependencias de sistema e Python no Linux/WSL..."
	sudo apt update && sudo apt install build-essential python3 python3-pip python3-tk python3-pil.imagetk -y
	$(PYTHON) -m pip install --break-system-packages -r requirements.txt || $(PYTHON) -m pip install -r requirements.txt
endif

clean:
ifeq ($(OS),Windows_NT)
    ifeq ($(findstring sh,$(SHELL)),sh)
		-rm -f $(LIB) output/*.png 2>/dev/null
    else
		-del /q /f $(LIB) 2>nul
		-del /q /f output\*.png 2>nul
    endif
else
	rm -f $(LIB) libraytracer.so output/*.png
endif

.PHONY: dll run study clean setup
