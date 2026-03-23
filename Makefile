CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -pedantic
SRC := bank_reverse_task.c
GUI_SRC := ybank_win_gui.c
LINUX_OUT := ybank_task
WIN_OUT := ybank_task.exe
WIN_GUI_OUT := ybank_gui.exe
MINGW_CC ?= x86_64-w64-mingw32-gcc

.PHONY: all linux win-exe win-gui-exe clean

all: linux

linux: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(LINUX_OUT)

win-exe: $(SRC)
	@if command -v $(MINGW_CC) >/dev/null 2>&1; then \
		$(MINGW_CC) $(CFLAGS) $(SRC) -o $(WIN_OUT); \
		echo "Built Windows executable: $(WIN_OUT)"; \
	else \
		echo "Error: $(MINGW_CC) not found. Install MinGW-w64 cross-compiler to build .exe"; \
		exit 1; \
	fi

win-gui-exe: $(GUI_SRC)
	@if command -v $(MINGW_CC) >/dev/null 2>&1; then \
		$(MINGW_CC) $(CFLAGS) -mwindows $(GUI_SRC) -o $(WIN_GUI_OUT); \
		echo "Built Windows GUI executable: $(WIN_GUI_OUT)"; \
	else \
		echo "Error: $(MINGW_CC) not found. Install MinGW-w64 cross-compiler to build GUI .exe"; \
		exit 1; \
	fi

clean:
	rm -f $(LINUX_OUT) $(WIN_OUT) $(WIN_GUI_OUT)
