# REPLACER - TODO List

## Planned Features / Планируемые функции

### 1. Capture groups / Группы захвата
**Priority: High / Приоритет: Высокий**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
# \0 for entire match (COMPLETED)
replacer.exe "file.txt":- "'error':'\0 found'"
replacer.exe "file.txt":- "'Version: '+*:'Found: \0'"

# \1-\9 for numbered captures (COMPLETED)
replacer.exe "file.txt":- "'['+{*}+'] '+{*}:'\2 (\1)'"
# Input: [ERROR] File not found
# Output: File not found (ERROR)

# Named captures with {name=pattern} (COMPLETED)
replacer.exe "file.txt":- "'Name: '+{name=*}+', Age: '+{age=*}:{age}+' years, name='+{name}"
# Input: Name: John, Age: 30
# Output: 30 years, name=John
```

**English:** 
- `\0` captures entire match (COMPLETED)
- `\1-\9` for numbered captures (COMPLETED - max 9 groups)
- `{name=pattern}` for named captures (COMPLETED - unlimited)
- `{name}` references work outside quotes only
- Swap parts, reuse captured segments, mix numbered and named groups

**Русский:**
- `\0` захватывает всё вхождение (ЗАВЕРШЕНО)
- `\1-\9` для нумерованных захватов (ЗАВЕРШЕНО - максимум 9 групп)
- `{name=pattern}` для именованных захватов (ЗАВЕРШЕНО - неограниченно)
- Ссылки `{name}` работают только вне кавычек
- Переставить части, переиспользовать захваченные сегменты, смешивать нумерованные и именованные группы

**Implementation notes:**
- `\0` for entire match: COMPLETED in version 26.0420
- Numbered captures `\1-\9`: COMPLETED in version 26.0421
- Named captures `{name=pattern}`: COMPLETED in version 26.0421
- Syntax uses `=` instead of `:` to avoid conflict with search:replace separator
- Bilingual error messages for all capture group errors
- Debug mode `-d` shows capture group information

---

### 1a. Debug mode / Режим отладки
**Priority: High / Приоритет: Высокий**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
replacer.exe -d file.txt:- "'pattern':'replacement'"
```

**English:**
- Shows command line arguments (argc, argv)
- Shows parsed file specification (files, encodings)
- Shows operations details (pattern type, segments, groups)
- Shows input file size
- Shows processing statistics (replacements, size changes)

**Русский:**
- Показывает аргументы командной строки (argc, argv)
- Показывает распарсенную спецификацию файла (файлы, кодировки)
- Показывает детали операций (тип паттерна, сегменты, группы)
- Показывает размер входного файла
- Показывает статистику обработки (замены, изменения размера)

**Implementation notes:**
- `-d` flag must be first argument
- Colorized output for better readability
- Completed in version 26.0421

---

### 2. Case-insensitive search / Поиск без учёта регистра
**Priority: High / Приоритет: Высокий**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
replacer file.txt "'hello':'HELLO'/i"
```
**English:** Find hello, Hello, HELLO, hElLo - replace with HELLO (flag /i ignores case)  
**Русский:** Найти hello, Hello, HELLO, hElLo - заменить на HELLO (флаг /i игнорирует регистр)

**Implementation notes:**
- Add `/i` flag at end of operation string (after encoding or replacement)
- Modified `match_pattern()` to accept `ignore_case` parameter
- Modified `match_pattern_with_captures()` to accept `ignore_case` parameter
- Use `tolower()` for byte-by-byte comparison when flag is set
- Works with both literal patterns and wildcard patterns
- Works with capture groups
- Debug mode shows `[/i case-insensitive]` flag
- Completed in version 26.0422

---

### 3. Counters and variables / Счётчики и переменные
**Priority: Medium / Приоритет: Средний**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
replacer file.txt "'TODO':'TODO-'+{#}"
replacer file.txt "'TODO':'Task '+{#:5}"
replacer file.txt "'TODO':'Task '+{#:001}"
replacer file.txt "'TODO':'Task '+{#:03-05}"
replacer file.txt "'TODO':'Task '+{#:0x01}"
replacer file.txt "'TODO':'Task '+{#:A}"
replacer file.txt "'TODO':'Task '+{#:a}"
```
**English:** Auto-increment counter with various formats  
**Русский:** Автоинкремент счётчика с различными форматами

