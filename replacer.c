#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_CYAN    "\033[1;36m"

typedef enum {
    ENCODING_UTF8,
    ENCODING_WIN1251,
    ENCODING_DOS866,
    ENCODING_KOI8R
} Encoding;

typedef struct {
    unsigned char *search_bytes;
    size_t search_len;
    unsigned char *replace_bytes;
    size_t replace_len;
    int delete_mode;
} Operation;

int get_codepage(Encoding enc) {
    switch (enc) {
        case ENCODING_WIN1251: return 1251;
        case ENCODING_DOS866: return 866;
        case ENCODING_KOI8R: return 20866;
        case ENCODING_UTF8:
        default: return CP_UTF8;
    }
}

Encoding parse_encoding(const char *enc_str) {
    if (!enc_str) return ENCODING_UTF8;
    if (strcmp(enc_str, "win") == 0) return ENCODING_WIN1251;
    if (strcmp(enc_str, "dos") == 0) return ENCODING_DOS866;
    if (strcmp(enc_str, "koi") == 0) return ENCODING_KOI8R;
    if (strcmp(enc_str, "utf") == 0) return ENCODING_UTF8;
    return ENCODING_UTF8;
}

int wtext_to_bytes(const wchar_t *wtext, unsigned char **bytes, size_t *len, Encoding encoding) {
    int codepage = get_codepage(encoding);

    int byte_len = WideCharToMultiByte(codepage, 0, wtext, -1, NULL, 0, NULL, NULL);
    if (byte_len == 0) {
        return 0;
    }

    *bytes = (unsigned char *)malloc(byte_len);
    if (!*bytes) {
        return 0;
    }

    WideCharToMultiByte(codepage, 0, wtext, -1, (char *)*bytes, byte_len, NULL, NULL);
    *len = byte_len - 1;

    return 1;
}

int text_to_bytes(const char *text, unsigned char **bytes, size_t *len, Encoding encoding) {
    int wide_len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (wide_len == 0) return 0;

    wchar_t *wide_str = (wchar_t *)malloc(wide_len * sizeof(wchar_t));
    if (!wide_str) return 0;

    MultiByteToWideChar(CP_ACP, 0, text, -1, wide_str, wide_len);

    int result = wtext_to_bytes(wide_str, bytes, len, encoding);
    free(wide_str);

    return result;
}

int hex_to_bytes(const char *hex_str, unsigned char **bytes, size_t *len) {
    const char *start = hex_str;
    if (strncmp(hex_str, "0x", 2) == 0 || strncmp(hex_str, "0X", 2) == 0) {
        start += 2;
    } else if (hex_str[0] == '$') {
        start += 1;
    }

    size_t hex_len = strlen(start);
    if (hex_len % 2 != 0) {
        fprintf(stderr, "Error: Hex string must have even length\n");
        return 0;
    }

    *len = hex_len / 2;
    *bytes = (unsigned char *)malloc(*len);
    if (!*bytes) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 0;
    }

    for (size_t i = 0; i < *len; i++) {
        char byte_str[3] = {start[i*2], start[i*2+1], '\0'};
        if (!isxdigit(byte_str[0]) || !isxdigit(byte_str[1])) {
            fprintf(stderr, "Error: Invalid hex character\n");
            free(*bytes);
            return 0;
        }
        (*bytes)[i] = (unsigned char)strtol(byte_str, NULL, 16);
    }

    return 1;
}

int is_hex_format(const char *str) {
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) return 1;
    if (str[0] == '$') return 1;
    return 0;
}

int is_quoted_string(const char *str) {
    size_t len = strlen(str);
    return (len >= 2 && str[0] == '"' && str[len-1] == '"');
}

char* extract_quoted_string(const char *str) {
    size_t len = strlen(str);
    if (len < 2) return NULL;

    char *result = (char *)malloc(len - 1);
    if (!result) return NULL;

    memcpy(result, str + 1, len - 2);
    result[len - 2] = '\0';
    return result;
}

int parse_input(const char *input, unsigned char **bytes, size_t *len, Encoding encoding) {
    if (is_hex_format(input)) {
        return hex_to_bytes(input, bytes, len);
    } else if (is_quoted_string(input)) {
        char *text = extract_quoted_string(input);
        if (!text) return 0;
        int result = text_to_bytes(text, bytes, len, encoding);
        free(text);
        return result;
    } else {
        return text_to_bytes(input, bytes, len, encoding);
    }
}

