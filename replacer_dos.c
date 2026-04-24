#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define COLOR_RESET "\033[0m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_RED "\033[1;31m"

// Capture groups constants
#define MAX_GROUP_NAME_LEN 32
#define MAX_CAPTURE_GROUPS 99

typedef enum {
  ENCODING_UTF8,
  ENCODING_WIN1251,
  ENCODING_DOS866,
  ENCODING_KOI8R
} Encoding;

typedef enum {
  PATTERN_LITERAL,  // Regular byte sequence
  PATTERN_WILDCARD  // Contains wildcards
} PatternType;

typedef enum {
  WILDCARD_ANY_BYTE,      // . - any single byte
  WILDCARD_ZERO_OR_MORE,  // * - zero or more bytes
  WILDCARD_OPTIONAL       // ? - zero or one byte
} WildcardType;

// Named group mapping
typedef struct {
  char name[MAX_GROUP_NAME_LEN + 1];  // Group name
  int index;                           // Index in captures array
} NamedGroup;

typedef struct {
  int is_wildcard;
  WildcardType wildcard_type;
  unsigned char* bytes;
  size_t len;
  int is_capture_group;                    // NEW: 1 if this segment is in {...}
  int capture_group_index;                 // NEW: Index in captures[] (-1 if not a group)
  char group_name[MAX_GROUP_NAME_LEN + 1]; // NEW: Group name (empty if numbered)
} PatternSegment;

// Capture structures
typedef struct {
  unsigned char* data;
  size_t len;
} CapturedSegment;

typedef struct {
  CapturedSegment entire_match;                 // \0
  CapturedSegment captures[MAX_CAPTURE_GROUPS]; // NEW: All captures (numbered + named)
  int capture_count;                            // NEW: Total number of captures
  NamedGroup named_groups[MAX_CAPTURE_GROUPS];  // NEW: Name to index mapping
  int named_group_count;                        // NEW: Number of named groups
} CaptureContext;

typedef struct {
  unsigned char* search_bytes;
  size_t search_len;
  unsigned char* replace_bytes;
  size_t replace_len;
  int delete_mode;

  // Fields for wildcard support
  PatternType pattern_type;
  PatternSegment* segments;
  int segment_count;

  // Fields for capture support
  int has_captures_in_replace;  // 1 if replace string contains \0, \1, {name}, etc.
  char* replace_template;       // Original replace string with \0, \1, {name}...

  // NEW: Named groups defined in search pattern
  NamedGroup defined_groups[MAX_CAPTURE_GROUPS];
  int defined_group_count;

  // NEW: Case-insensitive flag
  int ignore_case;  // 1 if /i flag is present
} Operation;

// ============================================================================
// Bilingual error message helpers
// ============================================================================

void print_error(const char* msg_en, const char* msg_ru) {
  fprintf(stderr, COLOR_RED "Error: " COLOR_RESET "%s\n", msg_en);
  fprintf(stderr, COLOR_RED "Ошибка: " COLOR_RESET "%s\n", msg_ru);
}

void print_error_pos(const char* msg_en, const char* msg_ru, int pos) {
  fprintf(stderr, COLOR_RED "Error: " COLOR_RESET "%s %d\n", msg_en, pos);
  fprintf(stderr, COLOR_RED "Ошибка: " COLOR_RESET "%s %d\n", msg_ru, pos);
}

void print_error_str(const char* msg_en, const char* msg_ru, const char* str) {
  fprintf(stderr, COLOR_RED "Error: " COLOR_RESET "%s '%s'\n", msg_en, str);
  fprintf(stderr, COLOR_RED "Ошибка: " COLOR_RESET "%s '%s'\n", msg_ru, str);
}

void print_error_char(const char* msg_en, const char* msg_ru, char c, int pos) {
  fprintf(stderr, COLOR_RED "Error: " COLOR_RESET "%s '%c' at position %d\n", msg_en, c, pos);
  fprintf(stderr, COLOR_RED "Ошибка: " COLOR_RESET "%s '%c' в позиции %d\n", msg_ru, c, pos);
}

// Validate group name: [a-zA-Z][a-zA-Z0-9_]*, max 32 chars
int validate_group_name(const char* name, int len, int pos) {
  if (len == 0) {
    print_error_pos(
      "Empty capture group name '{:...}' at position",
      "Пустое имя группы захвата '{:...}' в позиции",
      pos
    );
    return 0;
  }

  if (len > MAX_GROUP_NAME_LEN) {
    print_error_pos(
      "Group name too long (max 32 characters) at position",
      "Слишком длинное имя группы (максимум 32 символа) в позиции",
      pos
    );
    return 0;
  }

  if (!isalpha((unsigned char)name[0])) {
    print_error_pos(
      "Group name must start with a letter at position",
      "Имя группы должно начинаться с буквы в позиции",
      pos
    );
    return 0;
  }

  for (int i = 0; i < len; i++) {
    char c = name[i];
    if (!isalnum((unsigned char)c) && c != '_') {
      print_error_char(
        "Invalid character in group name (use only: a-z, A-Z, 0-9, _)",
        "Недопустимый символ в имени группы (используйте только: a-z, A-Z, 0-9, _)",
        c, pos + i
      );
      return 0;
    }
  }

  return 1;
}

// ============================================================================

// Convert bytes to lowercase (for case-insensitive matching)
void to_lowercase(unsigned char* dest, const unsigned char* src, size_t len) {
  for (size_t i = 0; i < len; i++) {
    dest[i] = tolower(src[i]);
  }
}

int get_codepage(Encoding enc) {
  switch (enc) {
    case ENCODING_WIN1251:
      return 1251;
    case ENCODING_DOS866:
      return 866;
    case ENCODING_KOI8R:
      return 20866;
    case ENCODING_UTF8:
    default:
      return CP_UTF8;
  }
}

Encoding parse_encoding(const char* enc_str) {
  if (!enc_str) return ENCODING_UTF8;
  if (strcmp(enc_str, "win") == 0) return ENCODING_WIN1251;
  if (strcmp(enc_str, "dos") == 0) return ENCODING_DOS866;
  if (strcmp(enc_str, "koi") == 0) return ENCODING_KOI8R;
  if (strcmp(enc_str, "utf") == 0) return ENCODING_UTF8;
  return ENCODING_UTF8;
}

int wtext_to_bytes(const wchar_t* wtext, unsigned char** bytes, size_t* len,
                   Encoding encoding) {
  int codepage = get_codepage(encoding);

  int byte_len =
      WideCharToMultiByte(codepage, 0, wtext, -1, NULL, 0, NULL, NULL);
  if (byte_len == 0) {
    return 0;
  }

  *bytes = (unsigned char*)malloc(byte_len);
  if (!*bytes) {
    return 0;
  }

  WideCharToMultiByte(codepage, 0, wtext, -1, (char*)*bytes, byte_len, NULL,
                      NULL);
  *len = byte_len - 1;

  return 1;
}

// Process escape sequences in text string
// Returns processed string (caller must free) or NULL on error
char* process_escape_sequences(const char* text, int* has_wildcards) {
  size_t len = strlen(text);
  char* result = (char*)malloc(len + 1);
  if (!result) return NULL;

  *has_wildcards = 0;
  size_t out_pos = 0;
  size_t i = 0;

  while (i < len) {
    if (text[i] == '\\' && i + 1 < len) {
      char next = text[i + 1];

      // Check for wildcards
      if (next == '.' || next == '*' || next == '?') {
        *has_wildcards = 1;
        result[out_pos++] = '\\';
        result[out_pos++] = next;
        i += 2;
        continue;
      }

      // Standard escape sequences
      switch (next) {
        case 'n':
          result[out_pos++] = '\n';
          i += 2;
          break;
        case 'r':
          result[out_pos++] = '\r';
          i += 2;
          break;
        case 't':
          result[out_pos++] = '\t';
          i += 2;
          break;
        case '\\':
          result[out_pos++] = '\\';
          i += 2;
          break;
        case '"':
          result[out_pos++] = '"';
          i += 2;
          break;
        case '\'':
          result[out_pos++] = '\'';
          i += 2;
          break;
        case 'x':
          // Hex escape: \xHH
          if (i + 3 < len && isxdigit(text[i + 2]) && isxdigit(text[i + 3])) {
            char hex_str[3] = {text[i + 2], text[i + 3], '\0'};
            result[out_pos++] = (char)strtol(hex_str, NULL, 16);
            i += 4;
          } else {
            // Invalid hex escape - keep as is
            result[out_pos++] = text[i++];
          }
          break;
        default:
          // Unknown escape - keep backslash and character
          result[out_pos++] = text[i++];
          break;
      }
    } else {
      result[out_pos++] = text[i++];
    }
  }

  result[out_pos] = '\0';
  return result;
}

int text_to_bytes(const char* text, unsigned char** bytes, size_t* len,
                  Encoding encoding) {
  int wide_len = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
  if (wide_len == 0) return 0;

  wchar_t* wide_str = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
  if (!wide_str) return 0;

  MultiByteToWideChar(CP_ACP, 0, text, -1, wide_str, wide_len);

  int result = wtext_to_bytes(wide_str, bytes, len, encoding);
  free(wide_str);

  return result;
}

int hex_to_bytes(const char* hex_str, unsigned char** bytes, size_t* len) {
  const char* start = hex_str;
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
  *bytes = (unsigned char*)malloc(*len);
  if (!*bytes) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    return 0;
  }

  for (size_t i = 0; i < *len; i++) {
    char byte_str[3] = {start[i * 2], start[i * 2 + 1], '\0'};
    if (!isxdigit(byte_str[0]) || !isxdigit(byte_str[1])) {
      fprintf(stderr, "Error: Invalid hex character\n");
      free(*bytes);
      return 0;
    }
    (*bytes)[i] = (unsigned char)strtol(byte_str, NULL, 16);
  }

  return 1;
}

int is_hex_format(const char* str) {
  if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) return 1;
  if (str[0] == '$') return 1;
  return 0;
}

int is_quoted_string(const char* str) {
  size_t len = strlen(str);
  // Support both single and double quotes
  return (len >= 2 &&
          ((str[0] == '"' && str[len - 1] == '"') ||
           (str[0] == '\'' && str[len - 1] == '\'')));
}

char* extract_quoted_string(const char* str) {
  size_t len = strlen(str);
  if (len < 2) return NULL;

  char* result = (char*)malloc(len - 1);
  if (!result) return NULL;

  memcpy(result, str + 1, len - 2);
  result[len - 2] = '\0';
  return result;
}

// Check if replacement string contains capture references (\0, \1, \2, etc.)
// Check if replacement string contains capture references (\0-\9, {name})
int has_capture_references(const char* str) {
  int in_quotes = 0;
  for (const char* p = str; *p; p++) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
      continue;
    }

    // \0-\9 work everywhere
    if (*p == '\\' && p[1] >= '0' && p[1] <= '9') {
      return 1;
    }

    // {name} only outside quotes
    if (!in_quotes && *p == '{' && p[1] != '\0') {
      return 1;
    }
  }
  return 0;
}

int parse_input(const char* input, unsigned char** bytes, size_t* len,
                Encoding encoding) {
  if (is_hex_format(input)) {
    return hex_to_bytes(input, bytes, len);
  } else if (is_quoted_string(input)) {
    char* text = extract_quoted_string(input);
    if (!text) return 0;

    // Check if text contains wildcards
    int has_wildcards = 0;
    for (const char* p = text; *p; p++) {
      if (*p == '\\' && (p[1] == '.' || p[1] == '*' || p[1] == '?')) {
        has_wildcards = 1;
        break;
      }
    }

    if (has_wildcards) {
      // Text contains wildcards - cannot convert to bytes directly
      // Caller should use parse_concatenated_input instead
      free(text);
      return 0;
    }

    // Process escape sequences (no wildcards)
    int dummy = 0;
    char* processed = process_escape_sequences(text, &dummy);
    free(text);
    if (!processed) return 0;

    int result = text_to_bytes(processed, bytes, len, encoding);
    free(processed);
    return result;
  } else {
    // Check if unquoted text contains wildcards
    int has_wildcards = 0;
    for (const char* p = input; *p; p++) {
      if (*p == '\\' && (p[1] == '.' || p[1] == '*' || p[1] == '?')) {
        has_wildcards = 1;
        break;
      }
    }

    if (has_wildcards) {
      // Text contains wildcards - cannot convert to bytes directly
      return 0;
    }

    // Process escape sequences for unquoted text
    int dummy = 0;
    char* processed = process_escape_sequences(input, &dummy);
    if (!processed) return 0;

    int result = text_to_bytes(processed, bytes, len, encoding);
    free(processed);
    return result;
  }
}

