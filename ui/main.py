"""
ui/main.py — Interface gráfica do Ray Tracer
Linguagem: Python 3

Responsabilidade desta camada:
  - Criar a janela e os controles (tkinter)
  - Chamar o engine C++ via ctypes (FFI)
  - Receber o buffer de pixels e exibir a imagem (PIL/Pillow)

A comunicação entre Python e C++ se dá por meio de uma
biblioteca compartilhada (raytracer.dll / raytracer.so)
carregada em tempo de execução com ctypes.CDLL.
"""

import tkinter as tk
from tkinter import ttk, messagebox
import ctypes
import os
import sys
import threading
import time
from PIL import Image, ImageTk

# ============================================================
#  Localização da biblioteca C++
# ============================================================

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR   = os.path.dirname(SCRIPT_DIR)

# Tenta .dll (Windows) depois .so (Linux/macOS)
LIB_CANDIDATES = [
    os.path.join(ROOT_DIR, "raytracer.dll"),
    os.path.join(ROOT_DIR, "raytracer.so"),
    os.path.join(ROOT_DIR, "libraytracer.so"),
]

def load_library():
    for path in LIB_CANDIDATES:
        if os.path.exists(path):
            lib = ctypes.CDLL(path)
            # Assinatura: void render(int w, int h, int samples, uint8_t* buf)
            lib.render.restype  = None
            lib.render.argtypes = [
                ctypes.c_int,
                ctypes.c_int,
                ctypes.c_int,
                ctypes.POINTER(ctypes.c_uint8),
            ]
            return lib
    return None

# ============================================================
#  Chamada ao engine C++
# ============================================================

def call_render(lib, width, height, samples):
    """
    Aloca o buffer, chama render() da DLL e devolve
    um objeto PIL.Image com o resultado.
    """
    buf_size = width * height * 3
    buf      = (ctypes.c_uint8 * buf_size)()

    lib.render(width, height, samples, buf)

    # Converte buffer bruto para imagem PIL (modo RGB)
    img = Image.frombytes("RGB", (width, height), bytes(buf))
    return img

# ============================================================
#  Interface gráfica (tkinter)
# ============================================================

