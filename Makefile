CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Wno-format-truncation
TARGET = myz
SRC = myz.c utils.c \
      c_flag/c_flag.c \
      x_flag/x_flag.c \
      a_flag/a_flag.c \
      d_flag/d_flag.c \
      m_flag/m_flag.c \
      q_flag/q_flag.c \
      p_flag/p_flag.c

OBJ_DIR = build

OBJ = $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
