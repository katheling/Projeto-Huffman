# Como Compilar no CMD

### 1. Preparação
Crie uma pasta no seu computador e coloque dentro dela o código-fonte (**`huffman.c`**) junto com o arquivo de texto que deseja testar (ex: **`arquivoteste.txt`**).

### 2. Abrindo o Terminal (CMD) na Pasta
Abra a pasta criada no Explorador de Arquivos do Windows, clique na **barra de endereços** no topo, digite **`cmd`** e pressione **Enter**. 

### 3. Comandos
Com o CMD aberto dentro da pasta, execute os comandos abaixo em sequência para **compilar**, **compactar** e testar a **descompactação**:

**Compilar:** **`gcc -Wall -Wextra -O3 huffman.c -o huffman.exe`**

**Compactar:** **`huffman.exe c arquivoteste.txt compactado.huff`**

**Descompactar:** **`huffman.exe d compactado.huff restaurado.txt`**