**Syntax:**
- `{#}` - simple counter (1, 2, 3...)
- `{#:5}` - start from 5 (5, 6, 7...)
- `{#:001}` - zero-padded (001, 002, 003...)
- `{#:03-05}` - range from 03 to 05, then stop
- `{#:0x01}` - hex format (0x01, 0x02, 0x03...)
- `{#:A}` - uppercase alphabetic (A, B, C... Z, AA, AB...)
- `{#:a}` - lowercase alphabetic (a, b, c... z, aa, ab...)

**Implementation notes:**
- Counter syntax `{#...}` works outside single quotes (like capture groups)
- Example: `"'TODO':'Task '+{#}"` not `"'TODO':'Task {#}'"`
- Counter increments with each replacement
- Range format `{#:03-05}` stops after reaching end value
- Alphabetic format continues beyond Z: A-Z, AA-ZZ, AAA-ZZZ...
- Works together with capture groups: `"'error':'['+{#}+'] \0'"`
- Completed in version 26.0425

---

### 4. Byte ranges / Диапазоны байтов
**Priority: MERGED INTO #12 / Приоритет: ОБЪЕДИНЕНО В #12**

**Note:** This feature has been merged into feature #12 (Range-based operations).  
**Примечание:** Эта функция объединена с функцией #12 (Операции по диапазону).

---

### 5. Multiple files (glob patterns) / Множественные файлы
**Priority: Medium / Приоритет: Средний**

```bash
replacer *.txt "old":"new"
replacer src/**/*.c "TODO":"DONE"
```
**English:** Process all .txt files, or recursively process all .c files in src/  
**Русский:** Обработать все .txt файлы, или рекурсивно обработать все .c файлы в src/

**Implementation notes:**
- Parse glob patterns using Windows FindFirstFile/FindNextFile
- Support `*` (any chars), `**` (recursive), `?` (single char)
- Process each matched file with same operations
- Report summary: files processed, total replacements

---

### 6. Backup mode / Режим резервного копирования
**Priority: Low / Приоритет: Низкий**

```bash
replacer -b file.txt "old":"new"
replacer --backup file.txt "old":"new"
```
**English:** Create file.txt.bak before modifying  
**Русский:** Создать file.txt.bak перед изменением

**Implementation notes:**
- Add `-b` or `--backup` flag
- Copy original file to .bak before writing output
- Handle backup file conflicts (overwrite or error)

---

### 7. Dry-run / Preview mode / Режим предварительного просмотра
**Priority: High / Приоритет: Высокий**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
replacer -t file.txt "'old':'new'"
replacer --test file.txt "'old':'new'"
```
**English:** Show what would be replaced without modifying file  
**Русский:** Показать что будет заменено, но не менять файл

**Output format:**
```
Operation 1: Literal pattern [case-insensitive]
  Line 5: "This is old text"
  Line 12: "old value"
  Matches found: 2
---
Total matches: 2
No changes were made (test mode)
```

**Implementation notes:**
- Add `-t` or `--test` flag as first argument
- Created `preview_operations()` function to find matches without modifying
- Shows line numbers and context for each match
- Shows operation type (literal/wildcard) and flags (case-insensitive)
- Displays summary of total matches
- File is never modified in test mode
- Completed in version 26.0424
```
Line 5: "This is old text" -> "This is new text"
Line 12: "old value" -> "new value"
---
Total: 2 replacements would be made
```

**Implementation notes:**
- Add `-n` or `--dry-run` flag
- Perform matching but don't write output file
- Display line numbers and context for each match
- Show summary of what would change

---

### 8. Verbose statistics / Подробная статистика
**Priority: Low / Приоритет: Низкий**

```bash
replacer -v file.txt "old":"new"
replacer --verbose file.txt "old":"new"
```
**English:** Show positions, context, and detailed statistics  
**Русский:** Показать позиции замен, контекст и подробную статистику

**Output format:**
```
Match 1 at offset 0x0042: "old" -> "new"
  Context: "This is old text here"
