#include "buffer.h"

static int ishex(int c) {
    return isdigit(c) || (c >= 97 && c <= 102) || (c >= 65 && c <= 70);
}

static long filesize(FILE *fp) {
    long size;
    if (fseek(fp, 0, SEEK_END) == -1)
        return -1; 
    if ((size = ftell(fp)) == -1)
        return -1;
    rewind(fp);
    return size;
}

static char hex2bit(char ch) {
    char hexstr[2] = { ch, '\0' };
    if (!ishex(ch))
        return -1;
    return strtol(hexstr, NULL, 16);
}

void buf_init(char *filename) {
    buf.filename = filename;
    buf.fp = fopen(filename, "r+");
    CHECK(buf.fp != NULL);
    buf.size = filesize(buf.fp);
    CHECK(buf.size != -1);
    buf.mem = malloc(buf.size * sizeof(char));
    CHECK(buf.mem != NULL);
    fread(buf.mem, sizeof(char), buf.size, buf.fp);
    CHECK(ferror(buf.fp) == 0);
    rewind(buf.fp);

    buf.index = 0;
    buf.mode = HEX;
    buf.state = ESCAPE;
}

void buf_free() {
    if ( buf.fp )
        fclose(buf.fp);
    free(buf.mem);
}

void buf_write() {
    size_t write_size;
    rewind(buf.fp);
    write_size = fwrite(buf.mem, sizeof(char), buf.size, buf.fp);
    if (write_size < buf.size)
        clearerr(buf.fp);
}

void buf_revert() {
    rewind(buf.fp);
    fread(buf.mem, sizeof(char), buf.size, buf.fp);
    if (ferror(buf.fp) != 0)
        clearerr(buf.fp);
}

void buf_setindex(int index, bool nybble) {
    if (index < 0) { 
        buf.index = 0;
        buf.nybble = false;
    } else if (index >= buf.size) {
        buf.index = buf.size - 1;
        buf.nybble = true;
    } else {
        buf.index = index;
        buf.nybble = nybble;
    }
}

void buf_putchar(char ch) {
    if (buf.mode == HEX && ishex(ch)) {
        char orig = buf.mem[buf.index];
        ch = hex2bit(ch);
        if (buf.nybble) {
            orig &= 0xF0;
            ch |= orig;
            buf.mem[buf.index] = ch;
        } else { 
            orig &= 0x0F;
            ch <<= 4;
            ch |= orig;
            buf.mem[buf.index] = ch;
        }
    } else if (buf.mode == ASCII) {
        buf.mem[buf.index] = ch;
    }
}

void buf_revertchar() {
    fseek(buf.fp, buf.index, SEEK_SET);
    char orig = fgetc(buf.fp);
    if (buf.mode == HEX) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", orig);
        buf_putchar(buf.nybble ? hex[1] : hex[0]);
    } else if (buf.mode == ASCII) {
        buf_putchar(orig);
    }
}

