#!/bin/bash

# --- Установки ---
GAME_EXECUTABLE="./build/StealthActionGame"
LOG_FILE="error_log.txt"
CONFIG_FILE="config.ini"
STUN_LOG_MESSAGE="DEBUG_STUN"

# --- Функция для вывода сообщений ---
info() {
    echo "[INFO] $1"
}

error() {
    echo "[ERROR] $1" >&2
}

# --- Функция для очистки ---
cleanup() {
    info "Выполняется очистка..."
    if [ -f "$CONFIG_FILE.bak" ]; then
        mv "$CONFIG_FILE.bak" "$CONFIG_FILE"
        info "Восстановлен оригинальный $CONFIG_FILE."
    fi
    if [ ! -z "$GAME_PID" ]; then
        kill $GAME_PID 2>/dev/null
        wait $GAME_PID 2>/dev/null
    fi
}

# Перехватываем выход из скрипта для очистки
trap cleanup EXIT

# --- 1. Сборка проекта ---
info "Собираем проект..."
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"
if ! make; then
    error "Сборка проекта провалилась."
    exit 1
fi
cd ..

# --- 2. Подготовка к запуску ---
info "Подготовка к запуску..."
if [ -f "$LOG_FILE" ]; then
    rm "$LOG_FILE"
fi

info "Временно увеличиваем дальность электрошокера в $CONFIG_FILE..."
cp "$CONFIG_FILE" "$CONFIG_FILE.bak"
sed -i 's/^\(TaserRange\s*=\s*\).*/\1200.0/' "$CONFIG_FILE"

# --- 3. Запуск игры ---
info "Запускаем игру в фоновом режиме..."
xvfb-run -a $GAME_EXECUTABLE > game_output.log 2>&1 &
GAME_PID=$!
info "Игра запущена с PID: $GAME_PID"

# --- 4. Эмуляция ввода ---
info "Ожидаем появления окна игры..."
# Используем --sync, чтобы дождаться появления окна.
WINDOW_ID=$(xdotool search --sync --name "Stealth Action - Coordinated Assault" | head -1)

if [ -z "$WINDOW_ID" ]; then
    error "Не удалось найти окно игры. Тест не может быть продолжен."
    info "Вывод игры (game_output.log):"
    cat game_output.log
    exit 1
fi

info "Окно игры найдено: $WINDOW_ID. Отправляем команды..."
sleep 1

xdotool windowactivate $WINDOW_ID &>/dev/null
sleep 0.5
info "Нажимаем '3' для выбора электрошокера."
xdotool key --window $WINDOW_ID 3
sleep 0.5
info "Нажимаем 'Пробел' для атаки."
xdotool key --window $WINDOW_ID space
sleep 1

# --- 5. Проверка логов ---
info "Завершаем игру и проверяем логи..."
kill $GAME_PID
wait $GAME_PID 2>/dev/null

if [ -f "$LOG_FILE" ] && grep -q "$STUN_LOG_MESSAGE" "$LOG_FILE"; then
    info "========================================"
    info "    ТЕСТ ПРОЙДЕН УСПЕШНО! ✅"
    info "    Сообщение об оглушении найдено."
    info "========================================"
    info "Найденные строки в логе:"
    grep "$STUN_LOG_MESSAGE" "$LOG_FILE"
    exit 0
else
    error "========================================"
    error "    ТЕСТ ПРОВАЛЕН! ❌"
    error "    Сообщение об оглушении НЕ найдено."
    error "========================================"
    info "Содержимое лог-файла ($LOG_FILE):"
    cat "$LOG_FILE" 2>/dev/null || echo "Лог-файл пуст или не существует."
    info "----------------------------------------"
    info "Вывод игры (game_output.log):"
    cat game_output.log
    exit 1
fi