Match 2 at offset 0x0128: "old" -> "new"
  Context: "Another old value"
---
Total: 2 replacements made
Input size: 1024 bytes
Output size: 1024 bytes
```

---

### 9. Limit replacements / Ограничение замен
**Priority: High / Приоритет: Высокий**

```bash
replacer file.txt "old":"new":4        # replace only 4th occurrence
replacer file.txt "old":"new":1-3      # replace occurrences 1 to 3
replacer file.txt "old":"new":3-5      # replace occurrences 3 to 5
replacer file.txt "old":"new":-3       # replace last 3 occurrences
replacer file.txt "old":"new":5-       # replace from 5th to end
```

**English:**
- `4` - replace only 4th occurrence
- `1-3` - replace first 3 occurrences (1st, 2nd, 3rd)
- `3-5` - replace occurrences 3 to 5
- `-3` - replace last 3 occurrences
- `5-` - replace from 5th occurrence to end

**Русский:**
- `4` - заменить только 4-е вхождение
- `1-3` - заменить первые 3 вхождения (1-е, 2-е, 3-е)
- `3-5` - заменить вхождения с 3 по 5
- `-3` - заменить последние 3 вхождения
- `5-` - заменить с 5-го вхождения до конца

**Syntax format:**
```
search:replace:range[:encoding]
```

**Implementation notes:**
- Parse range syntax after second colon
- Support single number: `4`
- Support range: `3-5`, `1-3`
- Support open-ended: `5-` (from 5 to end), `-3` (last 3)
- Count matches during first pass for `-3` syntax
- Apply replacements only to matches in range

---

### 10. Negative patterns (NOT) / Негативные паттерны
**Priority: Low / Приоритет: Низкий**

```bash
replacer file.txt !"error"+\*+0x0A:
```
**English:** Delete all lines EXCEPT those containing "error"  
**Русский:** Удалить все строки КРОМЕ тех что содержат "error"

**Implementation notes:**
- Add `!` prefix for negative matching
- Invert match logic
- Useful for filtering/keeping specific patterns

---

## Additional Feature Ideas / Дополнительные идеи

### 11. Binary/Text mode auto-detection / Автоопределение бинарного/текстового режима
**Priority: Low / Приоритет: Низкий**

```bash
replacer -a file.dat "old":"new"  # auto-detect and handle accordingly
```
**English:** Automatically detect if file is binary or text and optimize operations  
**Русский:** Автоматически определить бинарный или текстовый файл и оптимизировать операции

---

### 12. Range-based operations / Операции по диапазону
**Priority: High / Приоритет: Высокий**

```bash
# Byte ranges (default - no prefix)
replacer -r 100-200 file.bin 0xAA:0xBB        # bytes 100-200
replacer -r 0x100-0x200 file.bin 0xAA:0xBB    # bytes 0x100-0x200 (hex)
replacer -r 100 file.bin 0xAA:0xBB            # only byte 100
replacer -r 100- file.bin 0xAA:0xBB           # from byte 100 to end
replacer -r -200 file.bin 0xAA:0xBB           # from start to byte 200
replacer -r 1K-2K file.bin 0xAA:0xBB          # 1KB-2KB (1024-2048)
replacer -r 1M-2M file.bin 0xAA:0xBB          # 1MB-2MB

# Line ranges (with L or l prefix)
replacer -r L10-20 file.txt "old":"new"       # lines 10-20
replacer -r l5 file.txt "old":"new"           # only line 5
replacer -r L1-3 file.txt "old":"new"         # first 3 lines
replacer -r l4- file.txt "old":"new"          # from line 4 to end
replacer -r L-10 file.txt "old":"new"         # from start to line 10