// Parse text with embedded wildcards: "test\.end" -> segments
// Returns array of segments and count
int parse_text_with_wildcards(const char* text, PatternSegment** segments,
                               int* segment_count, Encoding encoding) {
  *segments = NULL;
  *segment_count = 0;

  // Count segments (split by wildcards)
  int count = 1;
  for (const char* p = text; *p; p++) {
    // Check for both escaped and unescaped wildcards
    if ((*p == '.' || *p == '*' || *p == '?') ||
        (*p == '\\' && (p[1] == '.' || p[1] == '*' || p[1] == '?'))) {
      count += 2;  // literal part + wildcard
      if (*p == '\\') p++;  // skip next char if escaped
    }
  }

  *segments = (PatternSegment*)calloc(count, sizeof(PatternSegment));
  if (!*segments) return 0;

  int seg_idx = 0;
  const char* literal_start = text;
  size_t literal_len = 0;

  for (const char* p = text; *p; p++) {
    // Check for both escaped (\*) and unescaped (*) wildcards
    int is_wildcard = 0;
    char wildcard_char = 0;

    if (*p == '\\' && (p[1] == '.' || p[1] == '*' || p[1] == '?')) {
      is_wildcard = 1;
      wildcard_char = p[1];
    } else if (*p == '.' || *p == '*' || *p == '?') {
      is_wildcard = 1;
      wildcard_char = *p;
    }

    if (is_wildcard) {
      // Found wildcard - save literal part before it
      if (literal_len > 0) {
        char* literal = (char*)malloc(literal_len + 1);
        if (!literal) {
          for (int i = 0; i < seg_idx; i++) {
            if ((*segments)[i].bytes) free((*segments)[i].bytes);
          }
          free(*segments);
          return 0;
        }
        strncpy(literal, literal_start, literal_len);
        literal[literal_len] = '\0';

        // Process escape sequences in literal part
        int dummy = 0;
        char* processed = process_escape_sequences(literal, &dummy);
        free(literal);
        if (!processed) {
          for (int i = 0; i < seg_idx; i++) {
            if ((*segments)[i].bytes) free((*segments)[i].bytes);
          }
          free(*segments);
          return 0;
        }

        // Convert to bytes
        (*segments)[seg_idx].is_wildcard = 0;
        if (!text_to_bytes(processed, &(*segments)[seg_idx].bytes,
                          &(*segments)[seg_idx].len, encoding)) {
          free(processed);
          for (int i = 0; i < seg_idx; i++) {
            if ((*segments)[i].bytes) free((*segments)[i].bytes);
          }
          free(*segments);
          return 0;
        }
        free(processed);
        seg_idx++;
      }

      // Add wildcard segment
      (*segments)[seg_idx].is_wildcard = 1;
      if (wildcard_char == '.') {
        (*segments)[seg_idx].wildcard_type = WILDCARD_ANY_BYTE;
      } else if (wildcard_char == '*') {
        (*segments)[seg_idx].wildcard_type = WILDCARD_ZERO_OR_MORE;
      } else if (wildcard_char == '?') {
        (*segments)[seg_idx].wildcard_type = WILDCARD_OPTIONAL;
      }
      (*segments)[seg_idx].bytes = NULL;
      (*segments)[seg_idx].len = 0;
      seg_idx++;

      // Skip the wildcard character (and backslash if escaped)
      if (*p == '\\') p++;  // skip backslash
      // p will be incremented by loop
      literal_start = p + 1;
      literal_len = 0;
    } else {
      literal_len++;
    }
  }

  // Add remaining literal part
  if (literal_len > 0) {
    char* literal = (char*)malloc(literal_len + 1);
    if (!literal) {
      for (int i = 0; i < seg_idx; i++) {
        if ((*segments)[i].bytes) free((*segments)[i].bytes);
      }
      free(*segments);
      return 0;
    }
    strncpy(literal, literal_start, literal_len);
    literal[literal_len] = '\0';

    // Process escape sequences
    int dummy = 0;
    char* processed = process_escape_sequences(literal, &dummy);
    free(literal);
    if (!processed) {
      for (int i = 0; i < seg_idx; i++) {
        if ((*segments)[i].bytes) free((*segments)[i].bytes);
      }
      free(*segments);
      return 0;
    }

    // Convert to bytes
    (*segments)[seg_idx].is_wildcard = 0;
    if (!text_to_bytes(processed, &(*segments)[seg_idx].bytes,
                      &(*segments)[seg_idx].len, encoding)) {
      free(processed);
      for (int i = 0; i < seg_idx; i++) {
        if ((*segments)[i].bytes) free((*segments)[i].bytes);
      }
      free(*segments);
      return 0;
    }
    free(processed);
    seg_idx++;
  }

  *segment_count = seg_idx;
  return 1;
}

// Parse input with + concatenation: "text"+0x0D0A+"more"
// Returns array of segments and count
int parse_concatenated_input(const char* input, PatternSegment** segments,
                             int* segment_count, Encoding encoding) {
  *segments = NULL;
  *segment_count = 0;

  // First pass: count total segments (including wildcards inside text)
  int total_count = 0;
  int in_quotes = 0;
  const char* seg_start = input;

  for (const char* p = input; ; p++) {
    if (*p == '"' || *p == '\'') in_quotes = !in_quotes;

    if ((*p == '+' && !in_quotes) || *p == '\0') {
      // Extract segment to count wildcards
      size_t seg_len = p - seg_start;
      char* segment = (char*)malloc(seg_len + 1);
      if (!segment) return 0;
      strncpy(segment, seg_start, seg_len);
      segment[seg_len] = '\0';

      // Check if it's standalone wildcard: \? \. \* or ? . *
      int is_standalone = 0;
      if (seg_len == 2 && segment[0] == '\\' &&
          (segment[1] == '?' || segment[1] == '.' || segment[1] == '*')) {
        is_standalone = 1;
      } else if (seg_len == 1 &&
                 (segment[0] == '?' || segment[0] == '.' || segment[0] == '*')) {
        is_standalone = 1;
      }

      if (is_standalone) {
        total_count++;
      } else {
        // Count wildcards inside text (both escaped and unescaped)
        // But NOT inside quoted strings
        int wildcard_count = 0;

        if (!is_quoted_string(segment)) {
          // Only count wildcards if not a quoted string
          for (const char* wp = segment; *wp; wp++) {
            if ((*wp == '.' || *wp == '*' || *wp == '?') ||
                (*wp == '\\' && (wp[1] == '.' || wp[1] == '*' || wp[1] == '?'))) {
              wildcard_count++;
            }
          }
        }
        // Each wildcard splits text: "a\.b\.c" = 5 segments (a, \., b, \., c)
        total_count += (wildcard_count > 0) ? (wildcard_count * 2 + 1) : 1;
      }

      free(segment);

      if (*p == '\0') break;
      seg_start = p + 1;
    }
  }

  *segments = (PatternSegment*)calloc(total_count, sizeof(PatternSegment));
  if (!*segments) return 0;

  // Second pass: parse segments
  char* input_copy = strdup(input);
  if (!input_copy) {
    free(*segments);
    return 0;
  }

  char* p = input_copy;
  int seg_idx = 0;
  in_quotes = 0;
  seg_start = p;

  while (*p) {
    if (*p == '"' || *p == '\'') in_quotes = !in_quotes;

    if ((*p == '+' && !in_quotes) || *(p + 1) == '\0') {
      // Extract segment
      char* seg_end = (*p == '+') ? p : p + 1;
      size_t seg_len = seg_end - seg_start;
      char* segment = (char*)malloc(seg_len + 1);
      if (!segment) {
        free(input_copy);
        for (int i = 0; i < seg_idx; i++) {
          if ((*segments)[i].bytes) free((*segments)[i].bytes);
        }
        free(*segments);
        return 0;
      }
      strncpy(segment, seg_start, seg_len);
      segment[seg_len] = '\0';

      // Check if it's a standalone wildcard: \? \. \* or ? . *
      int is_standalone_wildcard = 0;
      WildcardType wc_type;

      if (seg_len == 2 && segment[0] == '\\' &&
          (segment[1] == '?' || segment[1] == '.' || segment[1] == '*')) {
        is_standalone_wildcard = 1;
        if (segment[1] == '.') wc_type = WILDCARD_ANY_BYTE;
        else if (segment[1] == '*') wc_type = WILDCARD_ZERO_OR_MORE;
        else if (segment[1] == '?') wc_type = WILDCARD_OPTIONAL;
      } else if (seg_len == 1 &&
                 (segment[0] == '?' || segment[0] == '.' || segment[0] == '*')) {
        is_standalone_wildcard = 1;
        if (segment[0] == '.') wc_type = WILDCARD_ANY_BYTE;
        else if (segment[0] == '*') wc_type = WILDCARD_ZERO_OR_MORE;
        else if (segment[0] == '?') wc_type = WILDCARD_OPTIONAL;
      }

      if (is_standalone_wildcard) {
        (*segments)[seg_idx].is_wildcard = 1;
        (*segments)[seg_idx].wildcard_type = wc_type;
        (*segments)[seg_idx].bytes = NULL;
        (*segments)[seg_idx].len = 0;
        seg_idx++;
      } else {
        // Check if segment contains wildcards (both escaped and unescaped)
        // But NOT inside quoted strings
        int has_wildcards = 0;

        if (!is_quoted_string(segment)) {
          // Only check for wildcards if not a quoted string
          for (const char* wp = segment; *wp; wp++) {
            if ((*wp == '.' || *wp == '*' || *wp == '?') ||
                (*wp == '\\' && (wp[1] == '.' || wp[1] == '*' || wp[1] == '?'))) {
              has_wildcards = 1;
              break;
            }
          }
        }

        if (has_wildcards) {
          // Parse text with wildcards
          char* text_to_parse = segment;
          if (is_quoted_string(segment)) {
            text_to_parse = extract_quoted_string(segment);
            if (!text_to_parse) {
              free(segment);
              free(input_copy);
              for (int i = 0; i < seg_idx; i++) {
                if ((*segments)[i].bytes) free((*segments)[i].bytes);
              }
              free(*segments);
              return 0;
            }
          }

          PatternSegment* sub_segments = NULL;
          int sub_count = 0;
          if (!parse_text_with_wildcards(text_to_parse, &sub_segments, &sub_count,
                                         encoding)) {
            if (text_to_parse != segment) free(text_to_parse);
            free(segment);
            free(input_copy);
            for (int i = 0; i < seg_idx; i++) {
              if ((*segments)[i].bytes) free((*segments)[i].bytes);
            }
            free(*segments);
            return 0;
          }

          if (text_to_parse != segment) free(text_to_parse);

          // Copy all sub_segments
          for (int i = 0; i < sub_count; i++) {
            (*segments)[seg_idx++] = sub_segments[i];
          }
          free(sub_segments);
        } else {
          // Parse as hex or text (no wildcards)
          (*segments)[seg_idx].is_wildcard = 0;
          if (!parse_input(segment, &(*segments)[seg_idx].bytes,
                           &(*segments)[seg_idx].len, encoding)) {
            free(segment);
            free(input_copy);
            for (int i = 0; i < seg_idx; i++) {
              if ((*segments)[i].bytes) free((*segments)[i].bytes);
            }
            free(*segments);
            return 0;
          }
          seg_idx++;
        }
      }

      free(segment);

      if (*p == '+') {
        seg_start = p + 1;
      }
    }
    p++;
  }

  free(input_copy);
  *segment_count = seg_idx;
  return 1;
}

