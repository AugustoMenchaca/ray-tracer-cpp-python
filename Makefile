# =============================================================
# Makefile — Ray Tracer (C++ engine + Python UI)
# =============================================================
# Alvos principais:
#   make dll      — compila a biblioteca compartilhada (engine C++)
#   make run      — abre a interface gráfica Python
#   make study    — renderiza um caso de estudo sem UI (baixa res)
#   make clean    — remove artefatos gerados
# =============================================================

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -shared
SRC      = src/raytracer.cpp
PYTHON   = python

# No Windows com MSYS2, adicione o ucrt64 ao PATH se necessário:
# export PATH=/c/msys64/ucrt64/bin:/c/msys64/usr/bin:$PATH

ifeq ($(OS),Windows_NT)
    LIB    = raytracer.dll
    LFLAGS =
else
    LIB    = raytracer.so
    LFLAGS = -lm
    CXXFLAGS += -fPIC
endif

# -------------------------------------------------------------
# dll: compila o engine C++ como biblioteca compartilhada
# -------------------------------------------------------------
dll:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(LIB) $(LFLAGS)
	@echo ">> Engine compilado: $(LIB)"

# -------------------------------------------------------------
# run: inicia a interface gráfica Python (requer dll)
# -------------------------------------------------------------
run: dll
	$(PYTHON) ui/main.py

# -------------------------------------------------------------
# study: caso de estudo headless — renderiza 480x270 com 8spp
#        e salva em output/study.png (sem abrir janela)
# -------------------------------------------------------------
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

# -------------------------------------------------------------
# clean: remove a DLL/SO e imagens geradas
# -------------------------------------------------------------
clean:
	rm -f raytracer.dll raytracer.so libraytracer.so
	rm -f output/*.png
	@echo ">> Limpo."

.PHONY: dll run study clean