# Multiple ranges (comma-separated)
replacer -r 100-200,500-600,1000 file.bin 0xAA:0xBB       # bytes
replacer -r L1-5,L10-15,L20 file.txt "old":"new"          # lines
replacer -r 1K-2K,5K-6K file.bin 0xAA:0xBB                # with suffixes
```

**English:**
- No prefix = byte offsets (decimal or hex with 0x)
- Prefix `L` or `l` = line numbers
- Suffixes: `K` (1024), `M` (1024*1024), `G` (1024*1024*1024)
- Comma for multiple ranges
- Limit replacements to specific byte offset or line number ranges

**Русский:**
- Без префикса = байтовые смещения (decimal или hex с 0x)
- Префикс `L` или `l` = номера строк
- Суффиксы: `K` (1024), `M` (1024*1024), `G` (1024*1024*1024)
- Запятая для множественных диапазонов
- Ограничить замены определённым диапазоном байтов или строк

**Implementation notes:**
- Parse `-r` flag and range specification
- Support decimal, hex (0x prefix), and size suffixes (K/M/G)
- Detect line mode by L/l prefix
- For line mode: scan file first to build line offset table
- For byte mode: direct offset comparison
- Support open-ended ranges: `100-`, `-200`
- Support multiple ranges with comma separator
- Only perform replacements within specified ranges

---

### 13. Regular expression support / Поддержка регулярных выражений
**Priority: Low / Приоритет: Низкий**

```bash
replacer file.txt -r "\d{3}-\d{2}-\d{4}":"XXX-XX-XXXX"
```
**English:** Full regex support with `-r` flag (heavy dependency, consider carefully)  
**Русский:** Полная поддержка regex с флагом `-r` (тяжёлая зависимость, обдумать)

**Note:** This would require external regex library (PCRE, etc.) - significant complexity increase

---

### 14. Encoding auto-detection / Автоопределение кодировки
**Priority: Medium / Приоритет: Средний**

```bash
replacer -e file.txt "old":"new"  # auto-detect input encoding
```
**English:** Detect file encoding automatically (UTF-8 BOM, UTF-16 LE/BE, etc.)  
**Русский:** Автоматически определить кодировку файла (UTF-8 BOM, UTF-16 LE/BE и т.д.)

---

### 15. Offset-based operations / Операции по смещению
**Priority: MERGED INTO #12 / Приоритет: ОБЪЕДИНЕНО В #12**

**Note:** This feature has been merged into feature #12 (Range-based operations) as byte offset ranges without L prefix.  
**Примечание:** Эта функция объединена с функцией #12 (Операции по диапазону) как байтовые смещения без префикса L.

---

### 16. Interactive mode / Интерактивный режим
**Priority: Low / Приоритет: Низкий**

```bash
replacer -I file.txt "old":"new"
```
**English:** Ask for confirmation before each replacement (y/n/a/q)  
**Русский:** Запрашивать подтверждение перед каждой заменой (y/n/a/q)

**Options:**
- `y` - yes, replace this one
- `n` - no, skip this one
- `a` - all, replace all remaining
- `q` - quit, stop processing

---

### 17. Diff output / Вывод различий
**Priority: Low / Приоритет: Низкий**

```bash
replacer -d file.txt "old":"new"
```
**English:** Show unified diff of changes  
**Русский:** Показать unified diff изменений

---

### 18. Escape sequence support in text / Поддержка escape последовательностей в тексте
**Priority: Medium / Приоритет: Средний**
**Status: COMPLETED / Статус: ЗАВЕРШЕНО**

```bash
# Standard escape sequences in replacements
replacer.exe "file.txt":- "'line1':'line2\n'"      # \n = newline (0x0A)
replacer.exe "file.txt":- "'text':'\t\0\t'"        # \t = tab, \0 = entire match
replacer.exe "file.txt":- "'path':'C:\\new'"       # \\ = backslash
replacer.exe "file.txt":- "'test':'\r\n'"          # \r = CR, \n = LF

# Wildcards outside quotes (unescaped)
replacer.exe "file.txt":- "'test'+*+'end':'MATCH'"     # * = zero or more bytes
replacer.exe "file.txt":- "'test'+.+'end':'MATCH'"     # . = any single byte
replacer.exe "file.txt":- "'colo'+?+'r':'COLOR'"       # ? = optional byte

