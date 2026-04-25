# REPLACER

**File content search and replace utility with encoding conversion**  
**Утилита поиска и замены содержимого файлов с конвертацией кодировок**

Version 26.0424d by BoyNG (Vyacheslav Burnosov)

---

## Table of Contents / Содержание

- [What's New / Что нового](#whats-new--что-нового)
  - [New in 26.0424d](#new-in-260424d--новое-в-версии-260424d)
  - [New in 26.0424c](#new-in-260424c--новое-в-версии-260424c)
  - [New in 26.0424b](#new-in-260424b--новое-в-версии-260424b)
  - [New in 26.0425](#new-in-260425--новое-в-версии-260425)
  - [New in 26.0424](#new-in-260424--новое-в-версии-260424)
  - [New in 26.0422](#new-in-260422--новое-в-версии-260422)
  - [New in 26.0421](#new-in-260421--новое-в-версии-260421)
  - [New in 26.0419](#new-in-260419--новое-в-версии-260419)
  - [New in 26.0418](#new-in-260418--новое-в-версии-260418)
- [Description / Описание](#description--описание)
- [Features / Возможности](#features--возможности)
- [Usage / Использование](#usage--использование)
  - [File Specification](#file-specification--формат-файла)
  - [Operation Format](#operation-format--формат-операций)
  - [Encodings](#encodings--кодировки)
  - [Wildcards](#wildcards--подстановочные-символы)
  - [Capture Groups](#capture-groups--группы-захвата)
  - [Counters](#counters--счётчики)
  - [Debug Mode](#debug-mode--режим-отладки)
  - [Case-Insensitive Search](#case-insensitive-search--поиск-без-учёта-регистра)
  - [Concatenation](#concatenation--конкатенация)
  - [Quoting Rules](#important-quoting-rules--важно-правила-кавычек)
- [Examples / Примеры](#examples--примеры)
  - [Basic Operations](#1-basic-hex-replacement--базовая-hex-замена)
  - [Text Replacement](#3-text-replacement--текстовая-замена)
  - [Encoding Conversion](#5-encoding-conversion--конвертация-кодировок)
  - [Wildcard Patterns](#9-wildcard-patterns--паттерны-с-wildcards)
  - [Capture Groups Examples](#14-capture-groups-examples--примеры-групп-захвата)
  - [Counter Examples](#16-counter-examples--примеры-счётчиков)
  - [Practical Use Cases](#15-practical-use-cases--практические-примеры)
- [Building / Сборка](#building--сборка)
- [Requirements / Требования](#requirements--требования)
- [License / Лицензия](#license--лицензия)

---

## What's New / Что нового

### New in 26.0424d / Новое в версии 26.0424d

**English:**
- **Fixed binary mode**: Hex-only operations now skip encoding conversion
  - Pure hex operations (e.g., `0xAA:0x00`) work correctly with any default encoding
  - Binary files are no longer corrupted by text encoding conversion
  - Mixed operations (hex + text) still apply encoding as needed

**Русский:**
- **Исправлен бинарный режим**: Операции только с hex теперь пропускают конвертацию кодировок
  - Чистые hex операции (например, `0xAA:0x00`) работают корректно с любой кодировкой по умолчанию
  - Бинарные файлы больше не искажаются текстовой конвертацией кодировок
  - Смешанные операции (hex + текст) по-прежнему применяют кодировку

### New in 26.0424c / Новое в версии 26.0424c

**English:**
- **Changed default encoding to DOS-866**: Better compatibility with Windows CMD
  - No need to specify `@dos` for Cyrillic text in console
  - Default is now `dos` instead of `utf`
  - Use `@utf` if you need UTF-8 output

**Русский:**
- **Изменена кодировка по умолчанию на DOS-866**: Лучшая совместимость с Windows CMD
  - Не нужно указывать `@dos` для кириллицы в консоли
  - По умолчанию теперь `dos` вместо `utf`
  - Используйте `@utf` если нужен вывод в UTF-8

### New in 26.0424b / Новое в версии 26.0424b

**English:**
- **Simplified encoding syntax**: Encodings now specified as separate parameter `"input@output"`
  - `"win@dos"` - read as WIN-1251, write as DOS-866
  - `"win@"` - read as WIN-1251, write as DOS-866 (default)
  - `"@dos"` - read as DOS-866, write as DOS-866
- **Cleaner file specification**: No more encoding mixed with file paths
  - Old: `win:file.txt:dos:output.txt` (confusing)
  - New: `file.txt:output.txt "win@dos"` (clear)
- **Simplified stdin/stdout syntax**:
  - `-:-` for stdin to stdout (no more `::` confusion)
  - `-:file.txt` for stdin to file
  - `file.txt:-` for file to stdout
- Encodings: `win` (Windows-1251), `dos` (DOS-866), `koi` (KOI8-R), `utf` (UTF-8)

**Русский:**
- **Упрощённый синтаксис кодировок**: Кодировки теперь отдельный параметр `"вход@выход"`
  - `"win@dos"` - читать как WIN-1251, писать как DOS-866
  - `"win@"` - читать как WIN-1251, писать как DOS-866 (по умолчанию)
  - `"@dos"` - читать как DOS-866, писать как DOS-866
- **Понятная спецификация файлов**: Кодировки больше не смешиваются с путями
  - Старый: `win:file.txt:dos:output.txt` (запутанно)
  - Новый: `file.txt:output.txt "win@dos"` (понятно)
- **Упрощённый синтаксис stdin/stdout**:
  - `-:-` для stdin в stdout (больше нет путаницы с `::`)
  - `-:file.txt` для stdin в файл
  - `file.txt:-` для файла в stdout
- Кодировки: `win` (Windows-1251), `dos` (DOS-866), `koi` (KOI8-R), `utf` (UTF-8)

---

### New in 26.0424a / Новое в версии 26.0424a

**English:**
- **Auto-increment counters**: Use `{#}` syntax for automatic numbering in replacements
  - `{#}` - simple counter (1, 2, 3...)
  - `{#:5}` - start from specific number (5, 6, 7...)
  - `{#:001}` - zero-padded format (001, 002, 003...)
  - `{#:03-05}` - range with auto-stop (03, 04, 05, then stop)
  - `{#:0x01}` - hexadecimal format (0x01, 0x02, 0x03...)
  - `{#:A}` - uppercase alphabetic (A, B, C... Z, AA, AB...)
  - `{#:a}` - lowercase alphabetic (a, b, c... z, aa, ab...)
- Counter works outside quotes like capture groups: `"'TODO':'Task '+{#}"`
- Counter can be combined with capture groups: `"'error':'['+{#}+'] \0'"`

**Русский:**
- **Автоинкремент счётчики**: Используйте синтаксис `{#}` для автоматической нумерации в заменах
  - `{#}` - простой счётчик (1, 2, 3...)
  - `{#:5}` - начать с определённого числа (5, 6, 7...)
  - `{#:001}` - формат с нулями (001, 002, 003...)
  - `{#:03-05}` - диапазон с автостопом (03, 04, 05, затем стоп)
  - `{#:0x01}` - шестнадцатеричный формат (0x01, 0x02, 0x03...)
  - `{#:A}` - заглавные буквы (A, B, C... Z, AA, AB...)
  - `{#:a}` - строчные буквы (a, b, c... z, aa, ab...)
- Счётчик работает вне кавычек как группы захвата: `"'TODO':'Task '+{#}"`
- Счётчик можно комбинировать с группами захвата: `"'error':'['+{#}+'] \0'"`

---

## What's New / Что нового

### New in 26.0424 / Новое в версии 26.0424

**English:**
- **Test mode**: Use `-t` or `--test` flag to preview changes without modifying files
- **Match preview**: Shows line numbers and context for each match
- **Safe testing**: Test complex patterns before applying them to important files
- **Operation details**: Displays pattern type and flags for each operation

**Русский:**
- **Тестовый режим**: Используйте флаг `-t` или `--test` для просмотра изменений без модификации файлов
- **Предпросмотр совпадений**: Показывает номера строк и контекст для каждого совпадения
- **Безопасное тестирование**: Тестируйте сложные паттерны перед применением к важным файлам
- **Детали операций**: Отображает тип паттерна и флаги для каждой операции

### New in 26.0422 / Новое в версии 26.0422

**English:**
- **Case-insensitive search**: Use `/i` flag at end of operation to ignore case
- **Flexible flag placement**: Works with or without encoding: `'hello':'HELLO'/i` or `'hello':'HELLO':utf/i`
- **Works with all patterns**: Case-insensitive matching for literal patterns, wildcards, and capture groups
- **Debug support**: Shows `[/i case-insensitive]` flag in debug mode output

**Русский:**
- **Поиск без учёта регистра**: Используйте флаг `/i` в конце операции для игнорирования регистра
- **Гибкое размещение флага**: Работает с кодировкой и без: `'hello':'HELLO'/i` или `'hello':'HELLO':utf/i` или даже `'hello':HELLO/i`
- **Работает со всеми паттернами**: Поиск без учёта регистра для литеральных паттернов, wildcards и групп захвата
- **Поддержка отладки**: Показывает флаг `[/i case-insensitive]` в режиме отладки

### New in 26.0421 / Новое в версии 26.0421

**English:**
- **Capture Groups**: Save and reuse matched patterns with `{pattern}` and `{name=pattern}`
- **Numbered groups**: Up to 9 groups `{pattern}` referenced as `\1`, `\2`, ..., `\9`
- **Named groups**: Unlimited groups `{name=pattern}` referenced as `{name}` (outside quotes)
- **Debug mode**: Use `-d` flag to see detailed processing information
- **Bilingual errors**: All error messages in English and Russian
- **Single quote syntax**: Use single quotes `'...'` for literals inside double quotes for CMD
- **Simplified quoting**: `"'pattern':'replacement'"` instead of complex escaping
- **Capture entire match**: `\0` in replacement captures the entire matched pattern
- **Escape sequences in replacements**: `\n`, `\r`, `\t`, `\\`, `\"`, `\'` work in replacement strings
- **Quote-aware wildcards**: Wildcards inside quotes are literal, outside quotes are patterns
- **Multiple operations**: Pass each operation as separate argument

**Русский:**
- **Группы захвата**: Сохранение и повторное использование найденных паттернов с `{pattern}` и `{name=pattern}`
- **Нумерованные группы**: До 9 групп `{pattern}` с ссылками `\1`, `\2`, ..., `\9`
- **Именованные группы**: Неограниченное количество групп `{name=pattern}` с ссылками `{name}` (вне кавычек)
- **Режим отладки**: Используйте флаг `-d` для просмотра детальной информации об обработке
- **Билингвальные ошибки**: Все сообщения об ошибках на английском и русском
- **Синтаксис с одинарными кавычками**: Используйте одинарные кавычки `'...'` для литералов внутри двойных для CMD
- **Упрощённые кавычки**: `"'паттерн':'замена'"` вместо сложного экранирования
- **Захват всего вхождения**: `\0` в замене захватывает весь найденный паттерн
- **Escape последовательности в заменах**: `\n`, `\r`, `\t`, `\\` работают в строках замены
- **Wildcards с учётом кавычек**: Wildcards внутри кавычек - литеральные, снаружи - паттерны
- **Множественные операции**: Передавайте каждую операцию как отдельный аргумент

## New in 26.0419 / Новое в версии 26.0419

**English:**
- **Escape sequences**: Support for `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, `\xHH` in text strings
- **Wildcards in text**: Use `\.`, `\*`, `\?` directly inside text without `+` concatenation
- **Flexible syntax**: Both `"test\.end"` and `"test"+\.+"end"` work
- **Hex escapes**: Use `\xHH` for hex bytes (e.g., `\x0A`, `\xFF`)

**Русский:**
- **Escape последовательности**: Поддержка `\n`, `\r`, `\t`, `\\`, `\"`, `\xHH` в текстовых строках
- **Wildcards в тексте**: Используйте `\.`, `\*`, `\?` прямо внутри текста без конкатенации `+`
- **Гибкий синтаксис**: Работают оба варианты `"test\.end"` и `"test"+\.+"end"`
- **Hex escape**: Используйте `\xHH` для hex байтов (например, `\x0A`, `\xFF`)

---

## New in 26.0418 / Новое в версии 26.0418

**English:**
- **Wildcard support**: Pattern matching with `\.` (any byte), `\*` (zero or more bytes), `\?` (optional byte)
- **Concatenation operator**: Join hex and text parts with `+` operator
- **Escape sequences**: Use backslash to enable wildcards, without backslash they are literal characters
- **Enhanced pattern matching**: Complex patterns like `"<tag>"+\*+"</tag>"` or `"colo"+\?+"r"`

**Русский:**
- **Поддержка wildcards**: Поиск по паттернам с `\.` (любой байт), `\*` (ноль или более байтов), `\?` (необязательный байт)
- **Оператор конкатенации**: Объединение hex и текста оператором `+`
- **Escape последовательности**: Используйте обратный слэш для wildcards, без слэша - литеральные символы
- **Расширенный поиск**: Сложные паттерны типа `"<tag>"+\*+"</tag>"` или `"colo"+\?+"r"`

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
- Text input format: quoted `'text'` or plain text
- Encoding conversion between different code pages
- Pipeline operations: chain multiple replacements
- Stdin/stdout support for use in pipes
- Custom output filenames
- Colorized help output

**Русский:**
- Бинарный и текстовый поиск и замена
- Поддержка множества кодировок: UTF-8, Windows-1251 (CP1251), DOS (CP866), KOI8-R
- Hex формат ввода: `0xFFAA` или `$FFAA`
- Текстовый формат: в кавычках `'текст'` или просто текст
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

**NEW in 26.0424b: Simplified syntax / НОВОЕ в 26.0424b: Упрощённый синтаксис**

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `file.bin` | Input file, output will be `file_OUT.bin`<br>Входной файл, выход будет `file_OUT.bin` |
| `file.bin:out.bin` | Input file with custom output name<br>Входной файл с указанием имени выходного |
| `file.bin:-` | Output to stdout<br>Вывод в stdout |
| `-` | Read from stdin, write to stdout<br>Читать из stdin, писать в stdout |
| `-:-` | Stdin to stdout (explicit)<br>Stdin в stdout (явно) |
| `-:output.txt` | Stdin to file<br>Stdin в файл |

**Encoding specification (separate parameter) / Спецификация кодировки (отдельный параметр):**

Add encoding as separate parameter after file specification:  
Добавьте кодировку отдельным параметром после спецификации файла:

```cmd
replacer.exe file.txt operations...                    # DOS-866 to DOS-866 (default)
replacer.exe file.txt "@utf" operations...             # DOS-866 to UTF-8
replacer.exe file.txt "win@dos" operations...          # WIN-1251 to DOS-866
replacer.exe file.txt:output.txt "win@utf" operations... # WIN-1251 to UTF-8
replacer.exe -:- operations...                         # stdin to stdout (DOS-866)
replacer.exe -:output.txt "@utf" operations...         # stdin to file (UTF-8)
```

### Operation Format / Формат операций

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `search:replace` | Search and replace<br>Найти и заменить |
| `search:` | Delete (empty replace)<br>Удалить (пустая замена) |
| `search:replace/i` | Case-insensitive search<br>Поиск без учёта регистра |

### Search/Replace Formats / Форматы поиска/замены

**Hex:**
- `0xFFAA` or `$FFAA`

**Text / Текст:**
- `'hello'` / `'привет'`
Расположен внутри одиночных кавычек.

### Encodings / Кодировки

**NEW in 26.0424b: Simplified syntax / НОВОЕ в 26.0424b: Упрощённый синтаксис**

Encodings are now specified as a separate parameter using `"input@output"` format:  
Кодировки теперь указываются отдельным параметром в формате `"вход@выход"`:

| Format / Формат | Description / Описание |
|-----------------|------------------------|
| `"win@dos"` | Read as WIN-1251, write as DOS-866 / Читать как WIN-1251, писать как DOS-866 |
| `"win@"` | Read as WIN-1251, write as DOS-866 / Читать как WIN-1251, писать как DOS-866 |
| `"@dos"` | Read as DOS-866, write as DOS-866 / Читать как DOS-866, писать как DOS-866 |
| (omitted) | Both DOS-866 (default) / Обе DOS-866 (по умолчанию) |

**Available encodings / Доступные кодировки:**

| Code / Код | Description / Описание |
|------------|------------------------|
| `win` | Windows-1251 (CP1251) |
| `dos` | DOS-866 (CP866) (default / по умолчанию)|
| `koi` | KOI8-R (CP20866) |
| `utf` | UTF-8 |

**Examples / Примеры:**

```cmd
REM Convert file encoding / Конвертация кодировки файла
replacer.exe file.txt "win@dos"

REM Read WIN-1251, process, write UTF-8 / Читать WIN-1251, обработать, писать UTF-8
replacer.exe file.txt "win@" "'old':'new'"

REM Stdin UTF-8 to stdout DOS-866 / Stdin UTF-8 в stdout DOS-866
echo "text" | replacer.exe -:- "@dos" "'test':'demo'"

REM File to file with encoding / Файл в файл с кодировкой
replacer.exe input.txt:output.txt "win@dos" "'pattern':'replacement'"
```

### Wildcards / Подстановочные символы

| Symbol / Символ | Description / Описание |
|-----------------|------------------------|
| `\.` | Any single byte / Любой один байт |
| `\*` | Zero or more bytes / Ноль или более байтов |
| `\?` | Optional byte (zero or one) / Необязательный байт (ноль или один) |

**Note / Примечание:** Use backslash to escape wildcards. Without backslash they are literal characters.  
Используйте обратный слэш для wildcards если внутри `'текстового'` блока, т.к без слэша - это литеральные символы!
Вне текстового блока возможно использование без слеша.

### Capture Groups / Группы захвата

**NEW in 26.0421 / НОВОЕ в 26.0421**

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `{pattern}` | Numbered capture group (max 9) / Нумерованная группа захвата (максимум 9) |
| `{name=pattern}` | Named capture group (unlimited) / Именованная группа захвата (неограниченно) |
| `\0` | Reference entire match / Ссылка на всё совпадение |
| `\1` to `\9` | Reference numbered groups / Ссылка на нумерованные группы |
| `{name}` | Reference named group (outside quotes only) / Ссылка на именованную группу (только вне кавычек) |

**Rules / Правила:**
- Group names: `[a-zA-Z][a-zA-Z0-9_]*`, max 32 chars / Имена групп: `[a-zA-Z][a-zA-Z0-9_]*`, максимум 32 символа
- `{name}` references work only outside quotes / Ссылки `{name}` работают только вне кавычек
- `\1-\9` references work everywhere / Ссылки `\1-\9` работают везде
- Max 9 numbered groups, unlimited named groups / Максимум 9 нумерованных групп, неограниченно именованных

**Examples / Примеры:**
```cmd
REM Swap two parts / Поменять местами две части
replacer.exe "file.txt":- "'['+{*}+'] '+{*}:'\2 (\1)'"
REM Input: [ERROR] File not found
REM Output: File not found (ERROR)

REM Named groups / Именованные группы
replacer.exe "file.txt":- "'Name: '+{name=*}+', Age: '+{age=*}:{age}+' years old, name='+{name}"
REM Input: Name: John, Age: 30
REM Output: 30 years old, name=John

REM Multiple references / Множественные ссылки
replacer.exe "file.txt":- "{word=*}:{word}+{word}+{word}"
REM Input: test
REM Output: testtesttest
```

### Counters / Счётчики

**NEW in 26.0425 / НОВОЕ в 26.0425**

Auto-increment counters for automatic numbering in replacements.  
Автоинкремент счётчики для автоматической нумерации в заменах.

| Syntax / Синтаксис | Description / Описание |
|---------------------|------------------------|
| `{#}` | Simple counter: 1, 2, 3... / Простой счётчик: 1, 2, 3... |
| `{#:N}` | Start from N: 5, 6, 7... / Начать с N: 5, 6, 7... |
| `{#:00N}` | Zero-padded: 001, 002, 003... / С нулями: 001, 002, 003... |
| `{#:N-M}` | Range N to M, then stop / Диапазон от N до M, затем стоп |
| `{#:0xNN}` | Hex format: 0x01, 0x02... / Hex формат: 0x01, 0x02... |
| `{#:A}` | Uppercase: A, B, C... Z, AA, AB... / Заглавные: A, B, C... Z, AA, AB... |
| `{#:a}` | Lowercase: a, b, c... z, aa, ab... / Строчные: a, b, c... z, aa, ab... |

**Rules / Правила:**
- Counter `{#...}` works outside quotes only / Счётчик `{#...}` работает только вне кавычек
- Counter increments with each replacement / Счётчик увеличивается с каждой заменой
- Range format stops after reaching end value / Формат диапазона останавливается после достижения конца
- Can be combined with capture groups / Можно комбинировать с группами захвата

**Examples / Примеры:**
```cmd
REM Simple numbering / Простая нумерация
replacer.exe "file.txt":- "'TODO':'Task '+{#}"
REM Input: TODO, TODO, TODO
REM Output: Task 1, Task 2, Task 3

REM Start from 5 / Начать с 5
replacer.exe "file.txt":- "'TODO':'Task '+{#:5}"
REM Output: Task 5, Task 6, Task 7

REM Zero-padded / С нулями
replacer.exe "file.txt":- "'TODO':'Task '+{#:001}"
REM Output: Task 001, Task 002, Task 003

REM Range with auto-stop / Диапазон с автостопом
replacer.exe "file.txt":- "'TODO':'Task '+{#:03-05}"
REM Output: Task 03, Task 04, Task 05 (then stops)

REM Hex format / Hex формат
replacer.exe "file.txt":- "'TODO':'Task '+{#:0x01}"
REM Output: Task 0x01, Task 0x02, Task 0x03

REM Alphabetic / Алфавитный
replacer.exe "file.txt":- "'TODO':'Item '+{#:A}"
REM Output: Item A, Item B, Item C... Item Z, Item AA, Item AB

REM Combined with capture groups / Комбинация с группами захвата
replacer.exe "file.txt":- "'error':'['+{#}+'] \0'"
REM Input: error, warning, error
REM Output: [1] error, warning, [2] error
```

### Debug Mode / Режим отладки

**NEW in 26.0421 / НОВОЕ в 26.0421**

Use `-d` flag as first argument to see detailed processing information:

```cmd
replacer.exe -d "file.txt":- "'pattern':'replacement'"
```

**Shows / Показывает:**
- Command line arguments / Аргументы командной строки
- Parsed file specification / Распарсенную спецификацию файла
- Operations details (pattern type, segments, groups) / Детали операций (тип паттерна, сегменты, группы)
- Input file size / Размер входного файла
- Processing statistics (replacements, size changes) / Статистику обработки (замены, изменения размера)

### Case-Insensitive Search / Поиск без учёта регистра

**NEW in 26.0422 / НОВОЕ в 26.0422**

Use `/i` flag at the end of operation to perform case-insensitive matching:

**Syntax / Синтаксис:**
```cmd
REM Without encoding / Без кодировки
replacer.exe "file.txt":- "'hello':'HELLO'/i"

REM With encoding / С кодировкой
replacer.exe "file.txt":- "'hello':'HELLO':utf/i"
```

**Features / Возможности:**
- Works with literal patterns / Работает с литеральными паттернами
- Works with wildcards / Работает с wildcards
- Works with capture groups / Работает с группами захвата
- Byte-by-byte case comparison using `tolower()` / Побайтовое сравнение регистра через `tolower()`

**Examples / Примеры:**
```cmd
REM Match any case variation / Найти любой вариант регистра
replacer.exe "file.txt":- "'hello':'HELLO'/i"
REM Matches: hello, Hello, HELLO, HeLLo, etc.

REM With wildcards / С wildcards
replacer.exe "file.txt":- "'error'+*+'found':'ERROR FOUND'/i"
REM Matches: error 123 found, ERROR abc FOUND, Error XYZ Found, etc.

REM With capture groups / С группами захвата
replacer.exe "file.txt":- "'name: '+{name=*}/i:'{name}'"
REM Matches: name: John, NAME: JOHN, Name: John, etc.
```

### Concatenation / Конкатенация

| Operator / Оператор | Description / Описание |
|---------------------|------------------------|
| `+` | Join operator to combine hex and text parts / Оператор объединения для комбинирования hex и текста |

**Examples / Примеры:**
- `'text'+0x0A+'more'` - join text, hex byte, and text / объединить текст, hex байт и текст
- `'<tag>'+\*+'</tag>'` - match anything between tags / найти что угодно между тегами
- `0xAA+\.+0xBB` - match 0xAA, any byte, then 0xBB / найти 0xAA, любой байт, затем 0xBB
- `'colo'+\?+'r'` - match color, colour, colo?r / найти color, colour, colo?r

### Important: Quoting Rules / Важно: Правила кавычек

**CRITICAL: New Syntax in 26.0420 / КРИТИЧНО: Новый синтаксис в 26.0420**

**For Windows CMD / Для Windows CMD:**
```cmd
REM Use double quotes outside, single quotes inside for literals
replacer.exe "file.txt":- "'pattern':'replacement'"

REM Wildcards work ONLY outside single quotes
replacer.exe "file.txt":- "'Version: '+*:'found'"

REM Wildcards inside single quotes are LITERAL
replacer.exe "file.txt":- "'file*.txt':'found'"

REM Multiple operations as separate arguments
replacer.exe "file.txt":- "'old':'new'" "'test':'demo'"

REM Capture entire match with \0
replacer.exe "file.txt":- "'error':'[\0]'"

REM Escape sequences in replacements
replacer.exe "file.txt":- "'line':'text\n'" "'tab':'\t\0\t'"
```

**English:**
- **Double quotes outside** `"..."` - required by CMD to group the argument
- **Single quotes inside** `'...'` - mark literal text (not wildcards)
- **Wildcards outside quotes** - `*`, `.`, `?` work as patterns
- **Wildcards inside quotes** - treated as literal characters
- **Spaces in replacements** - work naturally with this syntax
- **`\0` in replacement** - captures entire matched pattern
- **Escape sequences** - `\n`, `\r`, `\t`, `\\` work in replacements

**Русский:**
- **Двойные кавычки снаружи** `"..."` - требуются CMD для группировки аргумента
- **Одинарные кавычки внутри** `'...'` - обозначают литеральный текст (не wildcards)
- **Wildcards вне кавычек** - `*`, `.`, `?` работают как паттерны
- **Wildcards внутри кавычек** - обрабатываются как литеральные символы
- **Пробелы в заменах** - работают естественно с этим синтаксисом
- **`\0` в замене** - захватывает весь найденный паттерн
- **Escape последовательности** - `\n`, `\r`, `\t`, `\\` работают в заменах

**Why this syntax? / Почему такой синтаксис?**

Old syntax required complex escaping:
```cmd
REM OLD (complex, error-prone)
replacer.exe file.txt "\"Version: \"+\1+\".\"+\2:\"v\1.\2\""
```

New syntax is much simpler:
```cmd
REM NEW (simple, clear)
replacer.exe "file.txt":- "'Version: '+{*}+'.'+{*}:'v\1.\2'"
```

**For Git Bash (Unix shell on Windows) / Для Git Bash (Unix shell на Windows):**
```bash
# Still use single quotes to protect from bash
replacer file.txt 'pattern:replacement'
replacer file.txt "'pattern':'replacement'"
```
Use single quotes to protect the entire argument from bash processing.  
Используйте одинарные кавычки для защиты всего аргумента от обработки bash.

---

## Examples / Примеры

### 1. Basic hex replacement / Базовая hex замена

```bash
replacer file.bin 0xAA:0xBB
```
**English:** Replace all bytes `0xAA` with `0xBB` in `file.bin`, output to `file_OUT.bin`  
**Русский:** Заменить все байты `0xAA` на `0xBB` в `file.bin`, результат в `file_OUT.bin`

```bash
replacer file.bin $AA:$BB $CC:$DD
```
**English:** Multiple replacements: replace `0xAA` with `0xBB` AND `0xCC` with `0xDD` in one pass  
**Русский:** Множественные замены: заменить `0xAA` на `0xBB` И `0xCC` на `0xDD` за один проход

### 2. Custom output filename / Пользовательское имя выходного файла

```bash
replacer file.bin:output.bin 0xAA:0xBB
```
**English:** Replace bytes and save result to `output.bin` instead of default `file_OUT.bin`  
**Русский:** Заменить байты и сохранить результат в `output.bin` вместо `file_OUT.bin` по умолчанию

### 3. Text replacement / Текстовая замена

```cmd
replacer.exe "file.txt":- "'hello':'world'"
```
**English:** Replace text "hello" with "world" in `file.txt`, output to stdout  
**Русский:** Заменить текст "hello" на "world" в `file.txt`, вывод в stdout

```bash
replacer file.txt hello:world
```
**English:** Same as above but without quotes (plain text format)  
**Русский:** То же самое, но без кавычек (формат простого текста)

```cmd
replacer.exe "file.txt":- "'old text':'new text'" "'foo':'bar'"
```
**English:** Multiple text replacements in one pass  
**Русский:** Множественные текстовые замены за один проход

### 4. Delete operation / Операция удаления

```cmd
replacer.exe "file.txt":- "'hello':'"
```
**English:** Delete all occurrences of "hello" (replace with empty string)  
**Русский:** Удалить все вхождения "hello" (заменить на пустую строку)

```bash
replacer file.bin 0xAA:
```
**English:** Delete all bytes `0xAA` from binary file  
**Русский:** Удалить все байты `0xAA` из бинарного файла

### 5. Encoding conversion / Конвертация кодировок

```bash
replacer file.txt:output.txt "@win"
```
**English:** Convert from DOS (CP866) to Windows-1251, save as `output.txt`  
**Русский:** Конвертировать из DOS (CP866) в Windows-1251, сохранить как `output.txt`

```bash
replacer file.txt "koi@utf"
```
**English:** Convert from KOI8-R to UTF-8  
**Русский:** Конвертировать из KOI8-R в UTF-8

### 6. Text replacement with specific encoding / Текстовая замена с указанием кодировки

```cmd
replacer.exe "file.txt":- "win@utf" "'тест':'test'"
```
**English:** Read file as Windows-1251, replace Russian "тест" with "test", output as UTF-8  
**Русский:** Читать файл как Windows-1251, заменить русское "тест" на "test", вывод в UTF-8

```cmd
replacer.exe "file.txt":- "'привет':'hello':win"
```
**English:** Replace text using Windows-1251 encoding for this specific operation  
**Русский:** Заменить текст используя кодировку Windows-1251 для этой конкретной операции

```cmd
replacer.exe "file.txt":- "utf@" "'текст':'text':utf" "'hello':'мир':utf"
```
**English:** Read UTF file, perform multiple replacements with UTF encoding  
**Русский:** Читать UTF файл, выполнить множественные замены с кодировкой UTF

### 7. Output to stdout / Вывод в stdout

```bash
replacer file.txt:-
```
**English:** Output file content to stdout (like `type` command), no modifications  
**Русский:** Вывести содержимое файла в stdout (как команда `type`), без изменений

```bash
replacer file.txt:- 0xAA:0xBB
```
**English:** Replace bytes and output result to stdout instead of file  
**Русский:** Заменить байты и вывести результат в stdout вместо файла

```bash
replacer file.txt:- "win@" "'hello':'мир'"
```
**English:** Replace window text and output to stdout in DOS encoding  
**Русский:** Заменить window текст и вывести в stdout в кодировке DOS

### 8. Pipeline operations / Конвейерные операции

```cmd
replacer.exe "file.txt":- "'old':'new'" "'foo':'bar'" "'test':"
```
**English:** Chain multiple operations: replace "old" with "new", "foo" with "bar", delete "test"  
**Русский:** Цепочка операций: заменить "old" на "new", "foo" на "bar", удалить "test". Допускается опускать одиночные кавычки текста.

```cmd
echo color colour colorifer | replacer.exe -:- "{'co'+*}+'r':'\t\1bok'" "'lobok':'t'"
>        cot     coloubok        cotifer
```
**English:** Sequential text replacements (note: operations are applied in order)  
**Русский:** Последовательные текстовые замены (внимание: операции применяются по порядку)

```bash
replacer file.bin 0xAA0000:0x00BB00 0xBBAA00:0x00 0x0000CC:0xAAAAAA
```
**English:** Sequential hex replacements (note: operations are applied in order)  
**Русский:** Последовательные hex замены (внимание: операции применяются по порядку)

```cmd
replacer.exe file.txt:- "win@utf" "'тест':'test'" "'hello':'привет'"
```
**English:** Convert Windows-1251 to UTF-8 and perform replacements  
**Русский:** Конвертация Windows-1251 в UTF-8 с заменами

### 9. Wildcard patterns / Паттерны с wildcards

**NEW SYNTAX (26.0420):**

```cmd
replacer.exe "test.html":- "'<title>'+*+'</title>':'<title>New</title>'"
```
**English:** Match anything between tags using `*` wildcard (outside quotes)  
**Русский:** Найти что угодно между тегами используя wildcard `*` (вне кавычек)

```cmd
replacer.exe "test.bin":- "0xAA+.+0xBB:0xFF"
```
**English:** Match 0xAA, any single byte (`.`), then 0xBB, replace with 0xFF  
**Русский:** Найти 0xAA, любой один байт (`.`), затем 0xBB, заменить на 0xFF

```cmd
replacer.exe "test.txt":- "'colo'+?+'r':'COLOR'"
```
**English:** Match "color" or "colour" using optional byte wildcard `?`  
**Русский:** Найти "color" или "colour" используя wildcard необязательного байта `?`

```cmd
replacer.exe "test.txt":- "'file*.txt':'found'"
```
**English:** Match literal `file*.txt` (wildcards inside quotes are literal)  
**Русский:** Найти литеральный `file*.txt` (wildcards внутри кавычек - литеральные)

```cmd
replacer.exe "test.txt":- "'file'+*+'.txt':'found'"
```
**English:** Match `file` + any chars + `.txt` (wildcards outside quotes are patterns)  
**Русский:** Найти `file` + любые символы + `.txt` (wildcards вне кавычек - паттерны)

```bash
replacer test.txt "'test'+0x0D0A:'replaced'+0x0A"
```
**English:** Concatenate text and hex: match "test" + CRLF, replace with "replaced" + LF  
**Русский:** Конкатенация текста и hex: найти "test" + CRLF, заменить на "replaced" + LF

```bash
replacer file.bin 0xFF+\*+0x00:0xAA
```
**English:** Match 0xFF followed by any bytes followed by 0x00, replace entire match with 0xAA  
**Русский:** Найти 0xFF за которым следуют любые байты и затем 0x00, заменить всё совпадение на 0xAA

```bash
replacer log.txt "'ERROR'+*+0x0A:"
```
**English:** Delete entire lines starting with "ERROR" (match ERROR + any chars + newline, replace with empty)  
**Русский:** Удалить целые строки начинающиеся с "ERROR" (найти ERROR + любые символы + перевод строки, заменить на пустоту)

```bash
replacer data.txt "'['+0x22+*+0x22+']':'[REDACTED]'"
```
**English:** Match and replace JSON-like strings in brackets: `["anything"]` → `[REDACTED]`  
**Русский:** Найти и заменить JSON-подобные строки в скобках: `["что угодно"]` → `[REDACTED]`

```bash
replacer config.ini "'password'+*+0x0D0A:'password=***'+0x0D0A"
```
**English:** Redact passwords in INI files: match "password" + any chars + CRLF, replace with masked value  
**Русский:** Скрыть пароли в INI файлах: найти "password" + любые символы + CRLF, заменить на замаскированное значение

```bash
replacer source.c "'//'+*+0x0A:0x0A"
```
**English:** Remove C++ style comments: match "//" + any chars + newline, replace with just newline  
**Русский:** Удалить C++ комментарии: найти "//" + любые символы + перевод строки, заменить на просто перевод строки

```bash
replacer file.txt "'test'+.+.+.+'end':'MATCH'"
```
**English:** Match "test" followed by exactly 3 bytes, then "end"  
**Русский:** Найти "test" за которым следуют ровно 3 байта, затем "end"

```bash
replacer data.bin 0xAA+\?+0xBB+\?+0xCC:0xFF
```
**English:** Match hex patterns with optional bytes: 0xAA [optional] 0xBB [optional] 0xCC  
**Русский:** Найти hex паттерны с необязательными байтами: 0xAA [необязательный] 0xBB [необязательный] 0xCC

```bash
replacer html.txt "'<'+*+'>':"
```
**English:** Remove all HTML tags (match < + any chars + >, replace with empty)  
**Русский:** Удалить все HTML теги (найти < + любые символы + >, заменить на пустоту)

### 10. Capture and escape sequences / Захват и escape последовательности

**NEW SYNTAX (26.0420):**

```cmd
replacer.exe "file.txt":- "'error':'[\0]'"
```
**English:** Capture entire match with `\0` - outputs `[error]`  
**Русский:** Захватить всё вхождение с `\0` - выведет `[error]`

```cmd
replacer.exe "file.txt":- "'word':'\0 and \0'"
```
**English:** Duplicate matched text - outputs `word and word`  
**Русский:** Дублировать найденный текст - выведет `word and word`

```cmd
replacer.exe "file.txt":- "'Version: '+*:'Found: \0'"
```
**English:** Capture wildcard match - `Version: 1.2` becomes `Found: Version: 1.2`  
**Русский:** Захватить wildcard совпадение - `Version: 1.2` станет `Found: Version: 1.2`

```cmd
replacer.exe "file.txt":- "'line':'text\n'"
```
**English:** Add newline using `\n` escape sequence in replacement  
**Русский:** Добавить перевод строки используя escape последовательность `\n` в замене

```cmd
replacer.exe "file.txt":- "'text':'\t\0\t'"
```
**English:** Wrap match in tabs using `\t` escape  
**Русский:** Обернуть совпадение в табуляции используя `\t` escape

```cmd
replacer.exe "file.txt":- "'colo\*\x72':'change color'"
```
**English:** Hex escape `\xHH` in search pattern  
**Русский:** Hex escape `\xHH` в паттерне поиска

```cmd
replacer.exe "file.txt":- "'path':'C:\\new'"
```
**English:** Backslash escape `\\` in replacement  
**Русский:** Escape обратного слэша `\\` в замене

**Supported escape sequences in replacements / Поддерживаемые escape последовательности в заменах:**
- `\0` → entire matched pattern / весь найденный паттерн
- `\n` → newline (0x0A)
- `\r` → carriage return (0x0D)
- `\t` → tab (0x09)
- `\\` → backslash (0x5C)
- `\xHH` → hex byte (e.g., `\x0A`, `\xFF`)

**Wildcards (outside quotes only) / Wildcards (только вне кавычек):**
- `*` → zero or more bytes / ноль или более байтов
- `.` → any single byte / любой один байт
- `?` → optional byte / необязательный байт

### 11. Stdin/stdout mode / Режим stdin/stdout

```bash
replacer - 0xAA:0xBB < input.bin > output.bin
```
**English:** Read from stdin, replace bytes, write to stdout (full pipeline mode)  
**Русский:** Читать из stdin, заменить байты, записать в stdout (полный конвейерный режим)

```bash
type input.bin | replacer - 0xAA:0xBB > output.bin
```
**English:** Use with Windows `type` command in pipeline  
**Русский:** Использование с командой Windows `type` в конвейере

```bash
type file.txt | replacer - "'old':'new'"
```
**English:** Text replacement in pipeline  
**Русский:** Текстовая замена в конвейере

```bash
replacer - "'old':'new'" < file.txt
```
**English:** Read from redirected stdin, output to console  
**Русский:** Читать из перенаправленного stdin, вывод в консоль

```bash
replacer - "'old':'new'" < file.txt 2>nul
```
**English:** Same as above but suppress error messages (redirect stderr to nul)  
**Русский:** То же самое, но подавить сообщения об ошибках (перенаправить stderr в nul)

```bash
replacer win:-:utf < input.txt > output.txt
```
**English:** Convert Windows-1251 stdin to UTF-8 stdout using redirection  
**Русский:** Конвертировать Windows-1251 из stdin в UTF-8 в stdout используя перенаправление

```bash
replacer dos:-:utf < dos_file.txt > utf_file.txt
```
**English:** Convert DOS (CP866) file to UTF-8 using stdin/stdout  
**Русский:** Конвертировать DOS (CP866) файл в UTF-8 используя stdin/stdout

### 12. Mixed operations / Смешанные операции

```bash
echo test123  line 1  line 2  line 3  | replacer -:- 0x2020:'\n' "'test':'demo'" $20:'\t#' "'\n':'.\n'"  "'.\n\r.':"
```  
**Русский:** Заменяем 2 пробела на перенос строки, заменяем текст "test" на "demo", заменить `$20`(пробел) на табуляцию и #, перед всем концами строки поставить точку. Заменяем все точки на одну, которые вначале строки после строки с точкой.

```bash
replacer file.txt:output.txt "win@utf" "'старый':'новый'" "'test':'тест'"
```
**English:** Convert Windows-1251 to UTF-8 and perform replacements  
**Русский:** Конвертировать Windows-1251 в UTF-8 и выполнить замены

```bash
replacer file.txt:- 0x0D0A:0x0A
```
**English:** Convert Windows line endings (CRLF) to Unix (LF) and output to stdout  
**Русский:** Конвертировать Windows окончания строк (CRLF) в Unix (LF) и вывести в stdout

### 13. Practical use cases / Практические примеры использования

```bash
replacer config.ini "'localhost':'192.168.1.100'"
```
**English:** Update server address in configuration file  
**Русский:** Обновить адрес сервера в конфигурационном файле

```bash
replacer autoexec.bat "dos@win" "'C:\\OLD':'C:\\NEW'"
```
**English:** Update paths in DOS batch file, convert to Windows encoding  
**Русский:** Обновить пути в DOS batch файле, конвертировать в Windows кодировку

```bash
replacer firmware.bin 0x00FF00FF:0xFF00FF00
```
**English:** Patch binary firmware file  
**Русский:** Пропатчить бинарный файл прошивки

```bash
type template.html | replacer - "'{{NAME}}':'John'" "'{{DATE}}':'2026-04-17'" > output.html
```
**English:** Simple template engine using stdin/stdout  
**Русский:** Простой шаблонизатор используя stdin/stdout

```bash
replacer legacy.txt "win@utf" "'©':'(c)'" "'®':'(R)'"
```
**English:** Replace special characters in legacy Windows-1251 file, output as UTF-8  
**Русский:** Заменить специальные символы в старом файле Windows-1251, вывод в UTF-8

```bash
replacer access.log "'IP: '+*+' -':'IP: [REDACTED] -'"
```
**English:** Anonymize IP addresses in log files using wildcard pattern  
**Русский:** Анонимизировать IP адреса в лог файлах используя wildcard паттерн

```bash
replacer source.cpp "'TODO'+*+0x0A:'DONE'+0x0A"
```
**English:** Mark all TODO comments as DONE in source code  
**Русский:** Пометить все TODO комментарии как DONE в исходном коде

```bash
replacer data.csv "','+*+',':','"
```
**English:** Remove content between commas in CSV (clean empty fields)  
**Русский:** Удалить содержимое между запятыми в CSV (очистить пустые поля)

```bash
replacer email.txt "'password'+?+':'+*+0x0A:'password: [HIDDEN]'+0x0A"
```
**English:** Redact passwords from email dumps (handles "password:" and "passwords:")  
**Русский:** Скрыть пароли из дампов email (обрабатывает "password:" и "passwords:")

```bash
replacer binary.dat 0xDEADBEEF+\.+\.+\.+\.:0xCAFEBABE+0x00000000
```
**English:** Patch binary signature: find 0xDEADBEEF + 4 bytes, replace with 0xCAFEBABE + 4 zeros  
**Русский:** Пропатчить бинарную сигнатуру: найти 0xDEADBEEF + 4 байта, заменить на 0xCAFEBABE + 4 нуля

```bash
replacer script.bat "'REM'+*+0x0D0A:0x0D0A"
```
**English:** Remove all REM comments from batch file (keep line breaks)  
**Русский:** Удалить все REM комментарии из batch файла (сохранить переводы строк)

```bash
replacer json.txt "0x22+'token'+0x22+?+':'+?+0x22+*+0x22:0x22+'token'+0x22+': '+0x22+'***'+0x22"
```
**English:** Redact JSON tokens with flexible spacing: `"token":"value"` or `"token" : "value"`  
**Русский:** Скрыть JSON токены с гибкими пробелами: `"token":"value"` или `"token" : "value"`

### 14. Capture Groups Examples / Примеры групп захвата

**NEW in 26.0421 / НОВОЕ в 26.0421**

```cmd
REM Wrap matched text in brackets
replacer.exe "file.txt":- "'error':'[\0]'"
```
**English:** Input: `error` → Output: `[error]`  
**Русский:** Вход: `error` → Выход: `[error]`

```cmd
REM Duplicate matched text
replacer.exe "file.txt":- "'word':'\0 and \0'"
```
**English:** Input: `test` → Output: `test and test`  
**Русский:** Вход: `test` → Выход: `test and test`

```cmd
REM Swap log level and message
replacer.exe "log.txt":- "'['+{level=*}+'] '+{msg=*}:{msg}+' ['+{level}+']'"
```
**English:** Input: `[ERROR] File not found` → Output: `File not found [ERROR]`  
**Русский:** Вход: `[ERROR] File not found` → Выход: `File not found [ERROR]`

```cmd
REM Extract and reformat version numbers
replacer.exe "file.txt":- "'Version '+{major=*}+'.'+{minor=*}+'.'+{patch=*}:'v'+{major}+'.'+{minor}+'.'+{patch}"
```
**English:** Input: `Version 1.2.3` → Output: `v1.2.3`  
**Русский:** Вход: `Version 1.2.3` → Выход: `v1.2.3`

```cmd
REM Reorder date format (DD.MM.YYYY to YYYY-MM-DD)
replacer.exe "dates.txt":- "{day=*}+'.'+{month=*}+'.'+{year=*}:{year}+'-'+{month}+'-'+{day}"
```
**English:** Input: `25.12.2025` → Output: `2025-12-25`  
**Русский:** Вход: `25.12.2025` → Выход: `2025-12-25`

```cmd
REM Extract email username
replacer.exe "emails.txt":- "{user=*}+'@'+*:{user}"
```
**English:** Input: `john@example.com` → Output: `john`  
**Русский:** Вход: `john@example.com` → Выход: `john`

```cmd
REM Anonymize phone numbers but keep format
echo +7 (905) 098-76-54 | replacer -d -:- "'+7 ('+{code=*}+') '+{num1=*}+'-'+{num2=*}+'-'+{num3=*}:'+7 ('+{code}+') '+{num1}+'-\3-XX'"
```
**English:** Input: `+7 (905) 098-76-54` → Output: `+7 (905) 098-76-XX`  

```cmd
REM Reformat function calls: func(arg) to arg.func()
replacer.exe "code.txt":- "{fn=*}+'('+{arg=*}+')':'{arg}+'.'+{fn}+'()'"
```
**English:** Input: `print(message)` → Output: `message.print()`  
**Русский:** Вход: `print(message)` → Выход: `message.print()`

```cmd
REM Triple the matched word
replacer.exe "file.txt":- "{w=*}:{w}+{w}+{w}"
```
**English:** Input: `test` → Output: `testtesttest`  
**Русский:** Вход: `test` → Выход: `testtesttest`

### 16. Counter Examples / Примеры счётчиков

**NEW in 26.0425 / НОВОЕ в 26.0425**

**Simple numbering / Простая нумерация:**
```cmd
replacer.exe "tasks.txt":- "'TODO':'Task '+{#}"
```
**English:** Input: `TODO`, `TODO`, `TODO` → Output: `Task 1`, `Task 2`, `Task 3`  
**Русский:** Вход: `TODO`, `TODO`, `TODO` → Выход: `Task 1`, `Task 2`, `Task 3`

**Start from specific number / Начать с определённого числа:**
```cmd
replacer.exe "items.txt":- "'Item':'Item '+{#:10}"
```
**English:** Input: `Item`, `Item`, `Item` → Output: `Item 10`, `Item 11`, `Item 12`  
**Русский:** Вход: `Item`, `Item`, `Item` → Выход: `Item 10`, `Item 11`, `Item 12`

**Zero-padded format / Формат с нулями:**
```cmd
replacer.exe "files.txt":- "'file.txt':'file_'+{#:001}+'.txt'"
```
**English:** Input: `file.txt`, `file.txt`, `file.txt` → Output: `file_001.txt`, `file_002.txt`, `file_003.txt`  
**Русский:** Вход: `file.txt`, `file.txt`, `file.txt` → Выход: `file_001.txt`, `file_002.txt`, `file_003.txt`

**Range with auto-stop / Диапазон с автостопом:**
```cmd
replacer.exe "list.txt":- "'item':'Priority '+{#:1-3}+' item'"
```
**English:** Replaces only first 3 occurrences, rest unchanged  
**Русский:** Заменяет только первые 3 вхождения, остальные без изменений

**Hexadecimal format / Шестнадцатеричный формат:**
```cmd
replacer.exe "addresses.txt":- "'ADDR':'0x'+{#:0x100}"
```
**English:** Input: `ADDR`, `ADDR`, `ADDR` → Output: `0x0x100`, `0x0x101`, `0x0x102`  
**Русский:** Вход: `ADDR`, `ADDR`, `ADDR` → Выход: `0x0x100`, `0x0x101`, `0x0x102`

**Alphabetic numbering / Алфавитная нумерация:**
```cmd
replacer.exe "sections.txt":- "'Section':'Section '+{#:A}"
```
**English:** Input: `Section` (30 times) → Output: `Section A`, `Section B`, ... `Section Z`, `Section AA`, `Section AB`...  
**Русский:** Вход: `Section` (30 раз) → Выход: `Section A`, `Section B`, ... `Section Z`, `Section AA`, `Section AB`...

**Combined with capture groups / Комбинация с группами захвата:**
```cmd
replacer.exe "errors.txt":- "'error':'['+{#}+'] \0'"
```
**English:** Input: `error`, `warning`, `error` → Output: `[1] error`, `warning`, `[2] error`  
**Русский:** Вход: `error`, `warning`, `error` → Выход: `[1] error`, `warning`, `[2] error`

**Numbered list creation / Создание нумерованного списка:**
```cmd
replacer.exe "bullets.txt":- "'- ':''+{#}+'. '"
```
**English:** Input: `- Item`, `- Item`, `- Item` → Output: `1. Item`, `2. Item`, `3. Item`  
**Русский:** Вход: `- Item`, `- Item`, `- Item` → Выход: `1. Item`, `2. Item`, `3. Item`

### 15. Practical Use Cases / Практические примеры

**Log file processing / Обработка лог-файлов:**
```cmd
REM Extract timestamps from logs
replacer.exe "app.log":- "'['+{time=*}+'] '+{level=*}+': '+{msg=*}:{time}+' | '+{msg}"
REM Input: [2026-04-20 10:30:15] ERROR: Connection failed
REM Output: 2026-04-20 10:30:15 | Connection failed
```

**Configuration file updates / Обновление конфигурационных файлов:**
```cmd
REM Update port numbers while preserving parameter names
replacer.exe "config.ini":- "{param=*}+'='+*:{param}+'=8080'"
REM Input: server_port=3000
REM Output: server_port=8080
```

**Source code refactoring / Рефакторинг исходного кода:**
```cmd
REM Convert old-style to new-style function calls
replacer.exe "legacy.js":- "'oldFunc('+{args=*}+')':'newFunc('+{args}+')'"
REM Input: oldFunc(x, y, z)
REM Output: newFunc(x, y, z)
```

**Data anonymization / Анонимизация данных:**
```cmd
REM Mask credit card numbers but keep last 4 digits
replacer.exe "transactions.txt":- "*+'-'+*+'-'+*+'-'+{last=*}:'****-****-****-'+{last}"
REM Input: 1234-5678-9012-3456
REM Output: ****-****-****-3456
```

**URL manipulation / Манипуляция URL:**
```cmd
REM Change protocol from http to https
replacer.exe "links.txt":- "'http://'+{domain=*}:'https://'+{domain}"
REM Input: http://example.com/page
REM Output: https://example.com/page
```

**CSV/TSV processing / Обработка CSV/TSV:**
```cmd
REM Swap first and last columns in CSV
replacer.exe "data.csv":- "{first=*}+','+{middle=*}+','+{last=*}:{last}+','+{middle}+','+{first}"
REM Input: John,Doe,30
REM Output: 30,Doe,John
```

**HTML/XML tag manipulation / Манипуляция HTML/XML тегами:**
```cmd
REM Extract content from tags
replacer.exe "page.html":- "'<title>'+{content=*}+'</title>':{content}"
REM Input: <title>My Page</title>
REM Output: My Page
```

**Batch renaming patterns / Паттерны пакетного переименования:**
```cmd
REM Generate rename commands from file list
replacer.exe "files.txt":- "{name=*}+'.txt':'ren \"'+{name}+'.txt\" \"new_'+{name}+'.txt\"'"
REM Input: document.txt
REM Output: ren "document.txt" "new_document.txt"
```

**Case-insensitive text normalization / Нормализация текста без учёта регистра:**
```cmd
REM Normalize error messages regardless of case
replacer.exe "logs.txt":- "'error'+*+'failed':'ERROR: Operation failed'/i"
REM Input: Error 123 failed, ERROR abc FAILED, error XYZ Failed
REM Output: ERROR: Operation failed (all variants)

REM Fix inconsistent keywords in code
replacer.exe "code.sql":- "'select':'SELECT'/i 'from':'FROM'/i 'where':'WHERE'/i"
REM Input: Select * From users Where id=1
REM Output: SELECT * FROM users WHERE id=1
```

**Numbering with counters / Нумерация со счётчиками:**
```cmd
REM Number TODO items in source code
replacer.exe "code.js":- "'// TODO':'// TODO '+{#}+':'"
REM Input: // TODO fix this, // TODO refactor, // TODO test
REM Output: // TODO 1: fix this, // TODO 2: refactor, // TODO 3: test

REM Create numbered list from bullet points
replacer.exe "notes.txt":- "'* ':''+{#}+'. '"
REM Input: * First item, * Second item, * Third item
REM Output: 1. First item, 2. Second item, 3. Third item

REM Generate sequential filenames
replacer.exe "batch.txt":- "'output.txt':'file_'+{#:001}+'.txt'"
REM Input: output.txt (repeated 5 times)
REM Output: file_001.txt, file_002.txt, file_003.txt, file_004.txt, file_005.txt

REM Add line numbers to log entries
replacer.exe "app.log":- "'['+{time=*}+']':'['+{#:0001}+'] ['+{time}+']'"
REM Input: [10:30:15] Error, [10:30:16] Warning
REM Output: [0001] [10:30:15] Error, [0002] [10:30:16] Warning

REM Label test cases alphabetically
replacer.exe "tests.txt":- "'test(':'test'+{#:A}+'('"
REM Input: test(), test(), test()
REM Output: testA(), testB(), testC()
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

Version / Версия: 26.0421
