# ---- Tools ----
CXX       ?= g++
CC        ?= gcc
PKG_CONFIG?= pkg-config

# ---- Flags ----
XSTD_CPP  ?= -std=c++17
XSTD_C    ?= -std=c11
CXXFLAGS  ?= -Wall -Wextra -O2 $(XSTD_CPP)
CFLAGS    ?= -Wall -Wextra -O2 $(XSTD_C)
CPPFLAGS  += -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600
LDFLAGS   ?=
LDLIBS    ?= -pthread

# ---- OpenSSL via pkg-config ----
OPENSSL_PKG ?= openssl
ifeq ($(shell $(PKG_CONFIG) --exists $(OPENSSL_PKG) && echo yes),yes)
  CPPFLAGS  += $(shell $(PKG_CONFIG) --cflags $(OPENSSL_PKG))
  CFLAGS    += $(shell $(PKG_CONFIG) --cflags $(OPENSSL_PKG))
  LDLIBS    += $(shell $(PKG_CONFIG) --libs   $(OPENSSL_PKG))
else
  LDLIBS    += -lssl -lcrypto
endif

# ---- Dirs ----
SRC   := src
INC   := include
BIN   := bin
BUILD := build

INCLUDE := -I$(INC)

# ---- Sources ----
COMMON_CPP := \
  $(SRC)/SSLSocket.cc \
  $(SRC)/VSocket.cc \
  $(SRC)/Socket.cc

SERVER_CPP := $(SRC)/SSLServer.cc
CLIENT_CPP := $(SRC)/SSLClient.cc
CLIENT_C   := $(SRC)/SSLClient.c

# ---- Objects (C++) ----
COMMON_OBJS := $(COMMON_CPP:$(SRC)/%.cc=$(BUILD)/%.o)
SERVER_OBJS := $(SERVER_CPP:$(SRC)/%.cc=$(BUILD)/%.o) $(COMMON_OBJS)
CLIENT_OBJS := $(CLIENT_CPP:$(SRC)/%.cc=$(BUILD)/%.o)  $(COMMON_OBJS)

# ---- Objects (C) -> usa nombre distinto para evitar colisión ----
CLIENT_C_OBJ := $(BUILD)/SSLClient_c.o

# ---- Binaries ----
SERVER_BIN    := $(BIN)/sslserver
CLIENT_BIN    := $(BIN)/sslclient_cpp
CLIENT_C_BIN  := $(BIN)/sslclient_c

# ---- Default ----
.PHONY: all clean run-server run-client-cpp run-client-c
all: $(SERVER_BIN) $(CLIENT_BIN) $(CLIENT_C_BIN)

# ---- Link ----
$(SERVER_BIN): $(SERVER_OBJS) | $(BIN)/.
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(CLIENT_BIN): $(CLIENT_OBJS) | $(BIN)/.
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

$(CLIENT_C_BIN): $(CLIENT_C_OBJ) | $(BIN)/.
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

# ---- Compile rules (C++) ----
$(BUILD)/%.o: $(SRC)/%.cc
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) $(CPPFLAGS) -MMD -MP $< -o $@

# ---- Compile rule (C) único para el cliente C) ----
$(CLIENT_C_OBJ): $(CLIENT_C)
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $(INCLUDE) $(CPPFLAGS) -MMD -MP $< -o $@

$(BIN)/.:
	mkdir -p $(BIN)

# ---- Run helpers ----
run-server: $(SERVER_BIN)
	@echo ">> Running server on 0.0.0.0:4321"
	$(SERVER_BIN) 4321

run-client-cpp: $(CLIENT_BIN)
	@echo ">> Running C++ client -> 127.0.0.1:4321"
	$(CLIENT_BIN) 127.0.0.1 4321

run-client-c: $(CLIENT_C_BIN)
	@echo ">> Running C client -> 127.0.0.1:4321"
	$(CLIENT_C_BIN) 127.0.0.1 4321

# ---- Clean ----
clean:
	rm -rf $(BUILD) $(BIN)

# ---- Auto deps ----
-include $(BUILD)/*.d