# Wildcards inside quotes are LITERAL
replacer.exe "file.txt":- "'file*.txt':'found'"        # matches literal "file*.txt"
replacer.exe "file.txt":- "'test.end':'found'"         # matches literal "test.end"

# Hex bytes via \x escape in quotes
replacer.exe "file.txt":- "'test\x0D\x0A':'test\n'"    # \xHH = hex byte

# Both syntaxes supported
replacer.exe "file.txt":- "'text'+0x0A+'more':'new'"   # + concatenation
replacer.exe "file.txt":- "'line1\nline2':'single'"    # \ escape
```

**English:**
- Support `\n`, `\r`, `\t`, `\\`, `\"` in text strings
- Support `\xHH` for hex bytes (e.g., `\x0A`, `\xFF`)
- Support wildcards: `\.`, `\*`, `\?` (alternative to `+` concatenation)
- Both syntaxes work: `+` concatenation for complex cases, `\` escape for simple cases

**Русский:**
- Поддержка `\n`, `\r`, `\t`, `\\`, `\"` в текстовых строках
- Поддержка `\xHH` для hex байтов (например, `\x0A`, `\xFF`)
- Поддержка wildcards: `\.`, `\*`, `\?` (альтернатива конкатенации `+`)
- Оба синтаксиса работают: `+` конкатенация для сложных случаев, `\` escape для простых

**Supported escape sequences:**
- `\n` → 0x0A (LF, newline)
- `\r` → 0x0D (CR, carriage return)
- `\t` → 0x09 (tab)
- `\\` → 0x5C (backslash)
- `\"` → 0x22 (double quote)
- `\'` → 0x27 (single quote)
- `\xHH` → hex byte (e.g., `\x0A`, `\xFF`)
- `\.` → wildcard: any single byte
- `\*` → wildcard: zero or more bytes
- `\?` → wildcard: optional byte (zero or one)

**Implementation notes:**
- Process escape sequences during text parsing in `text_to_bytes()` or new function
- Scan string for `\` character and check next character
- For `\xHH`: parse two hex digits and convert to byte
- For wildcards `\.`, `\*`, `\?`: mark as wildcard segment (integrate with existing wildcard system)
- Keep `+` concatenation syntax working alongside escape sequences
- Escape sequences work inside quoted strings: `"text\nmore"`

---

### 19. Configuration file support / Поддержка конфигурационных файлов
**Priority: Low / Приоритет: Низкий**

```bash
replacer -c rules.txt file.dat
```
**File format (rules.txt):**
```
# Comments start with #
"old":"new"
0xAA:0xBB
"test":"demo":win
```
**English:** Load multiple operations from configuration file  
**Русский:** Загрузить множественные операции из конфигурационного файла

---

### 20. Progress bar for large files / Прогресс-бар для больших файлов
**Priority: Low / Приоритет: Низкий**

```bash
replacer -p largefile.bin "old":"new"
```
**Output:**
```
Processing: [████████████░░░░░░░░] 60% (600MB/1000MB)
```
**English:** Show progress bar for files larger than 10MB  
**Русский:** Показать прогресс-бар для файлов больше 10MB

---

## Implementation Priority / Приоритет реализации

**Phase 1 (High priority):**
1. Capture groups (#1) - COMPLETED
2. Case-insensitive search (#2) - COMPLETED
3. Counters and variables (#3) - COMPLETED
4. Dry-run mode (#7) - COMPLETED
5. Limit replacements - range syntax (#9)
6. Range-based operations - bytes and lines (#12)

**Phase 2 (Medium priority):**
6. Multiple files - glob patterns (#5)
7. Encoding auto-detection (#14)
8. Escape sequences in text (#18)
9. Counters and variables (#3)

**Phase 3 (Low priority):**
10. Backup mode (#6)
11. Verbose statistics (#8)
12. Negative patterns (#10)
13. Interactive mode (#16)
14. Configuration files (#19)
15. Progress bar (#20)
16. Other features

**Note:** Features #4 (Byte ranges), #12 (Line-based), and #15 (Offset-based) have been merged into unified #12 (Range-based operations).

---

---

## Quote Handling in CMD / Работа с кавычками в CMD

**IMPORTANT / ВАЖНО:**

**English:**
- Use **double quotes outside** to group the argument for CMD: `"..."`
- Use **single quotes inside** for literal text: `'...'`
- Wildcards (`*`, `.`, `?`) work ONLY outside single quotes
- Wildcards inside single quotes are treated as literal characters
- Multiple operations: pass each as separate argument

**Русский:**
- Используйте **двойные кавычки снаружи** для группировки аргумента в CMD: `"..."`
- Используйте **одинарные кавычки внутри** для литерального текста: `'...'`
- Wildcards (`*`, `.`, `?`) работают ТОЛЬКО вне одинарных кавычек
- Wildcards внутри одинарных кавычек - литеральные символы
- Множественные операции: передавайте каждую как отдельный аргумент

**Syntax examples / Примеры синтаксиса:**

```cmd
REM Single operation with spaces in replacement
replacer.exe "file.txt":- "'pattern':'replacement text'"