// ============================================================================
// NEW: Parse input with capture groups: {pattern} or {name:pattern}
// ============================================================================
int parse_concatenated_input_with_captures(const char* input,
                                           PatternSegment** segments,
                                           int* segment_count,
                                           Encoding encoding,
                                           NamedGroup* defined_groups,
                                           int* defined_group_count) {
  *segments = NULL;
  *segment_count = 0;
  *defined_group_count = 0;

  typedef struct {
    PatternSegment* segments;
    int segment_count;
    int group_index;
    char name[MAX_GROUP_NAME_LEN + 1];
    int is_named;
  } GroupInfo;

  GroupInfo groups[MAX_CAPTURE_GROUPS];
  int group_count = 0;
  int numbered_count = 0;

  // Phase 1: Find all {...} and validate
  typedef struct {
    int start_pos;
    int end_pos;
  } BracePos;

  BracePos brace_positions[MAX_CAPTURE_GROUPS];
  int brace_count = 0;

  int in_quotes = 0;
  int brace_depth = 0;
  const char* brace_start = NULL;

  for (const char* p = input; *p; p++) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
    }

    if (!in_quotes) {
      if (*p == '{') {
        if (brace_depth == 0) brace_start = p;
        brace_depth++;
      } else if (*p == '}') {
        brace_depth--;
        if (brace_depth == 0 && brace_start) {
          if (brace_count >= MAX_CAPTURE_GROUPS) {
            print_error("Too many capture groups (max 99)", "Слишком много групп захвата (максимум 99)");
            return 0;
          }
          brace_positions[brace_count].start_pos = brace_start - input;
          brace_positions[brace_count].end_pos = p - input;
          brace_count++;
          brace_start = NULL;
        }
      }
    }
  }

  if (brace_depth != 0) {
    print_error("Unclosed capture group '{'", "Незакрытая группа захвата '{'");
    return 0;
  }

  // Phase 2: Parse each group and literal parts separately
  int current_pos = 0;
  PatternSegment* all_segments = NULL;
  int total_segment_count = 0;
  int total_segment_capacity = 100;

  all_segments = (PatternSegment*)calloc(total_segment_capacity, sizeof(PatternSegment));
  if (!all_segments) return 0;

  for (int brace_idx = 0; brace_idx <= brace_count; brace_idx++) {
    // Parse literal part before this brace (or after last brace)
    int literal_start = current_pos;
    int literal_end = (brace_idx < brace_count) ? brace_positions[brace_idx].start_pos : strlen(input);

    if (literal_end > literal_start) {
      // Extract literal part
      int literal_len = literal_end - literal_start;
      char* literal = (char*)malloc(literal_len + 1);
      if (!literal) {
        for (int i = 0; i < total_segment_count; i++) {
          if (all_segments[i].bytes) free(all_segments[i].bytes);
        }
        free(all_segments);
        return 0;
      }
      strncpy(literal, input + literal_start, literal_len);
      literal[literal_len] = '\0';

      // Parse literal part
      PatternSegment* literal_segments = NULL;
      int literal_seg_count = 0;

      if (!parse_concatenated_input(literal, &literal_segments, &literal_seg_count, encoding)) {
        free(literal);
        for (int i = 0; i < total_segment_count; i++) {
          if (all_segments[i].bytes) free(all_segments[i].bytes);
        }
        free(all_segments);
        return 0;
      }

      // Add to all_segments (not capture groups)
      for (int i = 0; i < literal_seg_count; i++) {
        if (total_segment_count >= total_segment_capacity) {
          total_segment_capacity *= 2;
          all_segments = (PatternSegment*)realloc(all_segments, total_segment_capacity * sizeof(PatternSegment));
          if (!all_segments) {
            free(literal);
            for (int j = 0; j < literal_seg_count; j++) {
              if (literal_segments[j].bytes) free(literal_segments[j].bytes);
            }
            free(literal_segments);
            return 0;
          }
        }
        all_segments[total_segment_count] = literal_segments[i];
        all_segments[total_segment_count].is_capture_group = 0;
        all_segments[total_segment_count].capture_group_index = -1;
        all_segments[total_segment_count].group_name[0] = '\0';
        total_segment_count++;
      }

      free(literal_segments);
      free(literal);
    }

    // Parse group content if this is not the last iteration
    if (brace_idx < brace_count) {
      int group_start = brace_positions[brace_idx].start_pos + 1;
      int group_end = brace_positions[brace_idx].end_pos;
      int group_len = group_end - group_start;

      char* group_content = (char*)malloc(group_len + 1);
      if (!group_content) {
        for (int i = 0; i < total_segment_count; i++) {
          if (all_segments[i].bytes) free(all_segments[i].bytes);
        }
        free(all_segments);
        return 0;
      }
      strncpy(group_content, input + group_start, group_len);
      group_content[group_len] = '\0';

      // Check if named: {name=pattern}
      char* equals = strchr(group_content, '=');
      char* pattern_part = group_content;
      int is_named = 0;
      char group_name[MAX_GROUP_NAME_LEN + 1] = {0};

      if (equals && equals > group_content) {
        int name_len = equals - group_content;
        if (!validate_group_name(group_content, name_len, group_start)) {
          free(group_content);
          for (int i = 0; i < total_segment_count; i++) {
            if (all_segments[i].bytes) free(all_segments[i].bytes);
          }
          free(all_segments);
          return 0;
        }

        // Check duplicates
        for (int i = 0; i < group_count; i++) {
          if (groups[i].is_named && strncmp(groups[i].name, group_content, name_len) == 0 &&
              groups[i].name[name_len] == '\0') {
            char temp[MAX_GROUP_NAME_LEN + 1];
            strncpy(temp, group_content, name_len);
            temp[name_len] = '\0';
            print_error_str("Duplicate capture group name", "Дублирующееся имя группы захвата", temp);
            free(group_content);
            for (int i = 0; i < total_segment_count; i++) {
              if (all_segments[i].bytes) free(all_segments[i].bytes);
            }
            free(all_segments);
            return 0;
          }
        }

        strncpy(group_name, group_content, name_len);
        group_name[name_len] = '\0';
        pattern_part = equals + 1;
        is_named = 1;
      } else {
        numbered_count++;
        if (numbered_count > 9) {
          print_error("Too many numbered capture groups (max 9)", "Слишком много нумерованных групп захвата (максимум 9)");
          free(group_content);
          for (int i = 0; i < total_segment_count; i++) {
            if (all_segments[i].bytes) free(all_segments[i].bytes);
          }
          free(all_segments);
          return 0;
        }
      }

      // Parse group pattern
      PatternSegment* group_segments = NULL;
      int group_seg_count = 0;

      if (!parse_concatenated_input(pattern_part, &group_segments, &group_seg_count, encoding)) {
        free(group_content);
        for (int i = 0; i < total_segment_count; i++) {
          if (all_segments[i].bytes) free(all_segments[i].bytes);
        }
        free(all_segments);
        return 0;
      }

      // Add to all_segments (mark as capture group)
      for (int i = 0; i < group_seg_count; i++) {
        if (total_segment_count >= total_segment_capacity) {
          total_segment_capacity *= 2;
          all_segments = (PatternSegment*)realloc(all_segments, total_segment_capacity * sizeof(PatternSegment));
          if (!all_segments) {
            free(group_content);
            for (int j = 0; j < group_seg_count; j++) {
              if (group_segments[j].bytes) free(group_segments[j].bytes);
            }
            free(group_segments);
            return 0;
          }
        }
        all_segments[total_segment_count] = group_segments[i];
        all_segments[total_segment_count].is_capture_group = 1;
        all_segments[total_segment_count].capture_group_index = group_count;
        // Store group name in segment (for both named and numbered groups)
        if (is_named) {
          strcpy(all_segments[total_segment_count].group_name, group_name);
        } else {
          all_segments[total_segment_count].group_name[0] = '\0';
        }
        total_segment_count++;
      }

      // Save group info
      groups[group_count].segments = group_segments;
      groups[group_count].segment_count = group_seg_count;
      groups[group_count].group_index = group_count;
      groups[group_count].is_named = is_named;
      if (is_named) {
        strcpy(groups[group_count].name, group_name);
      } else {
        groups[group_count].name[0] = '\0';
      }
      group_count++;

      free(group_segments);
      free(group_content);

      current_pos = brace_positions[brace_idx].end_pos + 1;
    }
  }

  // Phase 3: Build defined_groups - add ALL named groups
  for (int i = 0; i < group_count; i++) {
    if (groups[i].is_named) {
      // Named group: add to defined_groups
      strcpy(defined_groups[*defined_group_count].name, groups[i].name);
      defined_groups[*defined_group_count].index = groups[i].group_index;
      (*defined_group_count)++;
    }
  }

  // Phase 4: Remove empty segments (artifacts from parsing)
  int final_count = 0;
  for (int i = 0; i < total_segment_count; i++) {
    if (all_segments[i].is_wildcard || all_segments[i].len > 0) {
      if (final_count != i) {
        all_segments[final_count] = all_segments[i];
      }
      final_count++;
    } else {
      // Free empty segment
      if (all_segments[i].bytes) free(all_segments[i].bytes);
    }
  }
  total_segment_count = final_count;

  *segments = all_segments;
  *segment_count = total_segment_count;
  return 1;
}