int parse_file_spec(const char *spec, char **input_file, char **output_file,
                    Encoding *input_enc, Encoding *output_enc, int *use_stdio) {
    *input_enc = ENCODING_UTF8;
    *output_enc = ENCODING_UTF8;
    *use_stdio = 0;
    *input_file = NULL;
    *output_file = NULL;

    if (strcmp(spec, "-") == 0) {
        *use_stdio = 1;
        return 1;
    }

    char *spec_copy = strdup(spec);
    if (!spec_copy) return 0;

    char *parts[4] = {NULL, NULL, NULL, NULL};
    int part_count = 0;
    char *token = spec_copy;
    char *next;

    while (token && part_count < 4) {
        next = strchr(token, ':');
        if (next) {
            *next = '\0';
            parts[part_count++] = token;
            token = next + 1;
        } else {
            parts[part_count++] = token;
            break;
        }
    }

    int idx = 0;

    // Check if first part is encoding
    if (parts[0] && (strcmp(parts[0], "win") == 0 || strcmp(parts[0], "dos") == 0 ||
                     strcmp(parts[0], "koi") == 0 || strcmp(parts[0], "utf") == 0)) {
        *input_enc = parse_encoding(parts[0]);
        idx++;
    }

    // Next part must be filename or "-"
    if (!parts[idx]) {
        free(spec_copy);
        return 0;
    }

    if (strcmp(parts[idx], "-") == 0) {
        *use_stdio = 1;
        idx++;

        // Check for output encoding after "-"
        if (parts[idx] && (strcmp(parts[idx], "win") == 0 || strcmp(parts[idx], "dos") == 0 ||
                          strcmp(parts[idx], "koi") == 0 || strcmp(parts[idx], "utf") == 0)) {
            *output_enc = parse_encoding(parts[idx]);
        }

        free(spec_copy);
        return 1;
    }

    *input_file = strdup(parts[idx]);
    idx++;

    // Check next part - could be output encoding, output filename, or "-" for stdout
    if (parts[idx]) {
        if (strcmp(parts[idx], "-") == 0) {
            // Output to stdout
            *use_stdio = 1;
            idx++;

            // Check for output encoding after "-"
            if (parts[idx] && (strcmp(parts[idx], "win") == 0 || strcmp(parts[idx], "dos") == 0 ||
                              strcmp(parts[idx], "koi") == 0 || strcmp(parts[idx], "utf") == 0)) {
                *output_enc = parse_encoding(parts[idx]);
            }
        } else if (strcmp(parts[idx], "win") == 0 || strcmp(parts[idx], "dos") == 0 ||
                   strcmp(parts[idx], "koi") == 0 || strcmp(parts[idx], "utf") == 0) {
            *output_enc = parse_encoding(parts[idx]);
            idx++;

            // Check if there's output filename or "-" after encoding
            if (parts[idx]) {
                if (strcmp(parts[idx], "-") == 0) {
                    *use_stdio = 1;
                } else {
                    *output_file = strdup(parts[idx]);
                }
            }
        } else {
            // It's output filename
            *output_file = strdup(parts[idx]);
        }
    }

    free(spec_copy);
    return 1;
}

char* create_output_filename(const char *input_filename) {
    size_t len = strlen(input_filename);
    const char *dot = strrchr(input_filename, '.');
    const char *slash = strrchr(input_filename, '\\');
    if (!slash) slash = strrchr(input_filename, '/');

    char *output = (char *)malloc(len + 10);
    if (!output) return NULL;

    if (dot && (!slash || dot > slash)) {
        size_t base_len = dot - input_filename;
        strncpy(output, input_filename, base_len);
        output[base_len] = '\0';
        strcat(output, "_OUT");
        strcat(output, dot);
    } else {
        strcpy(output, input_filename);
        strcat(output, "_OUT");
    }

    return output;
}

