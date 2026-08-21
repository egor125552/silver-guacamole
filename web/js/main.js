import {Game} from "./core/game.js";

const startButton = document.querySelector("#start");
const status = document.querySelector("#status");
let game = null;

async function start() {
  if (game) return;
  startButton.disabled = true;
  status.textContent = "Загружаю звуки...";
  try {
    game = new Game(status);
    await game.start();
    startButton.hidden = true;
    status.textContent = "Игра запущена. Стрелки или WASD — движение. E — сонар.";
  } catch (error) {
    console.error(error);
    status.textContent = `Ошибка запуска: ${error.message}`;
    startButton.disabled = false;
    game = null;
  }
}
startButton.addEventListener("click", start);
addEventListener("keydown", e => {
  if (!game && (e.key === "Enter" || e.key === " ")) {
    e.preventDefault();
    start();
  }
});
