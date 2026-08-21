#!/bin/zsh
cd "$(dirname "$0")" || exit 1

if ! command -v python3 >/dev/null 2>&1; then
  echo "Для локального запуска нужен Python 3."
  echo "Установите Python 3 и снова откройте этот файл."
  read -k 1 "?Нажмите любую клавишу для выхода..."
  exit 1
fi

PORT=8765
while lsof -nP -iTCP:"$PORT" -sTCP:LISTEN >/dev/null 2>&1; do
  PORT=$((PORT + 1))
done
LOG="/tmp/stealth-action-web-$PORT.log"

python3 -m http.server "$PORT" --bind 127.0.0.1 >"$LOG" 2>&1 &
SERVER_PID=$!

cleanup() {
  kill "$SERVER_PID" 2>/dev/null
}
trap cleanup EXIT INT TERM

sleep 0.7
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
  echo "Локальный сервер не запустился. Лог: $LOG"
  cat "$LOG"
  read -k 1 "?Нажмите любую клавишу для выхода..."
  exit 1
fi

URL="http://127.0.0.1:$PORT/web/"
open "$URL"
echo "Stealth Action Web запущена: $URL"
echo "Не закрывайте это окно Terminal во время игры."
wait "$SERVER_PID"