int apply_encoding_conversion(unsigned char *buffer, size_t buffer_size,
                               Encoding input_enc, Encoding output_enc,
                               unsigned char **result, size_t *result_size) {
    if (input_enc == output_enc) {
        *result = (unsigned char *)malloc(buffer_size);
        if (!*result) return 0;
        memcpy(*result, buffer, buffer_size);
        *result_size = buffer_size;
        return 1;
    }

    int input_cp = get_codepage(input_enc);
    int output_cp = get_codepage(output_enc);

    int wide_len = MultiByteToWideChar(input_cp, 0, (char *)buffer, buffer_size, NULL, 0);
    if (wide_len == 0) return 0;

    wchar_t *wide_str = (wchar_t *)malloc((wide_len + 1) * sizeof(wchar_t));
    if (!wide_str) return 0;

    MultiByteToWideChar(input_cp, 0, (char *)buffer, buffer_size, wide_str, wide_len);
    wide_str[wide_len] = 0;

    int byte_len = WideCharToMultiByte(output_cp, 0, wide_str, wide_len, NULL, 0, NULL, NULL);
    if (byte_len == 0) {
        free(wide_str);
        return 0;
    }

    *result = (unsigned char *)malloc(byte_len);
    if (!*result) {
        free(wide_str);
        return 0;
    }

    WideCharToMultiByte(output_cp, 0, wide_str, wide_len, (char *)*result, byte_len, NULL, NULL);
    *result_size = byte_len;

    free(wide_str);
    return 1;
}

int apply_operations(unsigned char *buffer, size_t buffer_size, Operation *ops, int op_count,
                     unsigned char **result, size_t *result_size, int *total_replacements) {
    unsigned char *current = buffer;
    size_t current_size = buffer_size;
    *total_replacements = 0;

    for (int op_idx = 0; op_idx < op_count; op_idx++) {
        Operation *op = &ops[op_idx];

        unsigned char *temp = (unsigned char *)malloc(current_size * 2 + 1024);
        if (!temp) {
            if (op_idx > 0) free(current);
            return 0;
        }

        size_t temp_pos = 0;
        size_t i = 0;
        int replacements = 0;

        while (i < current_size) {
            if (i + op->search_len <= current_size &&
                memcmp(current + i, op->search_bytes, op->search_len) == 0) {
                if (!op->delete_mode) {
                    memcpy(temp + temp_pos, op->replace_bytes, op->replace_len);
                    temp_pos += op->replace_len;
                }
                i += op->search_len;
                replacements++;
            } else {
                temp[temp_pos++] = current[i++];
            }
        }

        if (op_idx > 0) free(current);
        current = temp;
        current_size = temp_pos;
        *total_replacements += replacements;
    }

    *result = current;
    *result_size = current_size;
    return 1;
}

int parse_operation(const char *arg, Operation *op, Encoding encoding) {
    char *colon = strchr(arg, ':');
    if (!colon) {
        fprintf(stderr, "Error: Invalid operation format. Use search:replace or search:replace:encoding\n");
        return 0;
    }

    size_t search_len = colon - arg;
    char *search_str = (char *)malloc(search_len + 1);
    if (!search_str) return 0;
    strncpy(search_str, arg, search_len);
    search_str[search_len] = '\0';

    char *replace_start = colon + 1;
    char *second_colon = strchr(replace_start, ':');

    char *replace_str = NULL;
    char *encoding_str = NULL;

    if (second_colon) {
        size_t replace_len = second_colon - replace_start;
        replace_str = (char *)malloc(replace_len + 1);
        if (!replace_str) {
            free(search_str);
            return 0;
        }
        strncpy(replace_str, replace_start, replace_len);
        replace_str[replace_len] = '\0';
        encoding_str = second_colon + 1;
        encoding = parse_encoding(encoding_str);
    } else {
        replace_str = strdup(replace_start);
    }

    if (!parse_input(search_str, &op->search_bytes, &op->search_len, encoding)) {
        free(search_str);
        free(replace_str);
        return 0;
    }

    if (strlen(replace_str) == 0) {
        op->delete_mode = 1;
        op->replace_bytes = NULL;
        op->replace_len = 0;
    } else {
        op->delete_mode = 0;
        if (!parse_input(replace_str, &op->replace_bytes, &op->replace_len, encoding)) {
            free(op->search_bytes);
            free(search_str);
            free(replace_str);
            return 0;
        }
    }

    free(search_str);
    free(replace_str);
    return 1;
}

void free_operation(Operation *op) {
    if (op->search_bytes) free(op->search_bytes);
    if (op->replace_bytes) free(op->replace_bytes);
}