REM Wildcards outside quotes (pattern matching)
replacer.exe "file.txt":- "'Version: '+*:'found'"

REM Wildcards inside quotes (literal characters)
replacer.exe "file.txt":- "'file*.txt':'found'"

REM Multiple operations (separate arguments)
replacer.exe "file.txt":- "'old':'new'" "'test':'demo'" "'foo':'bar'"

REM Capture entire match with \0
replacer.exe "file.txt":- "'error':'[\0]'"

REM Escape sequences in replacement
replacer.exe "file.txt":- "'line':'line\n'" "'text':'\t\0\t'"

REM Hex escapes
replacer.exe "file.txt":- "'colo\*\x72':'change color'"
```

**Why this syntax? / Почему такой синтаксис?**

**English:**
- CMD splits arguments by spaces, even inside single quotes
- Double quotes outside tell CMD to treat the whole thing as one argument
- Single quotes inside mark literal text (not processed as wildcards)
- This avoids complex escaping like `\"Version: \"+\1+\".\"+\2:\"v\1.\2\"`

**Русский:**
- CMD разделяет аргументы по пробелам, даже внутри одинарных кавычек
- Двойные кавычки снаружи говорят CMD обрабатывать всё как один аргумент
- Одинарные кавычки внутри обозначают литеральный текст (не wildcards)
- Это избегает сложного экранирования типа `\"Version: \"+\1+\".\"+\2:\"v\1.\2\"`

---

**Version:** 26.0424b  
**Last updated:** 2026-04-24

## Changelog / История изменений

### v26.0424d (2026-04-24)
- **Fixed binary mode**: Hex-only operations skip encoding conversion
- Binary files no longer corrupted by text encoding
- Pure hex operations work correctly with DOS-866 default

### v26.0424c (2026-04-24)
- **Changed default encoding to DOS-866**: Better Windows CMD compatibility
- No need to specify `@dos` for Cyrillic in console
- Use `@utf` if UTF-8 output needed

### v26.0424b (2026-04-24)
- **Simplified encoding syntax**: Encodings as separate parameter `"input@output"`
- **Cleaner file specification**: `file.txt:output.txt "win@dos"` instead of `win:file.txt:dos:output.txt`
- **Fixed stdin/stdout syntax**: `-:-` (no more `::` confusion)

### v26.0425 (2026-04-25)
- **Auto-increment counters**: `{#}`, `{#:001}`, `{#:A}`, `{#:0x01}`, etc.
- Counter formats: decimal, zero-padded, hex, alphabetic, ranges

### v26.0424 (2026-04-24)
- **Test mode**: `-t` or `--test` flag for preview
- Combinable flags: `-d -t` for debug + test mode

### v26.0422 (2026-04-22)
- **Case-insensitive search**: `/i` flag

### v26.0421 (2026-04-21)
- **Capture groups**: `\0-\9`, `{name=pattern}`, `{name}` references
- **Debug mode**: `-d` flag
