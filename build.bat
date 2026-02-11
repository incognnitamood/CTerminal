@echo off
setlocal
if not exist backend (
  echo backend directory not found
  exit /b 1
)
gcc -Wall -Wextra -o terminal.exe ^
  backend\main.c backend\utils.c backend\filesystem.c backend\history.c ^
  backend\stack.c backend\hashmap.c backend\parser.c backend\trie.c ^
  backend\logger.c backend\commands.c
if %errorlevel% neq 0 (
  echo Build failed
  exit /b 1
)
echo Built terminal.exe successfully
endlocal

