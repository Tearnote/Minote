SOURCE = src
BUILD = build
ifeq ($(DEBUG),1)
	BUILD := $(BUILD)-debug
else
	BUILD := $(BUILD)-release
endif

src = $(wildcard $(SOURCE)/*.c) \
      $(wildcard $(SOURCE)/glad/*.c)
obj = $(src:$(SOURCE)/%.c=$(BUILD)/%.o)
dep = $(obj:.o=.d)
glslsrc = $(wildcard $(SOURCE)/glsl/*.glsl)
glslobj = $(glslsrc:$(SOURCE)/%=$(BUILD)/%)
glslobj := $(basename $(glslobj))

CFLAGS = -pipe -std=c99 -pthread -Wall -Wextra -Wfloat-equal -Wundef -Wshadow \
         -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 \
         -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -flto\
         -Wswitch-enum -Wconversion -Wunreachable-code -Wformat=2 -Winit-self \
         -Wmissing-prototypes -I$(BUILD)/glsl
LDFLAGS = -lglfw3 -pthread
OUTPUT = minote
ifeq ($(DEBUG),1)
	CFLAGS += -D_DEBUG -Og -g3 -D_FORTIFY_SOURCE=2 -fstack-protector-strong   \
              -fasynchronous-unwind-tables -fexceptions -fno-omit-frame-pointer
	LDFLAGS += -lssp
	OUTPUT := $(OUTPUT)-debug
else
	CFLAGS += -O2
	LDFLAGS += -s
	ifeq ($(MINGW),1)
		LDFLAGS += -mwindows
	endif
endif

ifeq ($(MINGW),1)
	OUTPUT := $(OUTPUT).exe
	LDFLAGS += -lgdi32 -lwinmm
endif

LDFLAGS += $(CFLAGS)

$(OUTPUT): $(obj)
	$(CC) -o $(OUTPUT) $^ $(LDFLAGS)

-include $(dep)

$(BUILD):
	mkdir -p $(BUILD)/glad
	mkdir -p $(BUILD)/glsl

$(BUILD)/%.o: $(SOURCE)/%.c | $(BUILD) $(glslobj)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/%.d: $(SOURCE)/%.c | $(BUILD)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.SECONDARY: $(glslobj)
$(BUILD)/glsl/%: $(SOURCE)/glsl/%.glsl
	cat $< |xxd -i >$@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)