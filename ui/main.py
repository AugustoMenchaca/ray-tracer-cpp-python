import tkinter as tk
from tkinter import ttk, messagebox
import ctypes
import os
import sys
import threading
import time
from PIL import Image, ImageTk

pasta_atual = os.path.dirname(os.path.abspath(__file__))
pasta_raiz = os.path.dirname(pasta_atual)

def load_engine():
    # Carrega a DLL apropriada dependendo do Sistema Operacional
    if sys.platform == "win32":
        arquivos_dll = [os.path.join(pasta_raiz, "raytracer.dll")]
    else:
        arquivos_dll = [
            os.path.join(pasta_raiz, "raytracer.so"),
            os.path.join(pasta_raiz, "libraytracer.so"),
        ]

    for dll in arquivos_dll:
        if os.path.exists(dll):
            lib = ctypes.CDLL(dll)
            lib.render.restype = None
            lib.render.argtypes = [
                ctypes.c_int,
                ctypes.c_int,
                ctypes.c_int,
                ctypes.POINTER(ctypes.c_uint8),
            ]
            return lib
    return None

def chama_engine(lib, width, height, samples):
    buf_size = width * height * 3
    buf = (ctypes.c_uint8 * buf_size)()
    lib.render(width, height, samples, buf)
    return Image.frombytes("RGB", (width, height), bytes(buf))

class AppRayTracer(tk.Tk):
    def __init__(self):
        super().__init__()

        self.lib = load_engine()
        self.title("Ray Tracer - C++ e Python")
        self.resizable(False, False)
        self.configure(bg="#1a1a2e")

        self.img_atual = None
        self.monta_tela()

    def monta_tela(self):
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("TButton", background="#e94560", foreground="#ffffff", font=("Courier", 10, "bold"), relief="flat", padding=6)
        
        menu = tk.Frame(self, bg="#16213e", padx=14, pady=14)
        menu.pack(side=tk.LEFT, fill=tk.Y)

        tk.Label(menu, text="CONFIGURACOES", bg="#16213e", fg="#e94560", font=("Courier", 11, "bold")).pack(pady=(0, 12))

        tk.Label(menu, text="Largura (px):", bg="#16213e", fg="#a0c4ff", font=("Courier", 9)).pack(anchor="w")
        self.val_largura = tk.IntVar(value=640)
        tk.Spinbox(menu, textvariable=self.val_largura, from_=64, to=1920, increment=64, width=10).pack(anchor="w", pady=(0, 10))

        tk.Label(menu, text="Altura (px):", bg="#16213e", fg="#a0c4ff", font=("Courier", 9)).pack(anchor="w")
        self.val_altura = tk.IntVar(value=360)
        tk.Spinbox(menu, textvariable=self.val_altura, from_=64, to=1080, increment=64, width=10).pack(anchor="w", pady=(0, 10))

        tk.Label(menu, text="Amostras/pixel:", bg="#16213e", fg="#a0c4ff", font=("Courier", 9)).pack(anchor="w")
        self.val_samples = tk.IntVar(value=4)
        tk.Spinbox(menu, textvariable=self.val_samples, from_=1, to=64, increment=1, width=10).pack(anchor="w", pady=(0, 10))

        ttk.Button(menu, text="RENDERIZAR", command=self.btn_render).pack(fill=tk.X, pady=5)
        ttk.Button(menu, text="SALVAR IMAGEM", command=self.btn_salvar).pack(fill=tk.X)

        self.texto_status = tk.Label(menu, text="Pronto.", bg="#16213e", fg="#a0a0c0", font=("Courier", 9), wraplength=160)
        self.texto_status.pack(pady=10)

        self.tela_img = tk.Canvas(self, width=640, height=360, bg="#0f0f1a", highlightthickness=0)
        self.tela_img.pack(side=tk.RIGHT, padx=8, pady=8)
        self.tela_img.create_text(320, 180, text="Aguardando render...", fill="#3a3a5c", font=("Courier", 13))

    def btn_render(self):
        if not self.lib:
            messagebox.showerror("Erro", "Engine DLL nao encontrado. Rode make dll antes.")
            return

        w = self.val_largura.get()
        h = self.val_altura.get()
        s = self.val_samples.get()

        self.texto_status.config(text=f"Calculando {w}x{h}...", fg="#ffda77")
        self.tela_img.config(width=w, height=h)
        
        t = threading.Thread(target=self.roda_engine, args=(w, h, s), daemon=True)
        t.start()

    def roda_engine(self, w, h, s):
        t0 = time.perf_counter()
        img = chama_engine(self.lib, w, h, s)
        tempo = time.perf_counter() - t0

        self.img_atual = img
        self.after(0, self.mostra_resultado, img, tempo)

    def mostra_resultado(self, img, tempo):
        self.tk_img = ImageTk.PhotoImage(img)
        self.tela_img.delete("all")
        self.tela_img.create_image(0, 0, anchor="nw", image=self.tk_img)
        self.texto_status.config(text=f"Feito em {tempo:.2f}s", fg="#77ffaa")

    def btn_salvar(self):
        if not self.img_atual: return
        pasta_out = os.path.join(pasta_raiz, "output")
        os.makedirs(pasta_out, exist_ok=True)
        arq = os.path.join(pasta_out, f"render_{int(time.time())}.png")
        self.img_atual.save(arq)
        self.texto_status.config(text=f"Salvo em:\n{arq}")

if __name__ == "__main__":
    app = AppRayTracer()
    app.mainloop()
