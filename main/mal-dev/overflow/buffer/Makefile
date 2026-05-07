ifeq ($(OS),Windows_NT)
    RM         = del /Q
    NULLDEV    = NUL
    TARGET     = -target x86_64-pc-windows-msvc
    RUN_CMD    = $(EXE)
    MKDIR      = if not exist $(BIN_DIR) mkdir $(BIN_DIR)

else
    RM         = rm -rf
    NULLDEV    = /dev/null
    TARGET     = -target x86_64-w64-mingw32
    RUN_CMD    = wine $(EXE)
    MKDIR      = mkdir -p $(BIN_DIR)
endif

BIN_DIR = bin
EXE     = $(BIN_DIR)/main.exe
CC      = clang

# ⚠️  FLAGS VULNERABLES — eliminamos TODAS las protecciones:
#   -O0                  → sin optimización (stack más predecible)
#   -fno-stack-protector → desactiva stack canary (cookie de protección)
#   -z execstack         → stack ejecutable (permite ejecutar shellcode)
#   -D_FORTIFY_SOURCE=0  → desactiva chequeos de funciones peligrosas
#   -mno-sse2            → evita que el compilador use registros XMM
#                          que alinean el stack de forma impredecible
# En Windows no existe -z execstack, lo omitimos
CFLAGS  = -O0 \
            -fno-stack-protector \
            -D_FORTIFY_SOURCE=0 \
            -mno-sse2 \
            -Wno-deprecated-declarations

LDFLAGS = -lws2_32

SRC_MAIN = main.c

all: $(EXE)

$(EXE): $(SRC_MAIN)
	@echo "Compilando main.exe VULNERABLE para Windows..."
	$(CC) $(CFLAGS) $(TARGET) $(SRC_MAIN) $(LDFLAGS) -o $(EXE)
	$(RUN_CMD)

clean:
	@echo "Limpiando..."
	-$(RM) $(BIN_DIR) 2>$(NULLDEV)

run: $(EXE)
	@echo "Ejecutando $(EXE)..."
	$(RUN_CMD)

.PHONY: all clean run