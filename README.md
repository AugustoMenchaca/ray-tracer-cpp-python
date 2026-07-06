# Ray Tracer — C++ + Python

Aplicação de Ray Tracing com engine em **C++** e interface gráfica em **Python**.

## Arquivos do Repositório

```
.
├── src/
│   ├── raytracer.h        # Declarações: Vec3, Material, Sphere, Light, Camera
│   └── raytracer.cpp      # Engine: interseção, shading Phong, reflexões, câmera
├── ui/
│   └── main.py            # Interface gráfica: tkinter + ctypes → chama a DLL
├── output/                # Imagens renderizadas (gerado automaticamente)
├── docs/
│   └── documentacao.pdf   # Documentação da implementação
├── Makefile               # Build e execução
└── README.md              # Este arquivo
```

## Dependências

### C++
- `g++` com suporte a C++17 (MSYS2/MinGW no Windows ou GCC no Linux)

### Python
```bash
pip install Pillow
```

## Como Compilar

```bash
make dll
```

Gera `raytracer.dll` (Windows) ou `raytracer.so` (Linux/macOS).

## Como Executar

### Interface gráfica
```bash
make run
```

Abre a janela Python onde você escolhe resolução e amostras por pixel e clica em **RENDERIZAR**.

### Caso de estudo (headless)
```bash
make study
```

Renderiza uma cena 480×270 com 8 amostras/pixel e salva em `output/study.png`, sem abrir janela.

## Arquitetura — Integração entre Linguagens

| Camada | Linguagem | Responsabilidade |
|--------|-----------|-----------------|
| Engine | **C++**   | Cálculo de Ray Tracing (Vec3, interseções, Phong, reflexões, anti-aliasing) |
| Interface | **Python** | Janela gráfica (tkinter), controles, exibição e salvamento da imagem |

A integração é feita via **FFI (Foreign Function Interface)** usando `ctypes`:

1. O C++ compila como biblioteca compartilhada (`-shared -fPIC`)
2. O Python carrega a DLL com `ctypes.CDLL`
3. A função `render(width, height, samples, buffer)` é chamada diretamente
4. O buffer de bytes retornado é convertido em `PIL.Image` e exibido no canvas

## Limpeza

```bash
make clean
```

## Integrantes

- Nome 1
- Nome 2
- Nome 3
