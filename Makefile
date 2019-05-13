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

CFLAGS = -std=c99 -Wall -Wextra -pthread
LDFLAGS = -lglfw3 -pthread
OUTPUT = minote
ifeq ($(DEBUG),1)
	CFLAGS += -D_DEBUG -g3
	OUTPUT := $(OUTPUT)-debug
else
	CFLAGS += -O3
	LDFLAGS += -mwindows
endif

ifeq ($(MINGW),1)
	OUTPUT := $(OUTPUT).exe
	LDFLAGS += -lgdi32 -lwinmm
endif

$(OUTPUT): $(obj)
	$(CC) -o $(OUTPUT) $^ $(LDFLAGS)

-include $(dep)

$(BUILD):
	mkdir -p $(BUILD)/glad

$(BUILD)/%.o: $(SOURCE)/%.c $(BUILD)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(BUILD)/%.d: $(SOURCE)/%.c $(BUILD)
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -rf $(BUILD)
	rm -f $(OUTPUT)

.PHONY: cleandep
cleandep:
	rm -f $(dep)