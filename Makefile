PREFIX   ?= $(HOME)/.cradicle
CC       ?= gcc
CFLAGS   ?= -g -fPIC
LDFLAGS  ?=

CFLAGS  += -I/usr/local/include
LDFLAGS += -L/usr/local/lib64

LIBS     = -lssh -lgit2 -ljson-c -lsqlite3

BUILDDIR = build

LIB_SRC = src/radicle/base58.c \
          src/radicle/base64.c \
          src/radicle/cob.c \
          src/radicle/document.c \
          src/radicle/git.c \
          src/radicle/id.c \
          src/radicle/key.c \
          src/radicle/print.c \
          src/radicle/profile.c \
          src/radicle/project.c \
          src/radicle/rad.c \
          src/radicle/repo.c \
          src/radicle/set.c \
          src/radicle/storage.c \
          src/radicle/util.c \
          src/radicle/cob/common.c \
          src/radicle/cob/identity.c \
          src/radicle/cob/issue.c

LIB_OBJ = $(patsubst src/radicle/%.c,$(BUILDDIR)/lib/%.o,$(LIB_SRC))
LIB_OUT = $(BUILDDIR)/libradicle.so

CLI_SRC = src/radicle-cli/main.c \
          src/radicle-cli/command.c \
          src/radicle-cli/commands/auth.c \
          src/radicle-cli/commands/init.c \
          src/radicle-cli/commands/clone.c \
          src/radicle-cli/commands/validate.c \
          src/radicle-cli/commands/issue.c \
          src/radicle-cli/commands/id.c \
          src/radicle-cli/commands/self.c \
          src/radicle-cli/commands/sync.c \
          src/radicle-cli/commands/ls.c \
          src/radicle-cli/commands/inspect.c

CLI_OBJ = $(patsubst src/radicle-cli/%.c,$(BUILDDIR)/cli/%.o,$(CLI_SRC))
CLI_OUT = $(BUILDDIR)/crad

NODE_SRC = src/radicle-node/main.c \
           src/radicle-node/command.c

NODE_OBJ = $(patsubst src/radicle-node/%.c,$(BUILDDIR)/node/%.o,$(NODE_SRC))
NODE_OUT = $(BUILDDIR)/cradicle-node

REMOTE_SRC = src/radicle-remote/main.c \
             src/radicle-remote/push.c \
             src/radicle-remote/list.c \
             src/radicle-remote/fetch.c

REMOTE_OBJ = $(patsubst src/radicle-remote/%.c,$(BUILDDIR)/remote/%.o,$(REMOTE_SRC))
REMOTE_OUT = $(BUILDDIR)/git-remote-rad

SCRIPTS = src/radicle-node/radicle-node-wrapped \
          src/radicle-node/rad-clone-wrapped \
          src/radicle-node/rad-sync-wrapped \
          src/radicle-rsync/crad-rsync

EXECUTABLES = $(CLI_OUT) $(NODE_OUT) $(REMOTE_OUT)

.PHONY: all clean install uninstall

all: $(LIB_OUT) $(EXECUTABLES)

$(BUILDDIR)/lib/%.o: src/radicle/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc/radicle -c -o $@ $<

$(LIB_OUT): $(LIB_OBJ)
	$(CC) -shared -o $@ $^

$(BUILDDIR)/cli/%.o: src/radicle-cli/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc/radicle-cli -Isrc/radicle -c -o $@ $<

$(CLI_OUT): $(CLI_OBJ) $(LIB_OUT)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(BUILDDIR) -o $@ $(CLI_OBJ) -lradicle $(LIBS)

$(BUILDDIR)/node/%.o: src/radicle-node/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc/radicle-node -Isrc/radicle -c -o $@ $<

$(NODE_OUT): $(NODE_OBJ) $(LIB_OUT)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(BUILDDIR) -o $@ $(NODE_OBJ) -lradicle $(LIBS)

$(BUILDDIR)/remote/%.o: src/radicle-remote/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Isrc/radicle-remote -Isrc/radicle -c -o $@ $<

$(REMOTE_OUT): $(REMOTE_OBJ) $(LIB_OUT)
	$(CC) $(CFLAGS) $(LDFLAGS) -L$(BUILDDIR) -o $@ $(REMOTE_OBJ) -lradicle $(LIBS)

install: all
	install -d $(PREFIX)/bin $(PREFIX)/lib
	install -m 755 $(CLI_OUT) $(PREFIX)/bin/crad
	install -m 755 $(NODE_OUT) $(PREFIX)/bin/cradicle-node
	install -m 755 $(REMOTE_OUT) $(PREFIX)/bin/git-remote-rad
	install -m 755 $(SCRIPTS) $(PREFIX)/bin/
	install -m 755 $(LIB_OUT) $(PREFIX)/lib/libradicle.so

uninstall:
	rm -f $(PREFIX)/bin/crad
	rm -f $(PREFIX)/bin/cradicle-node
	rm -f $(PREFIX)/bin/git-remote-rad
	rm -f $(PREFIX)/bin/radicle-node-wrapped
	rm -f $(PREFIX)/bin/rad-clone-wrapped
	rm -f $(PREFIX)/bin/rad-sync-wrapped
	rm -f $(PREFIX)/bin/crad-rsync
	rm -f $(PREFIX)/lib/libradicle.so

clean:
	rm -rf $(BUILDDIR)
