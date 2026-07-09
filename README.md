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
- `Pillow>=10.0.0`

## Como Configurar e Rodar

### 1. Instalar as dependências
Execute o comando a seguir para preparar o ambiente instalando as dependências de Python (e de sistema, se estiver no Linux):
```bash
make setup
```

### 2. Compilar e executar a interface gráfica
Execute o comando a seguir para compilar a biblioteca de ray tracing e abrir a janela de controle do aplicativo:
```bash
make run
```

### 3. Executar o caso de estudo (sem interface)
Para renderizar uma cena padrão diretamente pelo terminal (salva automaticamente em `output/study.png`):
```bash
make study
```

## Limpeza

Para remover arquivos compilados e imagens geradas temporárias:
```bash
make clean
```

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

- Inácio Teixeira
- Hiago Muniz
- Augusto Menchaca
