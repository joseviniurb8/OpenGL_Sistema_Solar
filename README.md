# Sistema Solar - OpenGL

O trabalho consiste em uma simulação tridimensional do Sistema Solar, desenvolvida como projeto prático para a disciplina de Computação Gráfica. A implementação utiliza OpenGL e C++ para demonstrar conceitos fundamentais da área através da modelagem e animação de corpos celestes em um ambiente virtual.

---

##  O que o código faz?

- Renderiza o Sistema Solar 3D, com cada planeta em rotação e translação.  
- Utiliza curvas paramétricas para o movimento orbital dos planetas.  
- Possui iluminação e sombreamento para dar profundidade e realidade à cena.  
- Implementa mapeamento de texturas para aplicar imagens reais dos planetas.  
- Inclui oclusão correta.  
- Câmera dinâmica com possibilidade de controle pelo usuário.  
- Representação de elementos extras, como o anel de Saturno.  
- Exibição opcional das órbitas planetárias visíveis.  

---

## Imagem do programa

## Como compilar e executar

### Passo 1 – Clonar o repositório
```bash
git clone https://github.com/joseviniurb8/OpenGL_Sistema_Solar.git
cd OpenGL_Sistema_Solar
```

### Passo 2 – Verificar as Dependências
O projeto depende de:  
- OpenGL  
- GLUT (FreeGLUT)
- stb_image.h (arquivo incluído no projeto para carregar texturas)

### Passo 3 – Compilar
Compile o código com o seguinte comando:
```bash
gcc main.c -o sistema_solar -lGL -lGLU -lglut -lm
```

### Passo 4 – Executar
Depois de compilar, rode:
```bash
./sistema_solar
```

## Controles do teclado

- **Z** → Aproximar a câmera  
- **z** → Afastar a câmera  
- **w** → Aumentar altura da câmera  
- **s** → Diminuir altura da câmera  
- **a** → Girar câmera manualmente para a esquerda  
- **d** → Girar câmera manualmente para a direita  
- **p** → Pausar/retomar movimento  
- **o** → Mostrar/ocultar órbitas  

---

## Principais problemas enfrentados

- Ajuste da iluminação para que o brilho ficasse proporcional ao Sistema Solar.  
- Balanceamento dos raios das órbitas para evitar que planetas colidissem visualmente.  
- Dificuldade na configuração da câmera para manter uma boa perspectiva geral.  

---

## Possíveis melhorias

- Adicionar efeitos de luz mais realistas, como reflexões e reflexos atmosféricos.  
- Inserir satélites naturais e outros corpos celestes como asteroides e cometas.  

---

## Integrantes e contribuições

- José Vinícius Urbano → Desenvolvimento das translações, rotações planetárias e das órbitas paramétricas.  
- Ana Carolina → Implementação das texturas e controle da câmera.  
- João Granvile → Implementação da iluminação e ajustes gerais no código.