int parse_file_spec(const char* spec, char** input_file, char** output_file,
                    Encoding* input_enc, Encoding* output_enc, int* use_stdio) {
  *input_enc = ENCODING_UTF8;
  *output_enc = ENCODING_UTF8;
  *use_stdio = 0;
  *input_file = NULL;
  *output_file = NULL;

  if (strcmp(spec, "-") == 0) {
    *use_stdio = 1;
    return 1;
  }

  char* spec_copy = strdup(spec);
  if (!spec_copy) return 0;

  char* parts[4] = {NULL, NULL, NULL, NULL};
  int part_count = 0;
  char* token = spec_copy;
  char* next;

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
  if (parts[0] &&
      (strcmp(parts[0], "win") == 0 || strcmp(parts[0], "dos") == 0 ||
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
    if (parts[idx] &&
        (strcmp(parts[idx], "win") == 0 || strcmp(parts[idx], "dos") == 0 ||
         strcmp(parts[idx], "koi") == 0 || strcmp(parts[idx], "utf") == 0)) {
      *output_enc = parse_encoding(parts[idx]);
    }

    free(spec_copy);
    return 1;
  }

  *input_file = strdup(parts[idx]);
  idx++;

  // Check next part - could be output encoding, output filename, or "-" for
  // stdout
  if (parts[idx]) {
    if (strcmp(parts[idx], "-") == 0) {
      // Output to stdout
      *use_stdio = 1;
      idx++;

      // Check for output encoding after "-"
      if (parts[idx] &&
          (strcmp(parts[idx], "win") == 0 || strcmp(parts[idx], "dos") == 0 ||
           strcmp(parts[idx], "koi") == 0 || strcmp(parts[idx], "utf") == 0)) {
        *output_enc = parse_encoding(parts[idx]);
      }
    } else if (strcmp(parts[idx], "win") == 0 ||
               strcmp(parts[idx], "dos") == 0 ||
               strcmp(parts[idx], "koi") == 0 ||
               strcmp(parts[idx], "utf") == 0) {
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

char* create_output_filename(const char* input_filename) {
  size_t len = strlen(input_filename);
  const char* dot = strrchr(input_filename, '.');
  const char* slash = strrchr(input_filename, '\\');
  if (!slash) slash = strrchr(input_filename, '/');

  char* output = (char*)malloc(len + 10);
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

int apply_encoding_conversion(unsigned char* buffer, size_t buffer_size,
                              Encoding input_enc, Encoding output_enc,
                              unsigned char** result, size_t* result_size) {
  if (input_enc == output_enc) {
    *result = (unsigned char*)malloc(buffer_size);
    if (!*result) return 0;
    memcpy(*result, buffer, buffer_size);
    *result_size = buffer_size;
    return 1;
  }

  int input_cp = get_codepage(input_enc);
  int output_cp = get_codepage(output_enc);

  int wide_len =
      MultiByteToWideChar(input_cp, 0, (char*)buffer, buffer_size, NULL, 0);
  if (wide_len == 0) return 0;

  wchar_t* wide_str = (wchar_t*)malloc((wide_len + 1) * sizeof(wchar_t));
  if (!wide_str) return 0;

  MultiByteToWideChar(input_cp, 0, (char*)buffer, buffer_size, wide_str,
                      wide_len);
  wide_str[wide_len] = 0;

  int byte_len = WideCharToMultiByte(output_cp, 0, wide_str, wide_len, NULL, 0,
                                     NULL, NULL);
  if (byte_len == 0) {
    free(wide_str);
    return 0;
  }

  *result = (unsigned char*)malloc(byte_len);
  if (!*result) {
    free(wide_str);
    return 0;
  }

  WideCharToMultiByte(output_cp, 0, wide_str, wide_len, (char*)*result,
                      byte_len, NULL, NULL);
  *result_size = byte_len;

  free(wide_str);
  return 1;
}

// Match pattern with wildcards against buffer
// Returns match length if found, 0 if not found
size_t match_pattern(unsigned char* buffer, size_t buffer_size,
                     PatternSegment* segments, int segment_count,
                     size_t start_pos, int ignore_case) {
  size_t pos = start_pos;
  size_t total_matched = 0;

  for (int seg_idx = 0; seg_idx < segment_count; seg_idx++) {
    PatternSegment* seg = &segments[seg_idx];

    if (!seg->is_wildcard) {
      // Literal bytes - exact match (case-sensitive or case-insensitive)
      if (pos + seg->len > buffer_size) return 0;

      if (ignore_case) {
        // Case-insensitive comparison
        int match = 1;
        for (size_t i = 0; i < seg->len; i++) {
          if (tolower(buffer[pos + i]) != tolower(seg->bytes[i])) {
            match = 0;
            break;
          }
        }
        if (!match) return 0;
      } else {
        // Case-sensitive comparison
        if (memcmp(buffer + pos, seg->bytes, seg->len) != 0) return 0;
      }

      pos += seg->len;
      total_matched += seg->len;
    } else {
      // Wildcard
      if (seg->wildcard_type == WILDCARD_ANY_BYTE) {
        // . - match any single byte
        if (pos >= buffer_size) return 0;
        pos++;
        total_matched++;
      } else if (seg->wildcard_type == WILDCARD_OPTIONAL) {
        // ? - match zero or one byte
        if (seg_idx == segment_count - 1) {
          // Last segment - match one byte if available
          if (pos < buffer_size) {
            pos++;
            total_matched++;
          }
          // Otherwise match zero bytes (do nothing)
        } else {
          // Not last segment - need to check what comes next
          PatternSegment* next_seg = &segments[seg_idx + 1];

          if (!next_seg->is_wildcard) {
            // Next is literal - try matching with one byte first
            size_t saved_pos = pos;
            int matched_with_byte = 0;

            if (pos < buffer_size) {
              // Try with one byte consumed
              size_t test_pos = pos + 1;
              int match = 1;
              if (test_pos + next_seg->len <= buffer_size) {
                if (ignore_case) {
                  for (size_t i = 0; i < next_seg->len; i++) {
                    if (tolower(buffer[test_pos + i]) != tolower(next_seg->bytes[i])) {
                      match = 0;
                      break;
                    }
                  }
                } else {
                  if (memcmp(buffer + test_pos, next_seg->bytes, next_seg->len) != 0) {
                    match = 0;
                  }
                }
                if (match) {
                  pos++;
                  total_matched++;
                  matched_with_byte = 1;
                }
              }
            }

            if (!matched_with_byte) {
              // Try with zero bytes (skip the optional byte)
              int match = 1;
              if (pos + next_seg->len <= buffer_size) {
                if (ignore_case) {
                  for (size_t i = 0; i < next_seg->len; i++) {
                    if (tolower(buffer[pos + i]) != tolower(next_seg->bytes[i])) {
                      match = 0;
                      break;
                    }
                  }
                } else {
                  if (memcmp(buffer + pos, next_seg->bytes, next_seg->len) != 0) {
                    match = 0;
                  }
                }
                if (match) {
                  // Match with zero bytes - do nothing, continue to next segment
                } else {
                  return 0;  // Neither option matches
                }
              } else {
                return 0;
              }
            }
          } else {
            // Next is also wildcard - match one byte if available
            if (pos < buffer_size) {
              pos++;
              total_matched++;
            }
          }
        }
      } else if (seg->wildcard_type == WILDCARD_ZERO_OR_MORE) {
        // * - match zero or more bytes (greedy with backtracking)
        if (seg_idx == segment_count - 1) {
          // Last segment - match everything remaining
          total_matched += (buffer_size - pos);
          pos = buffer_size;
        } else {
          // Look for next literal segment
          PatternSegment* next_seg = &segments[seg_idx + 1];
          if (!next_seg->is_wildcard) {
            // Find next occurrence of literal
            size_t search_start = pos;
            size_t found_pos = buffer_size;

            for (size_t i = search_start; i <= buffer_size - next_seg->len; i++) {
              int match = 1;
              if (ignore_case) {
                for (size_t j = 0; j < next_seg->len; j++) {
                  if (tolower(buffer[i + j]) != tolower(next_seg->bytes[j])) {
                    match = 0;
                    break;
                  }
                }
              } else {
                if (memcmp(buffer + i, next_seg->bytes, next_seg->len) != 0) {
                  match = 0;
                }
              }
              if (match) {
                found_pos = i;
                break;
              }
            }

            if (found_pos == buffer_size) return 0;  // Not found

            total_matched += (found_pos - pos);
            pos = found_pos;
          } else {
            // Next is also wildcard - match zero bytes for now
            // (simplified - could be improved)
          }
        }
      }
    }
  }

  return total_matched;
}

// ============================================================================
// NEW: Match pattern with capture groups support
// ============================================================================
size_t match_pattern_with_captures(unsigned char* buffer, size_t buffer_size,
                                   PatternSegment* segments, int segment_count,
                                   size_t start_pos, CaptureContext* captures,
                                   int ignore_case) {
  size_t pos = start_pos;
  size_t total_matched = 0;

  // Track capture group boundaries
  typedef struct {
    size_t start;
    size_t length;
    int active;
  } GroupBoundary;

  GroupBoundary boundaries[MAX_CAPTURE_GROUPS] = {0};
  int current_group_idx = -1;

  for (int seg_idx = 0; seg_idx < segment_count; seg_idx++) {
    PatternSegment* seg = &segments[seg_idx];

    // Check if entering/exiting capture groups
    if (seg->is_capture_group) {
      // Entering or continuing a capture group
      if (seg->capture_group_index != current_group_idx) {
        // Close previous group if any
        if (current_group_idx >= 0 && current_group_idx < MAX_CAPTURE_GROUPS) {
          boundaries[current_group_idx].active = 0;
        }
        // Start new group
        current_group_idx = seg->capture_group_index;
        if (current_group_idx >= 0 && current_group_idx < MAX_CAPTURE_GROUPS) {
          boundaries[current_group_idx].start = pos;
          boundaries[current_group_idx].length = 0;
          boundaries[current_group_idx].active = 1;
        }
      }
    } else {
      // Not in a capture group - close current group if any
      if (current_group_idx >= 0 && current_group_idx < MAX_CAPTURE_GROUPS) {
        boundaries[current_group_idx].active = 0;
      }
      current_group_idx = -1;
    }

    size_t matched_len = 0;

    if (!seg->is_wildcard) {
      // Literal bytes - exact match (case-sensitive or case-insensitive)
      if (pos + seg->len > buffer_size) return 0;

      if (ignore_case) {
        // Case-insensitive comparison
        int match = 1;
        for (size_t i = 0; i < seg->len; i++) {
          if (tolower(buffer[pos + i]) != tolower(seg->bytes[i])) {
            match = 0;
            break;
          }
        }
        if (!match) return 0;
      } else {
        // Case-sensitive comparison
        if (memcmp(buffer + pos, seg->bytes, seg->len) != 0) return 0;
      }

      matched_len = seg->len;
    } else {
      // Wildcard matching (same logic as original match_pattern)
      if (seg->wildcard_type == WILDCARD_ANY_BYTE) {
        if (pos >= buffer_size) return 0;
        matched_len = 1;
      } else if (seg->wildcard_type == WILDCARD_OPTIONAL) {
        if (seg_idx == segment_count - 1) {
          if (pos < buffer_size) {
            matched_len = 1;
          }
        } else {
          PatternSegment* next_seg = &segments[seg_idx + 1];
          if (!next_seg->is_wildcard) {
            int matched_with_byte = 0;
            if (pos < buffer_size) {
              size_t test_pos = pos + 1;
              int match = 1;
              if (test_pos + next_seg->len <= buffer_size) {
                if (ignore_case) {
                  for (size_t i = 0; i < next_seg->len; i++) {
                    if (tolower(buffer[test_pos + i]) != tolower(next_seg->bytes[i])) {
                      match = 0;
                      break;
                    }
                  }
                } else {
                  if (memcmp(buffer + test_pos, next_seg->bytes, next_seg->len) != 0) {
                    match = 0;
                  }
                }
                if (match) {
                  matched_len = 1;
                  matched_with_byte = 1;
                }
              }
            }
            if (!matched_with_byte) {
              int match = 1;
              if (pos + next_seg->len <= buffer_size) {
                if (ignore_case) {
                  for (size_t i = 0; i < next_seg->len; i++) {
                    if (tolower(buffer[pos + i]) != tolower(next_seg->bytes[i])) {
                      match = 0;
                      break;
                    }
                  }
                } else {
                  if (memcmp(buffer + pos, next_seg->bytes, next_seg->len) != 0) {
                    match = 0;
                  }
                }
                if (match) {
                  matched_len = 0;
                } else {
                  return 0;
                }
              } else {
                return 0;
              }
            }
          } else {
            if (pos < buffer_size) {
              matched_len = 1;
            }
          }
        }
      } else if (seg->wildcard_type == WILDCARD_ZERO_OR_MORE) {
        if (seg_idx == segment_count - 1) {
          matched_len = buffer_size - pos;
        } else {
          PatternSegment* next_seg = &segments[seg_idx + 1];
          if (!next_seg->is_wildcard) {
            size_t found_pos = buffer_size;
            for (size_t i = pos; i <= buffer_size - next_seg->len; i++) {
              int match = 1;
              if (ignore_case) {
                for (size_t j = 0; j < next_seg->len; j++) {
                  if (tolower(buffer[i + j]) != tolower(next_seg->bytes[j])) {
                    match = 0;
                    break;
                  }
                }
              } else {
                if (memcmp(buffer + i, next_seg->bytes, next_seg->len) != 0) {
                  match = 0;
                }
              }
              if (match) {
                found_pos = i;
                break;
              }
            }
            if (found_pos == buffer_size) return 0;
            matched_len = found_pos - pos;
          } else {
            matched_len = 0;
          }
        }
      }
    }

    pos += matched_len;
    total_matched += matched_len;

    // Accumulate length for current group
    if (current_group_idx >= 0 && current_group_idx < MAX_CAPTURE_GROUPS &&
        boundaries[current_group_idx].active) {
      boundaries[current_group_idx].length += matched_len;
    }
  }

  // Save all captured groups
  for (int i = 0; i < MAX_CAPTURE_GROUPS; i++) {
    if (boundaries[i].length > 0) {
      captures->captures[i].data = buffer + boundaries[i].start;
      captures->captures[i].len = boundaries[i].length;
      if (i + 1 > captures->capture_count) {
        captures->capture_count = i + 1;
      }
    }
  }

  return total_matched;
}

// Build replacement string with capture references (\0-\9, {name})
int build_replacement_with_captures(const char* template,
                                    CaptureContext* captures,
                                    Encoding encoding,
                                    unsigned char** result,
                                    size_t* result_len) {
  if (!template || !captures) return 0;

  // First pass: calculate total size needed
  size_t total_size = 0;
  const char* p = template;
  int in_quotes = 0;

  while (*p) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
      p++;
      continue;
    }

    // Skip + outside quotes (concatenation operator)
    if (!in_quotes && *p == '+') {
      p++;
      continue;
    }

    if (*p == '\\' && p[1] >= '0' && p[1] <= '9') {
      // \0-\9
      int num = p[1] - '0';
      if (num == 0) {
        total_size += captures->entire_match.len;
      } else if (num <= captures->capture_count) {
        total_size += captures->captures[num - 1].len;
      }
      p += 2;
    } else if (!in_quotes && *p == '{') {
      // {name} - find closing }
      const char* close = strchr(p + 1, '}');
      if (close) {
        int name_len = close - p - 1;
        char name[MAX_GROUP_NAME_LEN + 1];
        if (name_len > 0 && name_len <= MAX_GROUP_NAME_LEN) {
          strncpy(name, p + 1, name_len);
          name[name_len] = '\0';

          // Find in named groups
          int found = 0;
          for (int i = 0; i < captures->named_group_count; i++) {
            if (strcmp(captures->named_groups[i].name, name) == 0) {
              int idx = captures->named_groups[i].index;
              if (idx < captures->capture_count) {
                total_size += captures->captures[idx].len;
              }
              found = 1;
              break;
            }
          }

          if (!found) {
            print_error_str(
              "Unknown capture group in replacement",
              "Неизвестная группа захвата в замене",
              name
            );
            return 0;
          }
        }
        p = close + 1;
      } else {
        total_size++;
        p++;
      }
    } else if (*p == '\\' && p[1]) {
      // Escape sequences
      total_size++;
      p += 2;
    } else {
      total_size++;
      p++;
    }
  }

  // Allocate result buffer
  *result = (unsigned char*)malloc(total_size);
  if (!*result) return 0;

  // Second pass: build the replacement
  size_t out_pos = 0;
  p = template;
  in_quotes = 0;

  while (*p) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
      p++;
      continue;
    }

    // Skip + outside quotes (concatenation operator)
    if (!in_quotes && *p == '+') {
      p++;
      continue;
    }

    if (*p == '\\' && p[1] >= '0' && p[1] <= '9') {
      // \0-\9
      int num = p[1] - '0';
      if (num == 0) {
        memcpy(*result + out_pos, captures->entire_match.data, captures->entire_match.len);
        out_pos += captures->entire_match.len;
      } else if (num <= captures->capture_count) {
        memcpy(*result + out_pos, captures->captures[num - 1].data, captures->captures[num - 1].len);
        out_pos += captures->captures[num - 1].len;
      }
      p += 2;
    } else if (!in_quotes && *p == '{') {
      // {name}
      const char* close = strchr(p + 1, '}');
      if (close) {
        int name_len = close - p - 1;
        char name[MAX_GROUP_NAME_LEN + 1];
        if (name_len > 0 && name_len <= MAX_GROUP_NAME_LEN) {
          strncpy(name, p + 1, name_len);
          name[name_len] = '\0';

          int found = 0;
          for (int i = 0; i < captures->named_group_count; i++) {
            if (strcmp(captures->named_groups[i].name, name) == 0) {
              int idx = captures->named_groups[i].index;
              if (idx < captures->capture_count) {
                memcpy(*result + out_pos, captures->captures[idx].data, captures->captures[idx].len);
                out_pos += captures->captures[idx].len;
              }
              found = 1;
              break;
            }
          }
        }
        p = close + 1;
      } else {
        (*result)[out_pos++] = *p++;
      }
    } else if (*p == '\\' && p[1]) {
      // Handle escape sequences
      switch (p[1]) {
        case 'n':
          (*result)[out_pos++] = '\n';
          p += 2;
          break;
        case 'r':
          (*result)[out_pos++] = '\r';
          p += 2;
          break;
        case 't':
          (*result)[out_pos++] = '\t';
          p += 2;
          break;
        case '\\':
          (*result)[out_pos++] = '\\';
          p += 2;
          break;
        default:
          // Unknown escape - keep as is
          (*result)[out_pos++] = *p++;
          break;
      }
    } else {
      (*result)[out_pos++] = *p++;
    }
  }

  *result_len = total_size;
  return 1;
}

// Forward declaration
int apply_operations(unsigned char* buffer, size_t buffer_size, Operation* ops,
                     int op_count, unsigned char** result, size_t* result_size,
                     int* total_replacements);

// Preview operations without modifying data (dry-run mode)
int preview_operations(unsigned char* buffer, size_t buffer_size, Operation* ops,
                       int op_count, int* total_matches) {
  *total_matches = 0;

  // Work with a copy to simulate sequential operations
  unsigned char* current = (unsigned char*)malloc(buffer_size);
  if (!current) return 0;
  memcpy(current, buffer, buffer_size);
  size_t current_size = buffer_size;

  // Process each operation sequentially
  for (int op_idx = 0; op_idx < op_count; op_idx++) {
    Operation* op = &ops[op_idx];
    int matches = 0;

    // Find line boundaries for context display
    size_t* line_starts = (size_t*)malloc(sizeof(size_t) * (current_size + 1));
    if (!line_starts) {
      free(current);
      return 0;
    }

    int line_count = 0;
    line_starts[line_count++] = 0;
    for (size_t i = 0; i < current_size; i++) {
      if (current[i] == '\n' && i + 1 < current_size) {
        line_starts[line_count++] = i + 1;
      }
    }

    fprintf(stderr, COLOR_YELLOW "Operation %d:" COLOR_RESET " ", op_idx + 1);
    if (op->pattern_type == PATTERN_LITERAL) {
      fprintf(stderr, "Literal pattern: \"");
      for (size_t i = 0; i < op->search_len && i < 30; i++) {
        if (op->search_bytes[i] >= 32 && op->search_bytes[i] < 127) {
          fputc(op->search_bytes[i], stderr);
        } else {
          fprintf(stderr, "\\x%02X", op->search_bytes[i]);
        }
      }
      if (op->search_len > 30) fprintf(stderr, "...");
      fprintf(stderr, "\"");
    } else {
      fprintf(stderr, "Wildcard pattern");
    }
    if (op->ignore_case) {
      fprintf(stderr, " " COLOR_CYAN "[case-insensitive]" COLOR_RESET);
    }

    // Show replacement
    fprintf(stderr, " -> ");
    if (op->delete_mode) {
      fprintf(stderr, COLOR_RED "DELETE" COLOR_RESET);
    } else if (op->has_captures_in_replace) {
      fprintf(stderr, COLOR_GREEN "\"");
      if (op->replace_template) {
        // Show template with capture references
        for (const char* p = op->replace_template; *p && (p - op->replace_template) < 30; p++) {
          if (*p == '"' || *p == '\'') continue;  // Skip quotes
          fputc(*p, stderr);
        }
        if (strlen(op->replace_template) > 30) fprintf(stderr, "...");
      }
      fprintf(stderr, "\"" COLOR_RESET);
    } else {
      fprintf(stderr, COLOR_GREEN "\"");
      for (size_t i = 0; i < op->replace_len && i < 30; i++) {
        if (op->replace_bytes[i] >= 32 && op->replace_bytes[i] < 127) {
          fputc(op->replace_bytes[i], stderr);
        } else {
          fprintf(stderr, "\\x%02X", op->replace_bytes[i]);
        }
      }
      if (op->replace_len > 30) fprintf(stderr, "...");
      fprintf(stderr, "\"" COLOR_RESET);
    }
    fprintf(stderr, "\n");

    // Collect all matches for this operation
    typedef struct {
      size_t pos;
      size_t len;
      int line_num;
    } MatchInfo;

    MatchInfo* match_list = (MatchInfo*)malloc(sizeof(MatchInfo) * 1000);
    if (!match_list) {
      free(line_starts);
      free(current);
      return 0;
    }
    int match_count = 0;

    // Find all matches
    size_t i = 0;
    while (i < current_size && match_count < 1000) {
      size_t match_pos = i;
      size_t match_len = 0;

      if (op->pattern_type == PATTERN_LITERAL) {
        if (i + op->search_len <= current_size) {
          int match = 0;
          if (op->ignore_case) {
            match = 1;
            for (size_t j = 0; j < op->search_len; j++) {
              if (tolower(current[i + j]) != tolower(op->search_bytes[j])) {
                match = 0;
                break;
              }
            }
          } else {
            match = (memcmp(current + i, op->search_bytes, op->search_len) == 0);
          }
          if (match) match_len = op->search_len;
        }
      } else {
        CaptureContext captures = {0};
        captures.entire_match.data = current + i;
        for (int ng = 0; ng < op->defined_group_count; ng++) {
          captures.named_groups[ng] = op->defined_groups[ng];
        }
        captures.named_group_count = op->defined_group_count;
        match_len = match_pattern_with_captures(current, current_size, op->segments,
                                                op->segment_count, i, &captures, op->ignore_case);
      }

      if (match_len > 0) {
        // Find line number
        int line_num = 0;
        for (int l = 0; l < line_count; l++) {
          if (line_starts[l] <= match_pos) {
            line_num = l + 1;
          } else {
            break;
          }
        }

        match_list[match_count].pos = match_pos;
        match_list[match_count].len = match_len;
        match_list[match_count].line_num = line_num;
        match_count++;
        matches++;
        i += match_len;
      } else {
        i++;
      }
    }

    // Display matches with inverted colors
    for (int m = 0; m < match_count; m++) {
      size_t match_pos = match_list[m].pos;
      size_t match_len = match_list[m].len;
      int line_num = match_list[m].line_num;

      size_t line_start = line_starts[line_num - 1];
      size_t line_end = (line_num < line_count) ? line_starts[line_num] - 1 : current_size;
      if (line_end > 0 && current[line_end - 1] == '\n') line_end--;

      fprintf(stderr, "  Line %d: \"", line_num);

      // Print line with inverted match
      for (size_t p = line_start; p < line_end && p < line_start + 80; p++) {
        // Start inversion at match beginning
        if (p == match_pos) {
          fprintf(stderr, "\033[7m");  // Inverted colors
        }

        if (current[p] >= 32 && current[p] < 127) {
          fputc(current[p], stderr);
        } else if (current[p] == '\t') {
          fputc('\t', stderr);
        } else {
          fprintf(stderr, "\\x%02X", current[p]);
        }

        // End inversion at match end
        if (p == match_pos + match_len - 1) {
          fprintf(stderr, "\033[0m");  // Reset colors
        }
      }
      fprintf(stderr, "\033[0m");  // Ensure reset
      if (line_end > line_start + 80) fprintf(stderr, "...");
      fprintf(stderr, "\"\n");
    }

    fprintf(stderr, "  " COLOR_GREEN "Matches found: %d" COLOR_RESET "\n\n", matches);
    *total_matches += matches;

    free(match_list);
    free(line_starts);

    // Apply this operation to current buffer for next operation
    if (op_idx < op_count - 1) {
      unsigned char* temp_result = NULL;
      size_t temp_size = 0;
      int temp_replacements = 0;

      if (!apply_operations(current, current_size, op, 1, &temp_result, &temp_size, &temp_replacements)) {
        free(current);
        return 0;
      }

      free(current);
      current = temp_result;
      current_size = temp_size;
    }
  }

  free(current);
  return 1;
}

int apply_operations(unsigned char* buffer, size_t buffer_size, Operation* ops,
                     int op_count, unsigned char** result, size_t* result_size,
                     int* total_replacements) {
  unsigned char* current = buffer;
  size_t current_size = buffer_size;
  *total_replacements = 0;

  for (int op_idx = 0; op_idx < op_count; op_idx++) {
    Operation* op = &ops[op_idx];

    unsigned char* temp = (unsigned char*)malloc(current_size * 2 + 1024);
    if (!temp) {
      if (op_idx > 0) free(current);
      return 0;
    }

    size_t temp_pos = 0;
    size_t i = 0;
    int replacements = 0;

    while (i < current_size) {
      if (op->pattern_type == PATTERN_LITERAL) {
        // Literal pattern matching (case-sensitive or case-insensitive)
        if (i + op->search_len <= current_size) {
          int match = 0;
          if (op->ignore_case) {
            // Case-insensitive comparison
            match = 1;
            for (size_t j = 0; j < op->search_len; j++) {
              if (tolower(current[i + j]) != tolower(op->search_bytes[j])) {
                match = 0;
                break;
              }
            }
          } else {
            // Case-sensitive comparison
            match = (memcmp(current + i, op->search_bytes, op->search_len) == 0);
          }

          if (match) {
            if (!op->delete_mode) {
              if (op->has_captures_in_replace) {
                // Build replacement with \0 substitution
                CaptureContext captures = {0};
                captures.entire_match.data = current + i;
                captures.entire_match.len = op->search_len;

                unsigned char* replacement = NULL;
                size_t replacement_len = 0;
                if (build_replacement_with_captures(op->replace_template, &captures,
                                                    ENCODING_UTF8, &replacement, &replacement_len)) {
                  memcpy(temp + temp_pos, replacement, replacement_len);
                  temp_pos += replacement_len;
                  free(replacement);
                }
              } else {
                // Use static replacement
                memcpy(temp + temp_pos, op->replace_bytes, op->replace_len);
                temp_pos += op->replace_len;
              }
            }
            i += op->search_len;
            replacements++;
          } else {
            temp[temp_pos++] = current[i++];
          }
        } else {
          temp[temp_pos++] = current[i++];
        }
      } else {
        // Wildcard matching with capture groups
        CaptureContext captures = {0};
        captures.entire_match.data = current + i;

        // Copy named groups from operation
        for (int ng = 0; ng < op->defined_group_count; ng++) {
          captures.named_groups[ng] = op->defined_groups[ng];
        }
        captures.named_group_count = op->defined_group_count;

        size_t match_len = match_pattern_with_captures(current, current_size, op->segments,
                                         op->segment_count, i, &captures, op->ignore_case);
        if (match_len > 0) {
          // Update entire_match length
          captures.entire_match.len = match_len;

          // Replace matched bytes
          if (!op->delete_mode) {
            if (op->has_captures_in_replace) {
              // Build replacement with captures
              unsigned char* replacement = NULL;
              size_t replacement_len = 0;
              if (build_replacement_with_captures(op->replace_template, &captures,
                                                  ENCODING_UTF8, &replacement, &replacement_len)) {
                memcpy(temp + temp_pos, replacement, replacement_len);
                temp_pos += replacement_len;
                free(replacement);
              }
            } else {
              // Use static replacement
              memcpy(temp + temp_pos, op->replace_bytes, op->replace_len);
              temp_pos += op->replace_len;
            }
          }
          i += match_len;
          replacements++;
        } else {
          temp[temp_pos++] = current[i++];
        }
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

int parse_operation(const char* arg, Operation* op, Encoding encoding) {
  // Find first colon outside of quotes AND outside of braces
  const char* colon = NULL;
  int in_quotes = 0;
  int brace_depth = 0;
  for (const char* p = arg; *p; p++) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
    }
    if (!in_quotes) {
      if (*p == '{') brace_depth++;
      if (*p == '}') brace_depth--;
      if (*p == ':' && brace_depth == 0) {
        colon = p;
        break;
      }
    }
  }

  if (!colon) {
    fprintf(stderr,
            "Error: Invalid operation format. Use search:replace or "
            "search:replace:encoding\n");
    return 0;
  }

  size_t search_len = colon - arg;
  char* search_str = (char*)malloc(search_len + 1);
  if (!search_str) return 0;
  strncpy(search_str, arg, search_len);
  search_str[search_len] = '\0';

  char* replace_start = (char*)(colon + 1);
  const char* second_colon = NULL;
  in_quotes = 0;
  brace_depth = 0;
  for (const char* p = replace_start; *p; p++) {
    if (*p == '"' || *p == '\'') {
      in_quotes = !in_quotes;
    }
    if (!in_quotes) {
      if (*p == '{') brace_depth++;
      if (*p == '}') brace_depth--;
      if (*p == ':' && brace_depth == 0) {
        second_colon = p;
        break;
      }
    }
  }

  char* replace_str = NULL;
  const char* encoding_str = NULL;
  int ignore_case = 0;

  if (second_colon) {
    size_t replace_len = second_colon - replace_start;
    replace_str = (char*)malloc(replace_len + 1);
    if (!replace_str) {
      free(search_str);
      return 0;
    }
    strncpy(replace_str, replace_start, replace_len);
    replace_str[replace_len] = '\0';
    encoding_str = second_colon + 1;

    // Check for /i flag at the end
    size_t enc_len = strlen(encoding_str);
    if (enc_len >= 2 && encoding_str[enc_len - 2] == '/' && encoding_str[enc_len - 1] == 'i') {
      ignore_case = 1;
      // Remove /i from encoding string
      char* enc_copy = strdup(encoding_str);
      if (enc_copy) {
        enc_copy[enc_len - 2] = '\0';

        // Remove trailing spaces before /i
        size_t copy_len = strlen(enc_copy);
        while (copy_len > 0 && enc_copy[copy_len - 1] == ' ') {
          enc_copy[copy_len - 1] = '\0';
          copy_len--;
        }

        encoding = parse_encoding(enc_copy);
        free(enc_copy);
      }
    } else {
      encoding = parse_encoding(encoding_str);
    }
  } else {
    replace_str = strdup(replace_start);

    // Check for /i flag at the end of replace_str
    size_t rep_len = strlen(replace_str);
    if (rep_len >= 2 && replace_str[rep_len - 2] == '/' && replace_str[rep_len - 1] == 'i') {
      ignore_case = 1;
      replace_str[rep_len - 2] = '\0';  // Remove /i

      // Remove trailing spaces before /i
      rep_len = strlen(replace_str);
      while (rep_len > 0 && replace_str[rep_len - 1] == ' ') {
        replace_str[rep_len - 1] = '\0';
        rep_len--;
      }
    }
  }

  // Check if search string contains '+', wildcards, or capture groups
  int has_concat = 0;
  int has_wildcard = 0;
  int has_capture_groups = 0;

  in_quotes = 0;
  brace_depth = 0;
  for (const char* p = search_str; *p; p++) {
    if (*p == '"' || *p == '\'') in_quotes = !in_quotes;
    if (!in_quotes && *p == '+') has_concat = 1;

    // Check for capture group definition: {pattern} or {name=pattern}
    // NOT just {name} (which is a reference)
    if (!in_quotes && *p == '{') {
      // Look ahead to see if this is a group definition
      const char* close = strchr(p + 1, '}');
      if (close) {
        int content_len = close - p - 1;
        // Check if content has = or wildcards (definition) vs just name (reference)
        int has_equals = 0;
        int has_wildcard_inside = 0;
        for (const char* c = p + 1; c < close; c++) {
          if (*c == '=') has_equals = 1;
          if (*c == '*' || *c == '.' || *c == '?') has_wildcard_inside = 1;
        }
        // It's a group definition if it has = or wildcards
        if (has_equals || has_wildcard_inside) {
          has_capture_groups = 1;
        }
      }
    }

    // Check for both escaped and unescaped wildcards (outside quotes)
    if (!in_quotes && (*p == '.' || *p == '*' || *p == '?')) {
      has_wildcard = 1;
    }
    if (*p == '\\' && (p[1] == '.' || p[1] == '*' || p[1] == '?')) {
      has_wildcard = 1;
    }
  }

  // Initialize defined_groups
  op->defined_group_count = 0;

  if (has_capture_groups) {
    // Use capture groups parsing
    op->pattern_type = PATTERN_WILDCARD;
    if (!parse_concatenated_input_with_captures(search_str, &op->segments, &op->segment_count,
                                  encoding, op->defined_groups, &op->defined_group_count)) {
      free(search_str);
      free(replace_str);
      return 0;
    }
    op->search_bytes = NULL;
    op->search_len = 0;
  } else if (has_concat) {
    // Use concatenation parsing (handles both + and wildcards)
    op->pattern_type = PATTERN_WILDCARD;
    if (!parse_concatenated_input(search_str, &op->segments, &op->segment_count,
                                  encoding)) {
      free(search_str);
      free(replace_str);
      return 0;
    }
    op->search_bytes = NULL;
    op->search_len = 0;
  } else if (has_wildcard) {
    // Use text with wildcards parsing (no + concatenation)
    op->pattern_type = PATTERN_WILDCARD;

    // Extract text from quotes if needed
    char* text_to_parse = search_str;
    if (is_quoted_string(search_str)) {
      text_to_parse = extract_quoted_string(search_str);
      if (!text_to_parse) {
        free(search_str);
        free(replace_str);
        return 0;
      }
    }

    if (!parse_text_with_wildcards(text_to_parse, &op->segments, &op->segment_count,
                                   encoding)) {
      if (text_to_parse != search_str) free(text_to_parse);
      free(search_str);
      free(replace_str);
      return 0;
    }

    if (text_to_parse != search_str) free(text_to_parse);
    op->search_bytes = NULL;
    op->search_len = 0;
  } else {
    // Use literal matching
    op->pattern_type = PATTERN_LITERAL;
    op->segments = NULL;
    op->segment_count = 0;
    if (!parse_input(search_str, &op->search_bytes, &op->search_len,
                     encoding)) {
      free(search_str);
      free(replace_str);
      return 0;
    }
  }

  if (strlen(replace_str) == 0) {
    op->delete_mode = 1;
    op->replace_bytes = NULL;
    op->replace_len = 0;
  } else {
    op->delete_mode = 0;

    // Check if replace string also has concatenation
    int replace_has_concat = 0;
    in_quotes = 0;
    for (const char* p = replace_str; *p; p++) {
      if (*p == '"' || *p == '\'') in_quotes = !in_quotes;
      if (!in_quotes && *p == '+') {
        replace_has_concat = 1;
        break;
      }
    }

    if (replace_has_concat) {
      // Parse replace string with concatenation
      PatternSegment* replace_segments = NULL;
      int replace_segment_count = 0;
      if (!parse_concatenated_input(replace_str, &replace_segments,
                                    &replace_segment_count, encoding)) {
        if (op->search_bytes) free(op->search_bytes);
        if (op->segments) {
          for (int i = 0; i < op->segment_count; i++) {
            if (op->segments[i].bytes) free(op->segments[i].bytes);
          }
          free(op->segments);
        }
        free(search_str);
        free(replace_str);
        return 0;
      }

      // Concatenate all segments into single byte array
      size_t total_len = 0;
      for (int i = 0; i < replace_segment_count; i++) {
        if (!replace_segments[i].is_wildcard) {
          total_len += replace_segments[i].len;
        }
      }

      op->replace_bytes = (unsigned char*)malloc(total_len);
      if (!op->replace_bytes) {
        for (int i = 0; i < replace_segment_count; i++) {
          if (replace_segments[i].bytes) free(replace_segments[i].bytes);
        }
        free(replace_segments);
        if (op->search_bytes) free(op->search_bytes);
        if (op->segments) {
          for (int i = 0; i < op->segment_count; i++) {
            if (op->segments[i].bytes) free(op->segments[i].bytes);
          }
          free(op->segments);
        }
        free(search_str);
        free(replace_str);
        return 0;
      }

      size_t offset = 0;
      for (int i = 0; i < replace_segment_count; i++) {
        if (!replace_segments[i].is_wildcard) {
          memcpy(op->replace_bytes + offset, replace_segments[i].bytes,
                 replace_segments[i].len);
          offset += replace_segments[i].len;
        }
      }
      op->replace_len = total_len;

      // Free replace segments
      for (int i = 0; i < replace_segment_count; i++) {
        if (replace_segments[i].bytes) free(replace_segments[i].bytes);
      }
      free(replace_segments);
    } else {
      // Parse replace string normally
      if (!parse_input(replace_str, &op->replace_bytes, &op->replace_len,
                       encoding)) {
        if (op->search_bytes) free(op->search_bytes);
        if (op->segments) {
          for (int i = 0; i < op->segment_count; i++) {
            if (op->segments[i].bytes) free(op->segments[i].bytes);
          }
          free(op->segments);
        }
        free(search_str);
        free(replace_str);
        return 0;
      }
    }
  }

  // Check if replace string contains capture references (\0, \1, etc.)
  op->has_captures_in_replace = has_capture_references(replace_str);
  if (op->has_captures_in_replace) {
    // Save template without processing quotes - they'll be handled in build_replacement_with_captures
    op->replace_template = strdup(replace_str);
  } else {
    op->replace_template = NULL;
  }

  // Set ignore_case flag
  op->ignore_case = ignore_case;

  free(search_str);
  free(replace_str);
  return 1;
}

void free_operation(Operation* op) {
  if (op->search_bytes) free(op->search_bytes);
  if (op->replace_bytes) free(op->replace_bytes);
  if (op->replace_template) free(op->replace_template);

  // Free pattern segments
  if (op->segments) {
    for (int i = 0; i < op->segment_count; i++) {
      if (op->segments[i].bytes) free(op->segments[i].bytes);
    }
    free(op->segments);
  }
}

int main(int argc, char* argv[]) {
  // Enable Windows virtual terminal processing for ANSI colors
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);

  // Check for -d debug flag and -t test flag
  int debug_mode = 0;
  int dry_run_mode = 0;
  int arg_offset = 0;

  // Check first two arguments for flags (can be combined)
  for (int i = 1; i < argc && i <= 2; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      debug_mode = 1;
      arg_offset++;
    } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
      dry_run_mode = 1;
      arg_offset++;
    } else {
      break;  // Not a flag, stop checking
    }
  }

  if (debug_mode) {
    fprintf(stderr, COLOR_CYAN "=== DEBUG MODE ===" COLOR_RESET "\n\n");

    // Print command line arguments
    fprintf(stderr, COLOR_YELLOW "Command line arguments:" COLOR_RESET "\n");
    fprintf(stderr, "  argc = %d\n", argc);
    for (int i = 0; i < argc; i++) {
      fprintf(stderr, "  argv[%d] = '%s'\n", i, argv[i]);
    }
    fprintf(stderr, "\n");
  }

  if (dry_run_mode) {
    fprintf(stderr, COLOR_CYAN "=== TEST MODE (Preview) ===" COLOR_RESET "\n");
    fprintf(stderr, COLOR_YELLOW "No changes will be made to files" COLOR_RESET "\n\n");
  }

  if (argc < 2 + arg_offset) {
    fprintf(stderr,
            "\n" COLOR_YELLOW "REPLACER " COLOR_YELLOW "v26.0424 - " COLOR_CYAN
            "File content search and replace utility with encoding "
            "conversion" COLOR_RESET "\n");
    fprintf(stderr,
            "                    Утилита поиска и замены содержимого файлов с "
            "конвертацией кодировок\n");
    fprintf(stderr, "(By BoyNG - \nVyacheslav Burnosov)\n\n\n");
    fprintf(stderr,
            COLOR_YELLOW "Usage / Использование:" COLOR_RESET " %s " COLOR_GREEN
                         "[-d] [-t] [encoding:]<input>[:[encoding][:output]]" COLOR_RESET
                         " [operations...]\n\n",
            argv[0]);

    fprintf(stderr,
            COLOR_YELLOW "File specification / Формат файла:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "file.bin" COLOR_RESET
                    "                    - " COLOR_CYAN
                    "input file, output will be file_OUT.bin" COLOR_RESET "\n");
    fprintf(stderr,
            "                                входной файл, выход будет "
            "file_OUT.bin\n");
    fprintf(stderr, "  " COLOR_GREEN "file.bin:out.bin" COLOR_RESET
                    "            - " COLOR_CYAN
                    "input file with custom output name" COLOR_RESET "\n");
    fprintf(stderr,
            "                                входной файл с указанием имени "
            "выходного\n");
    fprintf(stderr, "  " COLOR_GREEN "file.bin:-" COLOR_RESET
                    "                  - " COLOR_CYAN
                    "output to stdout (like 'type' command)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                вывод в stdout (как команда "
            "'type')\n");
    fprintf(stderr, "  " COLOR_GREEN "file.txt:utf" COLOR_RESET
                    "                - " COLOR_CYAN
                    "convert file to UTF-8 encoding" COLOR_RESET "\n");
    fprintf(stderr,
            "                                конвертировать файл в кодировку "
            "UTF-8\n");
    fprintf(stderr, "  " COLOR_GREEN "file.txt:-:utf" COLOR_RESET
                    "              - " COLOR_CYAN
                    "output to stdout with UTF-8 conversion" COLOR_RESET "\n");
    fprintf(stderr,
            "                                вывод в stdout с конвертацией в "
            "UTF-8\n");
    fprintf(stderr, "  " COLOR_GREEN "win:file.txt" COLOR_RESET
                    "                - " COLOR_CYAN
                    "read file as Windows-1251" COLOR_RESET "\n");
    fprintf(stderr,
            "                                читать файл как Windows-1251\n");
    fprintf(stderr, "  " COLOR_GREEN "win:file.txt:utf" COLOR_RESET
                    "            - " COLOR_CYAN
                    "convert Windows-1251 to UTF-8" COLOR_RESET "\n");
    fprintf(stderr,
            "                                конвертировать Windows-1251 в "
            "UTF-8\n");
    fprintf(stderr, "  " COLOR_GREEN "win:file.txt:-" COLOR_RESET
                    "              - " COLOR_CYAN
                    "output Windows-1251 file to stdout" COLOR_RESET "\n");
    fprintf(
        stderr,
        "                                вывод файла Windows-1251 в stdout\n");
    fprintf(stderr, "  " COLOR_GREEN "win:file.txt:-:utf" COLOR_RESET
                    "          - " COLOR_CYAN
                    "convert and output to stdout" COLOR_RESET "\n");
    fprintf(stderr,
            "                                конвертация и вывод в stdout\n");
    fprintf(stderr, "  " COLOR_GREEN "win:file.txt:utf:out.txt" COLOR_RESET
                    "    - " COLOR_CYAN
                    "convert with custom output name" COLOR_RESET "\n");
    fprintf(stderr,
            "                                конвертация с указанием имени "
            "выхода\n");
    fprintf(stderr, "  " COLOR_GREEN "-" COLOR_RESET
                    "                           - " COLOR_CYAN
                    "read from stdin, write to stdout" COLOR_RESET "\n");
    fprintf(
        stderr,
        "                                читать из stdin, писать в stdout\n");
    fprintf(stderr, "  " COLOR_GREEN "win:-:utf" COLOR_RESET
                    "                   - " COLOR_CYAN
                    "stdin/stdout with encoding conversion" COLOR_RESET "\n");
    fprintf(stderr,
            "                                stdin/stdout с конвертацией "
            "кодировки\n");
    fprintf(stderr, "\n");
    getchar();
    fprintf(stderr, "\n" COLOR_YELLOW
                    "Operation format / Формат операций:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "search:replace" COLOR_RESET
                    "              - " COLOR_CYAN
                    "search and replace" COLOR_RESET " / найти и заменить\n");
    fprintf(stderr, "  " COLOR_GREEN "search:" COLOR_RESET
                    "                     - " COLOR_CYAN
                    "delete (empty replace)" COLOR_RESET
                    " / удалить (пустая замена)\n");
    fprintf(stderr,
            "  " COLOR_GREEN "search:replace:encoding" COLOR_RESET
            "     - " COLOR_CYAN
            "with specific encoding for this operation" COLOR_RESET "\n");
    fprintf(
        stderr,
        "                                с указанием кодировки для операции\n");

    fprintf(stderr,
            "\n" COLOR_YELLOW
            "Search/Replace formats / Форматы поиска/замены:" COLOR_RESET "\n");
    fprintf(stderr,
            "  Hex: " COLOR_GREEN "0xFFAA" COLOR_RESET " or " COLOR_GREEN
            "$FFAA" COLOR_RESET " / или " COLOR_GREEN "$FFAA" COLOR_RESET "\n");
    fprintf(stderr, "  Text: " COLOR_GREEN "\"hello\"" COLOR_RESET
                    " or plain text / Текст: " COLOR_GREEN
                    "\"привет\"" COLOR_RESET " или просто текст\n");

    fprintf(stderr, "\n" COLOR_YELLOW "Encodings / Кодировки:" COLOR_RESET
                    " " COLOR_GREEN "win" COLOR_RESET " (CP1251), " COLOR_GREEN
                    "dos" COLOR_RESET " (CP866), " COLOR_GREEN "koi" COLOR_RESET
                    " (KOI8-R), " COLOR_GREEN "utf" COLOR_RESET
                    " (UTF-8, default/по умолчанию)\n");

    fprintf(stderr, "\n" COLOR_YELLOW "Debug mode / Режим отладки:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "-d" COLOR_RESET
                    "                          - " COLOR_CYAN
                    "show detailed debug information" COLOR_RESET "\n");
    fprintf(stderr,
            "                                показать подробную отладочную информацию\n");
    fprintf(stderr, "  " COLOR_CYAN "Note:" COLOR_RESET
                    " Must be first argument. Shows arguments, file spec, operations, sizes.\n");
    fprintf(stderr, "  " COLOR_CYAN "Примечание:" COLOR_RESET
                    " Должен быть первым аргументом. Показывает аргументы, файлы, операции, размеры.\n");

    fprintf(stderr, "\n" COLOR_YELLOW
                    "Wildcards / Подстановочные символы:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "\\." COLOR_RESET
                    "                          - " COLOR_CYAN
                    "any single byte" COLOR_RESET " / любой один байт\n");
    fprintf(stderr,
            "  " COLOR_GREEN "\\*" COLOR_RESET
            "                          - " COLOR_CYAN
            "zero or more bytes" COLOR_RESET " / ноль или более байтов\n");
    fprintf(stderr, "  " COLOR_GREEN "\\?" COLOR_RESET
                    "                          - " COLOR_CYAN
                    "optional byte (zero or one)" COLOR_RESET
                    " / необязательный байт (ноль или один)\n");
    fprintf(stderr, "  " COLOR_CYAN "Note:" COLOR_RESET
                    " Use backslash to escape wildcards. Without backslash "
                    "they are literal.\n");
    fprintf(stderr, "  " COLOR_CYAN "Примечание:" COLOR_RESET
                    " Используйте обратный слэш для wildcards. Без слэша - "
                    "литеральные символы.\n");

    fprintf(stderr,
            "\n" COLOR_YELLOW "Concatenation / Конкатенация:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "+" COLOR_RESET
                    "                           - " COLOR_CYAN
                    "join operator" COLOR_RESET " / оператор объединения\n");
    fprintf(stderr, "  " COLOR_GREEN "\"text\"+0x0A+\"more\"" COLOR_RESET
                    "          - " COLOR_CYAN
                    "join text, hex, and text" COLOR_RESET "\n");
    fprintf(stderr,
            "                                объединить текст, hex и текст\n");
    fprintf(stderr, "  " COLOR_GREEN "\"<tag>\"+\\*+\"</tag>\"" COLOR_RESET
                    "         - " COLOR_CYAN
                    "match anything between tags" COLOR_RESET "\n");
    fprintf(stderr,
            "                                найти что угодно между тегами\n");
    fprintf(stderr, "  " COLOR_GREEN "\"colo\"+\\?+\"r\"" COLOR_RESET
                    "               - " COLOR_CYAN
                    "match color, colour, colo?r" COLOR_RESET "\n");
    fprintf(stderr,
            "                                найти color, colour, colo?r\n");

    fprintf(stderr, "\n" COLOR_YELLOW "Capture groups / Группы захвата:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "{pattern}" COLOR_RESET
                    "                   - " COLOR_CYAN
                    "numbered capture group (max 9)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                нумерованная группа захвата (максимум 9)\n");
    fprintf(stderr, "  " COLOR_GREEN "{name=pattern}" COLOR_RESET
                    "              - " COLOR_CYAN
                    "named capture group (unlimited)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                именованная группа захвата (неограниченно)\n");
    fprintf(stderr, "  " COLOR_GREEN "\\0" COLOR_RESET
                    "                          - " COLOR_CYAN
                    "reference entire match in replacement" COLOR_RESET "\n");
    fprintf(stderr,
            "                                ссылка на всё вхождение в замене\n");
    fprintf(stderr, "  " COLOR_GREEN "\\1" COLOR_RESET " to " COLOR_GREEN "\\9" COLOR_RESET
                    "                     - " COLOR_CYAN
                    "reference numbered groups in replacement" COLOR_RESET "\n");
    fprintf(stderr,
            "                                ссылка на нумерованные группы в замене\n");
    fprintf(stderr, "  " COLOR_GREEN "{name}" COLOR_RESET
                    "                      - " COLOR_CYAN
                    "reference named group in replacement" COLOR_RESET "\n");
    fprintf(stderr,
            "                                ссылка на именованную группу в замене\n");

    fprintf(stderr, "\n" COLOR_YELLOW "Flags / Флаги:" COLOR_RESET "\n");
    fprintf(stderr, "  " COLOR_GREEN "-d" COLOR_RESET
                    "                          - " COLOR_CYAN
                    "debug mode (show detailed processing info)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                режим отладки (показать детальную информацию)\n");
    fprintf(stderr, "  " COLOR_GREEN "-t, --test" COLOR_RESET
                    "                    - " COLOR_CYAN
                    "test mode (show changes without modifying)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                тестовый режим (показать изменения без модификации)\n");
    fprintf(stderr, "  " COLOR_GREEN "/i" COLOR_RESET
                    "                          - " COLOR_CYAN
                    "case-insensitive search (add at end of operation)" COLOR_RESET "\n");
    fprintf(stderr,
            "                                поиск без учёта регистра (добавить в конец операции)\n");
    fprintf(stderr, "  " COLOR_GREEN "'hello':'HELLO'/i" COLOR_RESET
                    "         - " COLOR_CYAN
                    "match hello, Hello, HELLO, etc." COLOR_RESET "\n");
    fprintf(stderr,
            "                                найти hello, Hello, HELLO и т.д.\n");

    fprintf(stderr, "\n" COLOR_YELLOW "Examples / Примеры:" COLOR_RESET "\n");
    fprintf(stderr, "  %s file.bin 0xAA:0xBB 0xCC:0xDD\n", argv[0]);
    fprintf(stderr, "  %s file.bin:out.bin 0xAA:0xBB \"old\":\"new\":win\n",
            argv[0]);
    fprintf(stderr, "  %s file.txt:-\n", argv[0]);
    fprintf(stderr, "  %s file.txt:- 0xAA:0xBB\n", argv[0]);
    fprintf(stderr, "  %s win:file.txt:-:utf\n", argv[0]);
    fprintf(stderr, "  %s win:file.txt:utf\n", argv[0]);
    fprintf(stderr, "  %s win:file.txt:utf \"тест\":\"test\"\n", argv[0]);
    fprintf(stderr, "  %s file.txt \"тест\":\"test\":win \"hello\":\n",
            argv[0]);
    fprintf(stderr, "  %s - 0xAA:0xBB < in.bin > out.bin\n", argv[0]);
    fprintf(stderr, "  type in.bin | %s - 0xAA:0xBB > out.bin\n", argv[0]);
    fprintf(stderr, "  type file.txt | %s - \"old\":\"new\"\n", argv[0]);
    fprintf(stderr, "  %s - \"old\":\"new\" < file.txt\n", argv[0]);
    fprintf(stderr, "  %s - \"old\":\"new\" < file.txt 2>nul\n", argv[0]);
    fprintf(stderr, "  %s win:-:utf < input.txt > output.txt\n", argv[0]);
    fprintf(
        stderr,
        "  %s test.html \"<title>\"+\\*+\"</title>\":\"<title>New</title>\"\n",
        argv[0]);
    fprintf(stderr, "  %s test.txt \"colo\"+\\?+\"r\":\"COLOR\"\n", argv[0]);
    fprintf(stderr, "  %s test.bin 0xAA+\\.+0xBB:0xFF\n", argv[0]);
    fprintf(stderr, "\n" COLOR_YELLOW "Capture group examples / Примеры групп захвата:" COLOR_RESET "\n");
    fprintf(stderr, "  %s file.txt \"'error':'[\\0]'\"\n", argv[0]);
    fprintf(stderr, "    " COLOR_CYAN "Wrap 'error' in brackets: error -> [error]" COLOR_RESET "\n");
    fprintf(stderr, "    " COLOR_CYAN "Обернуть 'error' в скобки: error -> [error]" COLOR_RESET "\n");
    fprintf(stderr, "  %s file.txt \"'['+{*}+'] '+{*}:'\\2 (\\1)'\"\n", argv[0]);
    fprintf(stderr, "    " COLOR_CYAN "Swap parts: [ERROR] File not found -> File not found (ERROR)" COLOR_RESET "\n");
    fprintf(stderr, "    " COLOR_CYAN "Поменять части местами: [ERROR] File not found -> File not found (ERROR)" COLOR_RESET "\n");
    fprintf(stderr, "  %s file.txt \"'Name: '+{name=*}+', Age: '+{age=*}:'{age}+' years, name='+{name}'\"\n", argv[0]);
    fprintf(stderr, "    " COLOR_CYAN "Named groups: Name: John, Age: 30 -> 30 years, name=John" COLOR_RESET "\n");
    fprintf(stderr, "    " COLOR_CYAN "Именованные группы: Name: John, Age: 30 -> 30 years, name=John" COLOR_RESET "\n");
    fprintf(stderr, "  %s file.txt \"'hello':'HELLO'/i\"\n", argv[0]);
    fprintf(stderr, "    " COLOR_CYAN "Case-insensitive: hello, Hello, HELLO -> HELLO" COLOR_RESET "\n");
    fprintf(stderr, "    " COLOR_CYAN "Без учёта регистра: hello, Hello, HELLO -> HELLO" COLOR_RESET "\n");
    getchar();
    return 1;
  }

  const char* file_spec = argv[1 + arg_offset];

  char* input_file = NULL;
  char* output_file = NULL;
  Encoding input_enc = ENCODING_UTF8;
  Encoding output_enc = ENCODING_UTF8;
  int use_stdio = 0;

  if (!parse_file_spec(file_spec, &input_file, &output_file, &input_enc,
                       &output_enc, &use_stdio)) {
    fprintf(stderr, "Error: Invalid file specification\n");
    return 1;
  }

  if (debug_mode) {
    fprintf(stderr, COLOR_YELLOW "File specification:" COLOR_RESET "\n");
    fprintf(stderr, "  Input file:     %s\n", input_file ? input_file : "(stdin)");
    fprintf(stderr, "  Output file:    %s\n", output_file ? output_file : "(stdout)");
    fprintf(stderr, "  Input encoding: %s\n",
            input_enc == ENCODING_UTF8 ? "UTF-8" :
            input_enc == ENCODING_WIN1251 ? "WIN-1251" :
            input_enc == ENCODING_DOS866 ? "DOS-866" :
            input_enc == ENCODING_KOI8R ? "KOI8-R" : "Unknown");
    fprintf(stderr, "  Output encoding: %s\n",
            output_enc == ENCODING_UTF8 ? "UTF-8" :
            output_enc == ENCODING_WIN1251 ? "WIN-1251" :
            output_enc == ENCODING_DOS866 ? "DOS-866" :
            output_enc == ENCODING_KOI8R ? "KOI8-R" : "Unknown");
    fprintf(stderr, "  Use stdio:      %s\n\n", use_stdio ? "yes" : "no");
  }

  if (!use_stdio && !output_file) {
    output_file = create_output_filename(input_file);
    if (!output_file) {
      fprintf(stderr, "Error: Failed to create output filename\n");
      free(input_file);
      return 1;
    }
    if (debug_mode) {
      fprintf(stderr, COLOR_YELLOW "Auto-generated output filename:" COLOR_RESET " %s\n\n", output_file);
    }
  }

  int op_count = argc - 2 - arg_offset;
  Operation* operations = NULL;

  if (op_count > 0) {
    operations = (Operation*)calloc(op_count, sizeof(Operation));
    if (!operations) {
      fprintf(stderr, "Error: Memory allocation failed\n");
      if (input_file) free(input_file);
      if (output_file) free(output_file);
      return 1;
    }

    for (int i = 0; i < op_count; i++) {
      if (!parse_operation(argv[i + 2 + arg_offset], &operations[i], ENCODING_UTF8)) {
        for (int j = 0; j < i; j++) {
          free_operation(&operations[j]);
        }
        free(operations);
        if (input_file) free(input_file);
        if (output_file) free(output_file);
        return 1;
      }
    }

    if (debug_mode) {
      fprintf(stderr, COLOR_YELLOW "Operations (%d):" COLOR_RESET "\n", op_count);
      for (int i = 0; i < op_count; i++) {
        fprintf(stderr, "  [%d] ", i + 1);
        if (operations[i].pattern_type == PATTERN_LITERAL) {
          fprintf(stderr, "LITERAL: ");
          for (size_t j = 0; j < operations[i].search_len && j < 20; j++) {
            fprintf(stderr, "%02X ", operations[i].search_bytes[j]);
          }
          if (operations[i].search_len > 20) fprintf(stderr, "...");
        } else {
          fprintf(stderr, "WILDCARD: %d segments", operations[i].segment_count);
          if (operations[i].defined_group_count > 0) {
            fprintf(stderr, ", %d named groups (", operations[i].defined_group_count);
            for (int g = 0; g < operations[i].defined_group_count; g++) {
              fprintf(stderr, "%s%s", g > 0 ? ", " : "", operations[i].defined_groups[g].name);
            }
            fprintf(stderr, ")");
          }
        }
        fprintf(stderr, " -> ");
        if (operations[i].delete_mode) {
          fprintf(stderr, "DELETE");
        } else if (operations[i].has_captures_in_replace) {
          fprintf(stderr, "REPLACE with captures");
          if (operations[i].replace_template) {
            fprintf(stderr, "\n      Template: '%s'", operations[i].replace_template);
          }
        } else {
          fprintf(stderr, "REPLACE: ");
          for (size_t j = 0; j < operations[i].replace_len && j < 20; j++) {
            fprintf(stderr, "%02X ", operations[i].replace_bytes[j]);
          }
          if (operations[i].replace_len > 20) fprintf(stderr, "...");
        }
        if (operations[i].ignore_case) {
          fprintf(stderr, " " COLOR_CYAN "[/i case-insensitive]" COLOR_RESET);
        }
        fprintf(stderr, "\n");

        // Show detailed group information
        if (operations[i].defined_group_count > 0) {
          fprintf(stderr, "      Defined groups:\n");
          for (int g = 0; g < operations[i].defined_group_count; g++) {
            fprintf(stderr, "        [%d] '%s' -> capture index %d\n",
                    g, operations[i].defined_groups[g].name, operations[i].defined_groups[g].index);
          }
        }
      }
      fprintf(stderr, "\n");
    }
  }

  unsigned char* buffer = NULL;
  size_t buffer_size = 0;

  if (use_stdio && !input_file) {
    // Read from stdin
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);

    size_t capacity = 4096;
    buffer = (unsigned char*)malloc(capacity);
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
        unsigned char* new_buffer = (unsigned char*)realloc(buffer, capacity);
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
    FILE* fin = fopen(input_file, "rb");
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

    buffer = (unsigned char*)malloc(buffer_size);
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

    if (debug_mode) {
      fprintf(stderr, COLOR_YELLOW "Input file size:" COLOR_RESET " %zu bytes\n\n", buffer_size);
    }

    // Set stdout to binary mode if outputting to stdout
    if (use_stdio) {
      _setmode(_fileno(stdout), _O_BINARY);
    }
  }

  unsigned char* processed = NULL;
  size_t processed_size = 0;

  // Apply input encoding conversion if needed
  if (input_enc != ENCODING_UTF8) {
    if (!apply_encoding_conversion(buffer, buffer_size, input_enc,
                                   ENCODING_UTF8, &processed,
                                   &processed_size)) {
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
  unsigned char* result = NULL;
  size_t result_size = 0;
  int total_replacements = 0;

  if (op_count > 0) {
    if (dry_run_mode) {
      // Dry-run mode: preview changes without modifying
      int total_matches = 0;
      if (!preview_operations(buffer, buffer_size, operations, op_count, &total_matches)) {
        fprintf(stderr, "Error: Failed to preview operations\n");
        free(buffer);
        for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
        free(operations);
        if (input_file) free(input_file);
        if (output_file) free(output_file);
        return 1;
      }

      fprintf(stderr, COLOR_CYAN "=== PREVIEW SUMMARY ===" COLOR_RESET "\n");
      fprintf(stderr, COLOR_YELLOW "Total matches: %d" COLOR_RESET "\n", total_matches);
      fprintf(stderr, COLOR_GREEN "No changes were made (test mode)" COLOR_RESET "\n");

      free(buffer);
      for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
      free(operations);
      if (input_file) free(input_file);
      if (output_file) free(output_file);
      return 0;
    }

    if (!apply_operations(buffer, buffer_size, operations, op_count, &result,
                          &result_size, &total_replacements)) {
      fprintf(stderr, "Error: Failed to apply operations\n");
      free(buffer);
      for (int i = 0; i < op_count; i++) free_operation(&operations[i]);
      free(operations);
      if (input_file) free(input_file);
      if (output_file) free(output_file);
      return 1;
    }
    free(buffer);

    if (debug_mode) {
      fprintf(stderr, COLOR_YELLOW "Processing results:" COLOR_RESET "\n");
      fprintf(stderr, "  Total replacements: %d\n", total_replacements);
      fprintf(stderr, "  Input size:         %zu bytes\n", buffer_size);
      fprintf(stderr, "  Output size:        %zu bytes\n", result_size);
      fprintf(stderr, "  Size change:        %+zd bytes\n\n" COLOR_RED, (ssize_t)result_size - (ssize_t)buffer_size);
    }
  } else {
    result = buffer;
    result_size = buffer_size;
  }

  // Apply output encoding conversion if needed
  unsigned char* final_result = NULL;
  size_t final_size = 0;

  if (output_enc != ENCODING_UTF8) {
    if (!apply_encoding_conversion(result, result_size, ENCODING_UTF8,
                                   output_enc, &final_result, &final_size)) {
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
    FILE* fout = fopen(output_file, "wb");
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

  if (debug_mode) {
    fprintf(stderr, COLOR_GREEN "\n=== DEBUG MODE COMPLETE ===" COLOR_RESET "\n");
  }

  return 0;
}
