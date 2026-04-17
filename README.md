# REPLACER

**File content search and replace utility with encoding conversion**  
**Утилита поиска и замены содержимого файлов с конвертацией кодировок**

Version 26.0417 by BoyNG (Vyacheslav Burnosov)

---

## Description / Описание

**English:**  
REPLACER is a powerful command-line utility for Windows that performs binary and text search-and-replace operations on files with support for multiple character encodings. It can handle hex patterns, text strings, encoding conversions, and supports pipeline operations for chaining multiple replacements in a single pass.

**Русский:**  
REPLACER — мощная утилита командной строки для Windows, выполняющая операции поиска и замены в бинарных и текстовых файлах с поддержкой множества кодировок. Поддерживает hex-паттерны, текстовые строки, конвертацию кодировок и конвейерные операции для выполнения нескольких замен за один проход.

---

## Features / Возможности

**English:**
- Binary and text search and replace
- Multiple encoding support: UTF-8, Windows-1251 (CP1251), DOS (CP866), KOI8-R
- Hex input format: `0xFFAA` or `$FFAA`
- Text input format: quoted `"text"` or plain text
- Encoding conversion between different code pages
- Pipeline operations: chain multiple replacements
- Stdin/stdout support for use in pipes
- Custom output filenames
- Colorized help output

**Русский:**
- Бинарный и текстовый поиск и замена
- Поддержка множества кодировок: UTF-8, Windows-1251 (CP1251), DOS (CP866), KOI8-R
- Hex формат ввода: `0xFFAA` или `$FFAA`
- Текстовый формат: в кавычках `"текст"` или просто текст
- Конвертация между различными кодовыми страницами
- Конвейерные операции: цепочка нескольких замен
- Поддержка stdin/stdout для использования в конвейерах
- Пользовательские имена выходных файлов
- Цветная справка

---

## Usage / Использование

```
replacer [encoding:]<input>[:[encoding][:output]] [operations...]
```

### File Specification / Формат файла

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `file.bin` | Input file, output will be `file_OUT.bin`<br>Входной файл, выход будет `file_OUT.bin` |
| `file.bin:out.bin` | Input file with custom output name<br>Входной файл с указанием имени выходного |
| `file.bin:-` | Output to stdout (like 'type' command)<br>Вывод в stdout (как команда 'type') |
| `file.txt:utf` | Convert file to UTF-8 encoding<br>Конвертировать файл в кодировку UTF-8 |
| `file.txt:-:utf` | Output to stdout with UTF-8 conversion<br>Вывод в stdout с конвертацией в UTF-8 |
| `win:file.txt` | Read file as Windows-1251<br>Читать файл как Windows-1251 |
| `win:file.txt:utf` | Convert Windows-1251 to UTF-8<br>Конвертировать Windows-1251 в UTF-8 |
| `win:file.txt:-` | Output Windows-1251 file to stdout<br>Вывод файла Windows-1251 в stdout |
| `win:file.txt:-:utf` | Convert and output to stdout<br>Конвертация и вывод в stdout |
| `win:file.txt:utf:out.txt` | Convert with custom output name<br>Конвертация с указанием имени выхода |
| `-` | Read from stdin, write to stdout<br>Читать из stdin, писать в stdout |
| `win:-:utf` | Stdin/stdout with encoding conversion<br>Stdin/stdout с конвертацией кодировки |

### Operation Format / Формат операций

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `search:replace` | Search and replace<br>Найти и заменить |
| `search:` | Delete (empty replace)<br>Удалить (пустая замена) |
| `search:replace:encoding` | With specific encoding for this operation<br>С указанием кодировки для операции |

### Search/Replace Formats / Форматы поиска/замены

**Hex:**
- `0xFFAA` or `$FFAA`

**Text / Текст:**
- `"hello"` or plain text / `"привет"` или просто текст

### Encodings / Кодировки

| Code / Код | Description / Описание |
|------------|------------------------|
| `win` | Windows-1251 (CP1251) |
| `dos` | DOS (CP866) |
| `koi` | KOI8-R (CP20866) |
| `utf` | UTF-8 (default / по умолчанию) |

---

## Examples / Примеры

### Basic hex replacement / Базовая hex замена
```bash
replacer file.bin 0xAA:0xBB 0xCC:0xDD
```

### Text replacement with encoding / Текстовая замена с кодировкой
```bash
replacer file.bin:out.bin 0xAA:0xBB "old":"new":win
```

### Output to stdout / Вывод в stdout
```bash
replacer file.txt:-
replacer file.txt:- 0xAA:0xBB
```

### Encoding conversion / Конвертация кодировок
```bash
replacer win:file.txt:utf
replacer win:file.txt:-:utf
replacer win:file.txt:utf "тест":"test"
```

### Pipeline operations / Конвейерные операции
```bash
replacer file.txt "тест":"test":win "hello":
```

### Stdin/stdout mode / Режим stdin/stdout
```bash
replacer - 0xAA:0xBB < in.bin > out.bin
type in.bin | replacer - 0xAA:0xBB > out.bin
type file.txt | replacer - "old":"new"
replacer - "old":"new" < file.txt
replacer - "old":"new" < file.txt 2>nul
replacer win:-:utf < input.txt > output.txt
```

---

### CLI help information / Вывод помощи в консоли

![screenshot](screenshot.png)

---

## Building / Сборка

**English:**  
Compile with any C compiler for Windows (MSVC, MinGW, etc.):

```bash
gcc -o replacer.exe replacer.c
cl replacer.c
```

**Русский:**  
Компилируйте любым C компилятором для Windows (MSVC, MinGW и т.д.):

```bash
gcc -o replacer.exe replacer.c
cl replacer.c
```

---

## Requirements / Требования

- Windows OS
- Windows API support
- ANSI color support (Windows 10+ with virtual terminal processing)

---

## License / Лицензия

**English:**  
Free to use and modify.

**Русский:**  
Свободно для использования и модификации.

---

## Author / Автор

**BoyNG (Vyacheslav Burnosov)**

Version / Версия: 26.0417