int main(int argc, char *argv[]) {
    // Enable Windows virtual terminal processing for ANSI colors
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    if (argc < 2) {
        fprintf(stderr, "\n" COLOR_YELLOW "REPLACER" COLOR_RESET "        - " COLOR_CYAN "File content search and replace utility with encoding conversion" COLOR_RESET "\n");
        fprintf(stderr, "                  Утилита поиска и замены содержимого файлов с конвертацией кодировок\n");
        fprintf(stderr, "v26.0417 (by BoyNG - Vyacheslav Burnosov)\n\n\n");
        fprintf(stderr, COLOR_YELLOW "Usage / Использование:" COLOR_RESET " %s " COLOR_GREEN "[encoding:]<input>[:[encoding][:output]]" COLOR_RESET " [operations...]\n\n", argv[0]);

        fprintf(stderr, COLOR_YELLOW "File specification / Формат файла:" COLOR_RESET "\n");
        fprintf(stderr, "  " COLOR_GREEN "file.bin" COLOR_RESET "                    - " COLOR_CYAN "input file, output will be file_OUT.bin" COLOR_RESET "\n");
        fprintf(stderr, "                                входной файл, выход будет file_OUT.bin\n");
        fprintf(stderr, "  " COLOR_GREEN "file.bin:out.bin" COLOR_RESET "            - " COLOR_CYAN "input file with custom output name" COLOR_RESET "\n");
        fprintf(stderr, "                                входной файл с указанием имени выходного\n");
        fprintf(stderr, "  " COLOR_GREEN "file.bin:-" COLOR_RESET "                  - " COLOR_CYAN "output to stdout (like 'type' command)" COLOR_RESET "\n");
        fprintf(stderr, "                                вывод в stdout (как команда 'type')\n");
        fprintf(stderr, "  " COLOR_GREEN "file.txt:utf" COLOR_RESET "                - " COLOR_CYAN "convert file to UTF-8 encoding" COLOR_RESET "\n");
        fprintf(stderr, "                                конвертировать файл в кодировку UTF-8\n");
        fprintf(stderr, "  " COLOR_GREEN "file.txt:-:utf" COLOR_RESET "              - " COLOR_CYAN "output to stdout with UTF-8 conversion" COLOR_RESET "\n");
        fprintf(stderr, "                                вывод в stdout с конвертацией в UTF-8\n");
        fprintf(stderr, "  " COLOR_GREEN "win:file.txt" COLOR_RESET "                - " COLOR_CYAN "read file as Windows-1251" COLOR_RESET "\n");
        fprintf(stderr, "                                читать файл как Windows-1251\n");
        fprintf(stderr, "  " COLOR_GREEN "win:file.txt:utf" COLOR_RESET "            - " COLOR_CYAN "convert Windows-1251 to UTF-8" COLOR_RESET "\n");
        fprintf(stderr, "                                конвертировать Windows-1251 в UTF-8\n");
        fprintf(stderr, "  " COLOR_GREEN "win:file.txt:-" COLOR_RESET "              - " COLOR_CYAN "output Windows-1251 file to stdout" COLOR_RESET "\n");
        fprintf(stderr, "                                вывод файла Windows-1251 в stdout\n");
        fprintf(stderr, "  " COLOR_GREEN "win:file.txt:-:utf" COLOR_RESET "          - " COLOR_CYAN "convert and output to stdout" COLOR_RESET "\n");
        fprintf(stderr, "                                конвертация и вывод в stdout\n");
        fprintf(stderr, "  " COLOR_GREEN "win:file.txt:utf:out.txt" COLOR_RESET "    - " COLOR_CYAN "convert with custom output name" COLOR_RESET "\n");
        fprintf(stderr, "                                конвертация с указанием имени выхода\n");
        fprintf(stderr, "  " COLOR_GREEN "-" COLOR_RESET "                           - " COLOR_CYAN "read from stdin, write to stdout" COLOR_RESET "\n");
        fprintf(stderr, "                                читать из stdin, писать в stdout\n");
        fprintf(stderr, "  " COLOR_GREEN "win:-:utf" COLOR_RESET "                   - " COLOR_CYAN "stdin/stdout with encoding conversion" COLOR_RESET "\n");
        fprintf(stderr, "                                stdin/stdout с конвертацией кодировки\n");
        fprintf(stderr, "\n");
        getchar();
        fprintf(stderr, "\n" COLOR_YELLOW "Operation format / Формат операций:" COLOR_RESET "\n");
        fprintf(stderr, "  " COLOR_GREEN "search:replace" COLOR_RESET "              - " COLOR_CYAN "search and replace" COLOR_RESET " / найти и заменить\n");
        fprintf(stderr, "  " COLOR_GREEN "search:" COLOR_RESET "                     - " COLOR_CYAN "delete (empty replace)" COLOR_RESET " / удалить (пустая замена)\n");
        fprintf(stderr, "  " COLOR_GREEN "search:replace:encoding" COLOR_RESET "     - " COLOR_CYAN "with specific encoding for this operation" COLOR_RESET "\n");
        fprintf(stderr, "                                с указанием кодировки для операции\n");

        fprintf(stderr, "\n" COLOR_YELLOW "Search/Replace formats / Форматы поиска/замены:" COLOR_RESET "\n");
        fprintf(stderr, "  Hex: " COLOR_GREEN "0xFFAA" COLOR_RESET " or " COLOR_GREEN "$FFAA" COLOR_RESET " / или " COLOR_GREEN "$FFAA" COLOR_RESET "\n");
        fprintf(stderr, "  Text: " COLOR_GREEN "\"hello\"" COLOR_RESET " or plain text / Текст: " COLOR_GREEN "\"привет\"" COLOR_RESET " или просто текст\n");

        fprintf(stderr, "\n" COLOR_YELLOW "Encodings / Кодировки:" COLOR_RESET " " COLOR_GREEN "win" COLOR_RESET " (CP1251), " COLOR_GREEN "dos" COLOR_RESET " (CP866), " COLOR_GREEN "koi" COLOR_RESET " (KOI8-R), " COLOR_GREEN "utf" COLOR_RESET " (UTF-8, default/по умолчанию)\n");

        fprintf(stderr, "\n" COLOR_YELLOW "Examples / Примеры:" COLOR_RESET "\n");
        fprintf(stderr, "  %s file.bin 0xAA:0xBB 0xCC:0xDD\n", argv[0]);
        fprintf(stderr, "  %s file.bin:out.bin 0xAA:0xBB \"old\":\"new\":win\n", argv[0]);
        fprintf(stderr, "  %s file.txt:-\n", argv[0]);
        fprintf(stderr, "  %s file.txt:- 0xAA:0xBB\n", argv[0]);
        fprintf(stderr, "  %s win:file.txt:-:utf\n", argv[0]);
        fprintf(stderr, "  %s win:file.txt:utf\n", argv[0]);
        fprintf(stderr, "  %s win:file.txt:utf \"тест\":\"test\"\n", argv[0]);
        fprintf(stderr, "  %s file.txt \"тест\":\"test\":win \"hello\":\n", argv[0]);
        fprintf(stderr, "  %s - 0xAA:0xBB < in.bin > out.bin\n", argv[0]);
        fprintf(stderr, "  type in.bin | %s - 0xAA:0xBB > out.bin\n", argv[0]);
        fprintf(stderr, "  type file.txt | %s - \"old\":\"new\"\n", argv[0]);
        fprintf(stderr, "  %s - \"old\":\"new\" < file.txt\n", argv[0]);
        fprintf(stderr, "  %s - \"old\":\"new\" < file.txt 2>nul\n", argv[0]);
        fprintf(stderr, "  %s win:-:utf < input.txt > output.txt\n", argv[0]);
        getchar();
        return 1;
    }

    const char *file_spec = argv[1];

    char *input_file = NULL;
    char *output_file = NULL;
    Encoding input_enc = ENCODING_UTF8;
    Encoding output_enc = ENCODING_UTF8;
    int use_stdio = 0;

    if (!parse_file_spec(file_spec, &input_file, &output_file, &input_enc, &output_enc, &use_stdio)) {
        fprintf(stderr, "Error: Invalid file specification\n");
        return 1;
    }

    if (!use_stdio && !output_file) {
        output_file = create_output_filename(input_file);
        if (!output_file) {
            fprintf(stderr, "Error: Failed to create output filename\n");
            free(input_file);
            return 1;
        }
    }

    int op_count = argc - 2;
    Operation *operations = NULL;

    if (op_count > 0) {
        operations = (Operation *)calloc(op_count, sizeof(Operation));
        if (!operations) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }

        for (int i = 0; i < op_count; i++) {
            if (!parse_operation(argv[i + 2], &operations[i], ENCODING_UTF8)) {
                for (int j = 0; j < i; j++) {
                    free_operation(&operations[j]);
                }
                free(operations);
                if (input_file) free(input_file);
                if (output_file) free(output_file);
                return 1;
            }
        }
    }

    unsigned char *buffer = NULL;
    size_t buffer_size = 0;

    if (use_stdio && !input_file) {
        // Read from stdin
        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);

        size_t capacity = 4096;
        buffer = (unsigned char *)malloc(capacity);
        if (!buffer) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            return 1;
        }

        size_t bytes_read;
        while ((bytes_read = fread(buffer + buffer_size, 1, 4096, stdin)) > 0) {
            buffer_size += bytes_read;
            if (buffer_size + 4096 > capacity) {
                capacity *= 2;
                unsigned char *new_buffer = (unsigned char *)realloc(buffer, capacity);
                if (!new_buffer) {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    free(buffer);
                    if (operations) {
                        for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                        free(operations);
                    }
                    return 1;
                }
                buffer = new_buffer;
            }
        }
    } else {
        // Read from file
        FILE *fin = fopen(input_file, "rb");
        if (!fin) {
            fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            free(input_file);
            if (output_file) free(output_file);
            return 1;
        }

        fseek(fin, 0, SEEK_END);
        buffer_size = ftell(fin);
        fseek(fin, 0, SEEK_SET);

        buffer = (unsigned char *)malloc(buffer_size);
        if (!buffer) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(fin);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            free(input_file);
            if (output_file) free(output_file);
            return 1;
        }

        size_t bytes_read = fread(buffer, 1, buffer_size, fin);
        fclose(fin);

        if (bytes_read != buffer_size) {
            fprintf(stderr, "Error: Failed to read file\n");
            free(buffer);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            free(input_file);
            if (output_file) free(output_file);
            return 1;
        }

        // Set stdout to binary mode if outputting to stdout
        if (use_stdio) {
            _setmode(_fileno(stdout), _O_BINARY);
        }
    }

    unsigned char *processed = NULL;
    size_t processed_size = 0;

    // Apply input encoding conversion if needed
    if (input_enc != ENCODING_UTF8) {
        if (!apply_encoding_conversion(buffer, buffer_size, input_enc, ENCODING_UTF8, &processed, &processed_size)) {
            fprintf(stderr, "Error: Failed to convert input encoding\n");
            free(buffer);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }
        free(buffer);
        buffer = processed;
        buffer_size = processed_size;
    }

    // Apply operations if any
    unsigned char *result = NULL;
    size_t result_size = 0;
    int total_replacements = 0;

    if (op_count > 0) {
        if (!apply_operations(buffer, buffer_size, operations, op_count, &result, &result_size, &total_replacements)) {
            fprintf(stderr, "Error: Failed to apply operations\n");
            free(buffer);
            for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
            free(operations);
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }
        free(buffer);
    } else {
        result = buffer;
        result_size = buffer_size;
    }

    // Apply output encoding conversion if needed
    unsigned char *final_result = NULL;
    size_t final_size = 0;

    if (output_enc != ENCODING_UTF8) {
        if (!apply_encoding_conversion(result, result_size, ENCODING_UTF8, output_enc, &final_result, &final_size)) {
            fprintf(stderr, "Error: Failed to convert output encoding\n");
            free(result);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            if (input_file) free(input_file);
            if (output_file) free(output_file);
            return 1;
        }
        free(result);
        result = final_result;
        result_size = final_size;
    }

    if (use_stdio) {
        fwrite(result, 1, result_size, stdout);
    } else {
        FILE *fout = fopen(output_file, "wb");
        if (!fout) {
            fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
            free(result);
            if (operations) {
                for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
                free(operations);
            }
            free(input_file);
            free(output_file);
            return 1;
        }

        fwrite(result, 1, result_size, fout);
        fclose(fout);

        if (op_count > 0) {
            fprintf(stderr, "Total replacements: %d\n", total_replacements);
        }
        fprintf(stderr, "Output file: %s\n", output_file);
    }

    free(result);
    if (operations) {
        for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
        free(operations);
    }
    if (input_file) free(input_file);
    if (output_file) free(output_file);

    return 0;
}