class RayTracerApp(tk.Tk):
    def __init__(self):
        super().__init__()

        self.lib = load_library()
        self.title("Ray Tracer — C++ Engine / Python UI")
        self.resizable(False, False)
        self.configure(bg="#1a1a2e")

        self._build_ui()
        self._rendered_image = None

    # --------------------------------------------------------
    #  Construção da UI
    # --------------------------------------------------------

    def _build_ui(self):
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("TLabel",  background="#1a1a2e", foreground="#e0e0e0", font=("Courier", 10))
        style.configure("TButton", background="#e94560", foreground="#ffffff",
                        font=("Courier", 10, "bold"), relief="flat", padding=6)
        style.map("TButton", background=[("active", "#c73652")])
        style.configure("TScale",  background="#1a1a2e", troughcolor="#16213e",
                        sliderthickness=14)

        # ── Painel de controles (esquerda) ──────────────────
        ctrl = tk.Frame(self, bg="#16213e", padx=14, pady=14)
        ctrl.pack(side=tk.LEFT, fill=tk.Y)

        tk.Label(ctrl, text="⚙ CONFIGURAÇÕES", bg="#16213e",
                 fg="#e94560", font=("Courier", 11, "bold")).pack(pady=(0, 12))

        # Resolução
        self._add_label(ctrl, "Largura (px):")
        self.var_width = tk.IntVar(value=640)
        self._add_spin(ctrl, self.var_width, 64, 1920, 64)

        self._add_label(ctrl, "Altura (px):")
        self.var_height = tk.IntVar(value=360)
        self._add_spin(ctrl, self.var_height, 64, 1080, 64)

        # Amostras por pixel
        self._add_label(ctrl, "Amostras/pixel:")
        self.var_samples = tk.IntVar(value=4)
        self._add_spin(ctrl, self.var_samples, 1, 64, 1)

        ttk.Separator(ctrl, orient="horizontal").pack(fill=tk.X, pady=10)

        # Botão render
        ttk.Button(ctrl, text="▶  RENDERIZAR", command=self._start_render).pack(fill=tk.X)

        # Botão salvar
        ttk.Button(ctrl, text="💾  SALVAR PNG", command=self._save_image).pack(fill=tk.X, pady=(6, 0))

        ttk.Separator(ctrl, orient="horizontal").pack(fill=tk.X, pady=10)

        # Status / tempo
        self.lbl_status = tk.Label(ctrl, text="Aguardando...", bg="#16213e",
                                   fg="#a0a0c0", font=("Courier", 9), wraplength=160)
        self.lbl_status.pack()

        # ── Canvas de exibição da imagem (direita) ──────────
        self.canvas = tk.Canvas(self, width=640, height=360,
                                bg="#0f0f1a", highlightthickness=0)
        self.canvas.pack(side=tk.RIGHT, padx=8, pady=8)
        self._draw_placeholder()

    def _add_label(self, parent, text):
        tk.Label(parent, text=text, bg="#16213e", fg="#a0c4ff",
                 font=("Courier", 9)).pack(anchor="w", pady=(8, 0))

    def _add_spin(self, parent, var, from_, to, increment):
        sb = tk.Spinbox(parent, textvariable=var, from_=from_, to=to,
                        increment=increment, width=10,
                        bg="#0f3460", fg="#e0e0e0", insertbackground="#e0e0e0",
                        relief="flat", font=("Courier", 10))
        sb.pack(anchor="w")

    def _draw_placeholder(self):
        self.canvas.create_text(320, 180, text="[ Nenhuma imagem renderizada ]",
                                fill="#3a3a5c", font=("Courier", 13))

    # --------------------------------------------------------
    #  Render (thread separada para não travar a UI)
    # --------------------------------------------------------

    def _start_render(self):
        if self.lib is None:
            messagebox.showerror(
                "Biblioteca não encontrada",
                "raytracer.dll / raytracer.so não encontrado.\n"
                "Execute 'make dll' antes de rodar a interface."
            )
            return

        w = self.var_width.get()
        h = self.var_height.get()
        s = self.var_samples.get()

        self.lbl_status.config(text=f"Renderizando {w}×{h}, {s} spp...", fg="#ffda77")
        self.update_idletasks()

        # Redimensiona o canvas
        self.canvas.config(width=w, height=h)

        # Roda em thread para não bloquear a UI
        t = threading.Thread(target=self._render_thread, args=(w, h, s), daemon=True)
        t.start()

    def _render_thread(self, w, h, s):
        t0  = time.perf_counter()
        img = call_render(self.lib, w, h, s)
        elapsed = time.perf_counter() - t0

        self._rendered_image = img
        self.after(0, self._display_image, img, elapsed)

    def _display_image(self, img, elapsed):
        # Exibe a imagem no canvas via PIL→ImageTk
        self._tk_img = ImageTk.PhotoImage(img)
        self.canvas.delete("all")
        self.canvas.create_image(0, 0, anchor="nw", image=self._tk_img)
        self.lbl_status.config(
            text=f"✔ Concluído em {elapsed:.2f}s\n{img.width}×{img.height}",
            fg="#77ffaa"
        )

    # --------------------------------------------------------
    #  Salvar imagem
    # --------------------------------------------------------

    def _save_image(self):
        if self._rendered_image is None:
            messagebox.showinfo("Nenhuma imagem", "Renderize antes de salvar.")
            return
        out_dir  = os.path.join(ROOT_DIR, "output")
        os.makedirs(out_dir, exist_ok=True)
        filename = os.path.join(out_dir, f"render_{int(time.time())}.png")
        self._rendered_image.save(filename)
        self.lbl_status.config(text=f"Salvo em:\n{filename}", fg="#77aaff")

# ============================================================
#  Ponto de entrada
# ============================================================

if __name__ == "__main__":
    app = RayTracerApp()
    app.mainloop()
